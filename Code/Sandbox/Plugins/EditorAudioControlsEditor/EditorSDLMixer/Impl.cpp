// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Impl.h"

#include "Common.h"
#include "EventConnection.h"
#include "ParameterConnection.h"
#include "StateConnection.h"
#include "PreloadConnection.h"
#include "ProjectLoader.h"
#include "DataPanel.h"
#include "Utils.h"

#include <QtUtil.h>
#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySystem/XML/IXml.h>
#include <DragDrop.h>

#include <QDirIterator>

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
constexpr uint32 g_itemPoolSize = 2048;
constexpr uint32 g_eventConnectionPoolSize = 2048;
constexpr uint32 g_parameterConnectionPoolSize = 256;
constexpr uint32 g_stateConnectionPoolSize = 256;
constexpr uint32 g_preloadConnectionPoolSize = 256;

//////////////////////////////////////////////////////////////////////////
void CountConnections(EAssetType const assetType, CryAudio::ContextId const contextId, bool const isAdvanced)
{
	switch (assetType)
	{
	case EAssetType::Trigger:
		{
			++g_connections[contextId].events;
			break;
		}
	case EAssetType::Parameter:
		{
			if (isAdvanced)
			{
				++g_connections[contextId].parametersAdvanced;
			}
			else
			{
				++g_connections[contextId].parameters;
			}

			break;
		}
	case EAssetType::State:
		{
			++g_connections[contextId].switchStates;
			break;
		}
	case EAssetType::Preload:
		{
			++g_connections[contextId].files;
			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool HasDirValidData(QDir const& dir)
{
	bool hasValidData = false;

	if (dir.exists())
	{
		QDirIterator itFiles(dir.path(), (QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot));

		while (itFiles.hasNext())
		{
			QFileInfo const fileInfo(itFiles.next());

			if (fileInfo.isFile())
			{
				hasValidData = true;
				break;
			}
		}

		if (!hasValidData)
		{
			QDirIterator itDirs(dir.path(), (QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot));

			while (itDirs.hasNext())
			{
				QDir const folder(itDirs.next());

				if (HasDirValidData(folder))
				{
					hasValidData = true;
					break;
				}
			}
		}
	}

	return hasValidData;
}

//////////////////////////////////////////////////////////////////////////
void GetFilesFromDir(QDir const& dir, QString const& folderName, FileImportInfos& fileImportInfos)
{
	if (dir.exists())
	{
		QString const parentFolderName = (folderName.isEmpty() ? (dir.dirName() + "/") : (folderName + dir.dirName() + "/"));

		for (auto const& fileInfo : dir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot))
		{
			if (fileInfo.isFile())
			{
				bool isSupportedType = false;

				for (auto const& pair : CryAudio::Impl::SDL_mixer::g_supportedExtensions)
				{
					if (fileInfo.suffix().compare(pair.first, Qt::CaseInsensitive) == 0)
					{
						isSupportedType = true;
						break;
					}
				}

				fileImportInfos.emplace_back(fileInfo, isSupportedType, parentFolderName);
			}
		}

		for (auto const& fileInfo : dir.entryInfoList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
		{
			QDir const folder(fileInfo.absoluteFilePath());
			GetFilesFromDir(folder, parentFolderName, fileImportInfos);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_assetAndProjectPath(CRY_AUDIO_DATA_ROOT "/" +
	                        string(CryAudio::Impl::SDL_mixer::g_szImplFolderName) +
	                        "/"
	                        + string(CryAudio::g_szAssetsFolderName))
	, m_localizedAssetsPath(m_assetAndProjectPath)
{
}

//////////////////////////////////////////////////////////////////////////
CImpl::~CImpl()
{
	Clear();

	if (g_pDataPanel != nullptr)
	{
		delete g_pDataPanel;
	}

	CItem::FreeMemoryPool();
	CEventConnection::FreeMemoryPool();
	CParameterConnection::FreeMemoryPool();
	CStateConnection::FreeMemoryPool();
	CPreloadConnection::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Initialize(
	SImplInfo& implInfo,
	ExtensionFilterVector& extensionFilters,
	QStringList& supportedFileTypes)
{
	CItem::CreateAllocator(g_itemPoolSize);
	CEventConnection::CreateAllocator(g_eventConnectionPoolSize);
	CParameterConnection::CreateAllocator(g_parameterConnectionPoolSize);
	CStateConnection::CreateAllocator(g_stateConnectionPoolSize);
	CPreloadConnection::CreateAllocator(g_preloadConnectionPoolSize);

	CryAudio::SImplInfo systemImplInfo;
	gEnv->pAudioSystem->GetImplInfo(systemImplInfo);
	m_implName = systemImplInfo.name;

	SetImplInfo(implInfo);

	for (auto const& pair : CryAudio::Impl::SDL_mixer::g_supportedExtensions)
	{
		extensionFilters.push_back({ pair.second, pair.first });
		supportedFileTypes.push_back(pair.first);
	}
}

//////////////////////////////////////////////////////////////////////////
QWidget* CImpl::CreateDataPanel(QWidget* const pParent)
{
	g_pDataPanel = new CDataPanel(*this, pParent);
	return g_pDataPanel;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Reload(SImplInfo& implInfo)
{
	Clear();
	SetImplInfo(implInfo);

	CProjectLoader(m_assetAndProjectPath, m_localizedAssetsPath, m_rootItem, m_itemCache, *this);

	for (auto const& connection : m_connectionsByID)
	{
		if (connection.second > 0)
		{
			auto const pItem = static_cast<CItem* const>(GetItem(connection.first));

			if (pItem != nullptr)
			{
				pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsConnected);
			}
		}
	}

	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
IItem* CImpl::GetItem(ControlId const id) const
{
	IItem* pIItem = nullptr;

	if (id >= 0)
	{
		pIItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pIItem;
}

//////////////////////////////////////////////////////////////////////////
CryIcon const& CImpl::GetItemIcon(IItem const* const pIItem) const
{
	auto const pItem = static_cast<CItem const* const>(pIItem);
	CRY_ASSERT_MESSAGE(pItem != nullptr, "Impl item is null pointer during %s", __FUNCTION__);
	return GetTypeIcon(pItem->GetType());
}

//////////////////////////////////////////////////////////////////////////
QString const& CImpl::GetItemTypeName(IItem const* const pIItem) const
{
	auto const pItem = static_cast<CItem const* const>(pIItem);
	CRY_ASSERT_MESSAGE(pItem != nullptr, "Impl item is null pointer during %s", __FUNCTION__);
	return TypeToString(pItem->GetType());
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::IsTypeCompatible(EAssetType const assetType, IItem const* const pIItem) const
{
	bool isCompatible = false;

	switch (assetType)
	{
	case EAssetType::Trigger:   // Intentional fall-through.
	case EAssetType::Parameter: // Intentional fall-through.
	case EAssetType::State:     // Intentional fall-through.
	case EAssetType::Preload:
		{
			auto const pItem = static_cast<CItem const*>(pIItem);
			isCompatible = (pItem->GetType() == EItemType::Event);
			break;
		}
	default:
		{
			isCompatible = false;
			break;
		}
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
EAssetType CImpl::ImplTypeToAssetType(IItem const* const pIItem) const
{
	EAssetType assetType = EAssetType::None;
	auto const pItem = static_cast<CItem const*>(pIItem);

	switch (pItem->GetType())
	{
	case EItemType::Event:
		{
			assetType = EAssetType::Trigger;
			break;
		}
	default:
		{
			assetType = EAssetType::None;
			break;
		}
	}

	return assetType;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionToControl(EAssetType const assetType, IItem const* const pIItem)
{
	IConnection* pIConnection = nullptr;

	switch (assetType)
	{
	case EAssetType::Parameter:
		{
			pIConnection = static_cast<IConnection*>(new CParameterConnection(pIItem->GetId()));
			break;
		}
	case EAssetType::State:
		{
			pIConnection = static_cast<IConnection*>(new CStateConnection(pIItem->GetId()));
			break;
		}
	case EAssetType::Preload:
		{
			pIConnection = static_cast<IConnection*>(new CPreloadConnection(pIItem->GetId()));
			break;
		}
	default:
		{
			pIConnection = static_cast<IConnection*>(new CEventConnection(pIItem->GetId()));
			break;
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::DuplicateConnection(EAssetType const assetType, IConnection* const pIConnection)
{
	IConnection* pNewIConnection = nullptr;

	switch (assetType)
	{
	case EAssetType::Trigger:
		{
			auto const pOldConnection = static_cast<CEventConnection*>(pIConnection);
			auto const pNewConnection = new CEventConnection(pOldConnection->GetID());

			pNewConnection->SetActionType(pOldConnection->GetActionType());
			pNewConnection->SetVolume(pOldConnection->GetVolume());
			pNewConnection->SetFadeInTime(pOldConnection->GetFadeInTime());
			pNewConnection->SetFadeOutTime(pOldConnection->GetFadeOutTime());
			pNewConnection->SetMinAttenuation(pOldConnection->GetMinAttenuation());
			pNewConnection->SetMaxAttenuation(pOldConnection->GetMaxAttenuation());
			pNewConnection->SetPanningEnabled(pOldConnection->IsPanningEnabled());
			pNewConnection->SetAttenuationEnabled(pOldConnection->IsAttenuationEnabled());
			pNewConnection->SetInfiniteLoop(pOldConnection->IsInfiniteLoop());
			pNewConnection->SetLoopCount(pOldConnection->GetLoopCount());

			pNewIConnection = static_cast<IConnection*>(pNewConnection);

			break;
		}
	case EAssetType::Parameter:
		{
			auto const pOldConnection = static_cast<CParameterConnection*>(pIConnection);
			auto const pNewConnection = new CParameterConnection(
				pOldConnection->GetID(),
				pOldConnection->IsAdvanced(),
				pOldConnection->GetMultiplier(),
				pOldConnection->GetShift());

			pNewIConnection = static_cast<IConnection*>(pNewConnection);

			break;
		}
	case EAssetType::State:
		{
			auto const pOldConnection = static_cast<CStateConnection*>(pIConnection);
			auto const pNewConnection = new CStateConnection(pOldConnection->GetID(), pOldConnection->GetValue());

			pNewIConnection = static_cast<IConnection*>(pNewConnection);

			break;
		}
	case EAssetType::Preload:
		{
			auto const pOldConnection = static_cast<CPreloadConnection*>(pIConnection);
			auto const pNewConnection = new CPreloadConnection(pOldConnection->GetID());

			pNewIConnection = static_cast<IConnection*>(pNewConnection);

			break;
		}
	default:
		{
			break;
		}
	}

	return pNewIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionFromXMLNode(XmlNodeRef const& node, EAssetType const assetType)
{
	IConnection* pIConnection = nullptr;

	if (node.isValid())
	{
		char const* const szTag = node->getTag();

		if ((_stricmp(szTag, CryAudio::Impl::SDL_mixer::g_szEventTag) == 0) ||
		    (_stricmp(szTag, CryAudio::Impl::SDL_mixer::g_szSampleTag) == 0) ||
		    (_stricmp(szTag, "SDLMixerEvent") == 0) || // Backwards compatibility.
		    (_stricmp(szTag, "SDLMixerSample") == 0))  // Backwards compatibility.
		{
			string name = node->getAttr(CryAudio::g_szNameAttribute);
			string path = node->getAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute);
			// Backwards compatibility will be removed with CE 5.7.
#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && node->haveAttr("sdl_name"))
			{
				name = node->getAttr("sdl_name");
			}

			if (path.IsEmpty() && node->haveAttr("sdl_path"))
			{
				path = node->getAttr("sdl_path");
			}
#endif      // USE_BACKWARDS_COMPATIBILITY
			string const localizedAttribute = node->getAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute);
			bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::SDL_mixer::g_szTrueValue) == 0);
			ControlId const id = Utils::GetId(EItemType::Event, name, path, isLocalized);

			auto pItem = static_cast<CItem*>(GetItem(id));

			if (pItem == nullptr)
			{
				EItemFlags const flags = (isLocalized ? (EItemFlags::IsPlaceHolder | EItemFlags::IsLocalized) : EItemFlags::IsPlaceHolder);
				pItem = new CItem(name, id, EItemType::Event, path, flags);
				m_itemCache[id] = pItem;
			}

			if (pItem != nullptr)
			{
				switch (assetType)
				{
				case EAssetType::Trigger:
					{
						auto const pEventConnection = new CEventConnection(pItem->GetId());
						string actionType = node->getAttr(CryAudio::g_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (actionType.IsEmpty() && node->haveAttr("event_type"))
						{
							actionType = node->getAttr("event_type");
						}
#endif            // USE_BACKWARDS_COMPATIBILITY
						if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::g_szStopValue) == 0)
						{
							pEventConnection->SetActionType(CEventConnection::EActionType::Stop);
						}
						else if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::g_szPauseValue) == 0)
						{
							pEventConnection->SetActionType(CEventConnection::EActionType::Pause);
						}
						else if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::g_szResumeValue) == 0)
						{
							pEventConnection->SetActionType(CEventConnection::EActionType::Resume);
						}
						else
						{
							pEventConnection->SetActionType(CEventConnection::EActionType::Start);

							float fadeInTime = CryAudio::Impl::SDL_mixer::g_defaultFadeInTime;
							node->getAttr(CryAudio::Impl::SDL_mixer::g_szFadeInTimeAttribute, fadeInTime);
							pEventConnection->SetFadeInTime(fadeInTime);

							float fadeOutTime = CryAudio::Impl::SDL_mixer::g_defaultFadeOutTime;
							node->getAttr(CryAudio::Impl::SDL_mixer::g_szFadeOutTimeAttribute, fadeOutTime);
							pEventConnection->SetFadeOutTime(fadeOutTime);
						}

						string const enablePanning = node->getAttr(CryAudio::Impl::SDL_mixer::g_szPanningEnabledAttribute);
						pEventConnection->SetPanningEnabled(enablePanning.compareNoCase(CryAudio::Impl::SDL_mixer::g_szTrueValue) == 0);

						string enableDistAttenuation = node->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationEnabledAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (enableDistAttenuation.IsEmpty() && node->haveAttr("enable_distance_attenuation"))
						{
							enableDistAttenuation = node->getAttr("enable_distance_attenuation");
						}
#endif            // USE_BACKWARDS_COMPATIBILITY
						pEventConnection->SetAttenuationEnabled(enableDistAttenuation.compareNoCase(CryAudio::Impl::SDL_mixer::g_szTrueValue) == 0);

						float minAttenuation = CryAudio::Impl::SDL_mixer::g_defaultMinAttenuationDist;
						node->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMinDistanceAttribute, minAttenuation);
						pEventConnection->SetMinAttenuation(minAttenuation);

						float maxAttenuation = CryAudio::Impl::SDL_mixer::g_defaultMaxAttenuationDist;
						node->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMaxDistanceAttribute, maxAttenuation);
						pEventConnection->SetMaxAttenuation(maxAttenuation);

						float volume = pEventConnection->GetVolume();
						node->getAttr(CryAudio::Impl::SDL_mixer::g_szVolumeAttribute, volume);
						pEventConnection->SetVolume(volume);

						int loopCount = pEventConnection->GetLoopCount();
						node->getAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, loopCount);
						loopCount = std::max(0, loopCount); // Delete this when backwards compatibility gets removed and use uint32 directly.
						pEventConnection->SetLoopCount(static_cast<uint32>(loopCount));

						if (pEventConnection->GetLoopCount() == 0)
						{
							pEventConnection->SetInfiniteLoop(true);
						}

						pIConnection = static_cast<IConnection*>(pEventConnection);

						break;
					}
				case EAssetType::Parameter:
					{
						bool isAdvanced = false;

						float mult = CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier;
						float shift = CryAudio::Impl::SDL_mixer::g_defaultParamShift;

						isAdvanced |= node->getAttr(CryAudio::Impl::SDL_mixer::g_szMutiplierAttribute, mult);
						isAdvanced |= node->getAttr(CryAudio::Impl::SDL_mixer::g_szShiftAttribute, shift);

						pIConnection = static_cast<IConnection*>(new CParameterConnection(pItem->GetId(), isAdvanced, mult, shift));

						break;
					}
				case EAssetType::State:
					{
						float value = CryAudio::Impl::SDL_mixer::g_defaultStateValue;
						node->getAttr(CryAudio::Impl::SDL_mixer::g_szValueAttribute, value);

						pIConnection = static_cast<IConnection*>(new CStateConnection(pItem->GetId(), value));

						break;
					}
				case EAssetType::Preload:
					{
						pIConnection = static_cast<IConnection*>(new CPreloadConnection(pItem->GetId()));

						break;
					}
				default:
					{
						break;
					}
				}
			}
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::CreateXMLNodeFromConnection(
	IConnection const* const pIConnection,
	EAssetType const assetType,
	CryAudio::ContextId const contextId)
{
	XmlNodeRef node;

	auto const pItem = static_cast<CItem const*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		bool isAdvanced = false;

		switch (assetType)
		{
		case EAssetType::Trigger:
			{
				auto const pEventConnection = static_cast<CEventConnection const*>(pIConnection);

				if (pEventConnection != nullptr)
				{
					node = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					node->setAttr(CryAudio::g_szNameAttribute, pItem->GetName().c_str());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, path.c_str());
					}

					switch (pEventConnection->GetActionType())
					{
					case CEventConnection::EActionType::Start:
						{
							node->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szStartValue);
							node->setAttr(CryAudio::Impl::SDL_mixer::g_szVolumeAttribute, pEventConnection->GetVolume());

							if (pEventConnection->IsPanningEnabled())
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szPanningEnabledAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
							}

							if (pEventConnection->IsAttenuationEnabled())
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationEnabledAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMinDistanceAttribute, pEventConnection->GetMinAttenuation());
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMaxDistanceAttribute, pEventConnection->GetMaxAttenuation());
							}

							if (pEventConnection->GetFadeInTime() != CryAudio::Impl::SDL_mixer::g_defaultFadeInTime)
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szFadeInTimeAttribute, pEventConnection->GetFadeInTime());
							}

							if (pEventConnection->GetFadeOutTime() != CryAudio::Impl::SDL_mixer::g_defaultFadeOutTime)
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szFadeOutTimeAttribute, pEventConnection->GetFadeOutTime());
							}

							if (pEventConnection->IsInfiniteLoop())
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, 0);
							}
							else
							{
								node->setAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, pEventConnection->GetLoopCount());
							}

							break;
						}
					case CEventConnection::EActionType::Stop:
						{
							node->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szStopValue);
							break;
						}
					case CEventConnection::EActionType::Pause:
						{
							node->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szPauseValue);
							break;
						}
					case CEventConnection::EActionType::Resume:
						{
							node->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szResumeValue);
							break;
						}
					default:
						{
							break;
						}
					}

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != EItemFlags::None)
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}

				break;
			}
		case EAssetType::Parameter:
			{
				auto const pParameterConnection = static_cast<CParameterConnection const*>(pIConnection);

				if (pParameterConnection != nullptr)
				{
					isAdvanced = pParameterConnection->IsAdvanced();

					node = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					node->setAttr(CryAudio::g_szNameAttribute, pItem->GetName().c_str());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, path.c_str());
					}

					if (isAdvanced)
					{
						if (pParameterConnection->GetMultiplier() != CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier)
						{
							node->setAttr(CryAudio::Impl::SDL_mixer::g_szMutiplierAttribute, pParameterConnection->GetMultiplier());
						}

						if (pParameterConnection->GetShift() != CryAudio::Impl::SDL_mixer::g_defaultParamShift)
						{
							node->setAttr(CryAudio::Impl::SDL_mixer::g_szShiftAttribute, pParameterConnection->GetShift());
						}
					}

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != EItemFlags::None)
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}

				break;
			}
		case EAssetType::State:
			{
				auto const pStateConnection = static_cast<CStateConnection const*>(pIConnection);

				if (pStateConnection != nullptr)
				{
					node = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					node->setAttr(CryAudio::g_szNameAttribute, pItem->GetName().c_str());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, path.c_str());
					}

					node->setAttr(CryAudio::Impl::SDL_mixer::g_szValueAttribute, pStateConnection->GetValue());

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != EItemFlags::None)
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}

				break;
			}
		case EAssetType::Preload:
			{
				auto const pPreloadConnection = static_cast<CPreloadConnection const*>(pIConnection);

				if (pPreloadConnection != nullptr)
				{
					node = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					node->setAttr(CryAudio::g_szNameAttribute, pItem->GetName().c_str());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, path.c_str());
					}

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != EItemFlags::None)
					{
						node->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}

				break;
			}
		default:
			{
				break;
			}
		}

		CountConnections(assetType, contextId, isAdvanced);
	}

	return node;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::SetDataNode(char const* const szTag, CryAudio::ContextId const contextId)
{
	XmlNodeRef node;

	if (g_connections.find(contextId) != g_connections.end())
	{
		node = GetISystem()->CreateXmlNode(szTag);

		if (g_connections[contextId].events > 0)
		{
			node->setAttr(CryAudio::Impl::SDL_mixer::g_szEventsAttribute, g_connections[contextId].events);
		}

		if (g_connections[contextId].parameters > 0)
		{
			node->setAttr(CryAudio::Impl::SDL_mixer::g_szParametersAttribute, g_connections[contextId].parameters);
		}

		if (g_connections[contextId].parametersAdvanced > 0)
		{
			node->setAttr(CryAudio::Impl::SDL_mixer::g_szParametersAdvancedAttribute, g_connections[contextId].parametersAdvanced);
		}

		if (g_connections[contextId].switchStates > 0)
		{
			node->setAttr(CryAudio::Impl::SDL_mixer::g_szSwitchStatesAttribute, g_connections[contextId].switchStates);
		}

		if (g_connections[contextId].files > 0)
		{
			node->setAttr(CryAudio::Impl::SDL_mixer::g_szFilesAttribute, g_connections[contextId].files);
		}
	}

	return node;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeWriteLibrary()
{
	g_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterWriteLibrary()
{
	g_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::EnableConnection(IConnection const* const pIConnection)
{
	auto const pItem = static_cast<CItem*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		++m_connectionsByID[pItem->GetId()];
		pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsConnected);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DisableConnection(IConnection const* const pIConnection)
{
	auto const pItem = static_cast<CItem*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pItem->GetId()] - 1;

		if (connectionCount < 1)
		{
			CRY_ASSERT_MESSAGE(connectionCount >= 0, "Connection count is < 0 during %s", __FUNCTION__);
			connectionCount = 0;
			pItem->SetFlags(pItem->GetFlags() & ~EItemFlags::IsConnected);
		}

		m_connectionsByID[pItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructConnection(IConnection const* const pIConnection)
{
	delete pIConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeReload()
{
	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->OnBeforeReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterReload()
{
	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->OnAfterReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnSelectConnectedItem(ControlId const id) const
{
	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->OnSelectConnectedItem(id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterOpened()
{
	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->OnFileImporterOpened();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterClosed()
{
	if (g_pDataPanel != nullptr)
	{
		g_pDataPanel->OnFileImporterClosed();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::CanDropExternalData(QMimeData const* const pData) const
{
	bool hasValidData = false;
	CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

	if (pDragDropData->HasFilePaths())
	{
		QStringList const allFiles = pDragDropData->GetFilePaths();

		for (auto const& filePath : allFiles)
		{
			QFileInfo const fileInfo(filePath);
			bool isSupportedType = false;

			for (auto const& pair : CryAudio::Impl::SDL_mixer::g_supportedExtensions)
			{
				if (fileInfo.suffix().compare(pair.first, Qt::CaseInsensitive) == 0)
				{
					isSupportedType = true;
					break;
				}
			}

			if (fileInfo.isFile() && isSupportedType)
			{
				hasValidData = true;
				break;
			}
		}

		if (!hasValidData)
		{
			for (auto const& filePath : allFiles)
			{
				QDir const folder(filePath);

				if (HasDirValidData(folder))
				{
					hasValidData = true;
					break;
				}
			}
		}
	}

	return hasValidData;
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::DropExternalData(QMimeData const* const pData, FileImportInfos& fileImportInfos) const
{
	CRY_ASSERT_MESSAGE(fileImportInfos.empty(), "Passed container must be empty during %s", __FUNCTION__);

	if (CanDropExternalData(pData))
	{
		CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

		if (pDragDropData->HasFilePaths())
		{
			QStringList const allFiles = pDragDropData->GetFilePaths();

			for (auto const& filePath : allFiles)
			{
				QFileInfo const fileInfo(filePath);

				if (fileInfo.isFile())
				{
					bool isSupportedType = false;

					for (auto const& pair : CryAudio::Impl::SDL_mixer::g_supportedExtensions)
					{
						if (fileInfo.suffix().compare(pair.first, Qt::CaseInsensitive) == 0)
						{
							isSupportedType = true;
							break;
						}
					}

					fileImportInfos.emplace_back(fileInfo, isSupportedType);
				}
				else
				{
					QDir const folder(filePath);
					GetFilesFromDir(folder, "", fileImportInfos);
				}
			}
		}
	}

	return !fileImportInfos.empty();
}

//////////////////////////////////////////////////////////////////////////
ControlId CImpl::GenerateItemId(QString const& name, QString const& path, bool const isLocalized)
{
	return Utils::GetId(EItemType::Event, QtUtil::ToString(name), QtUtil::ToString(path), isLocalized);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Clear()
{
	for (auto const& itemPair : m_itemCache)
	{
		delete itemPair.second;
	}

	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetImplInfo(SImplInfo& implInfo)
{
	SetLocalizedAssetsPath();

	cry_strcpy(implInfo.name, m_implName.c_str());
	cry_strcpy(implInfo.folderName, CryAudio::Impl::SDL_mixer::g_szImplFolderName, strlen(CryAudio::Impl::SDL_mixer::g_szImplFolderName));
	cry_strcpy(implInfo.projectPath, m_assetAndProjectPath.c_str());
	cry_strcpy(implInfo.assetsPath, m_assetAndProjectPath.c_str());
	cry_strcpy(implInfo.localizedAssetsPath, m_localizedAssetsPath.c_str());

	implInfo.flags = (
		EImplInfoFlags::SupportsFileImport |
		EImplInfoFlags::SupportsTriggers |
		EImplInfoFlags::SupportsParameters |
		EImplInfoFlags::SupportsSwitches |
		EImplInfoFlags::SupportsStates |
		EImplInfoFlags::SupportsPreloads);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLocalizedAssetsPath()
{
	if (ICVar const* const pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		char const* const szLanguage = pCVar->GetString();

		if (szLanguage != nullptr)
		{
			m_localizedAssetsPath = PathUtil::GetLocalizationFolder().c_str();
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += szLanguage;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CRY_AUDIO_DATA_ROOT;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::Impl::SDL_mixer::g_szImplFolderName;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::g_szAssetsFolderName;
		}
	}
}
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
