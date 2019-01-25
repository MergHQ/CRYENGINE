// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Impl.h"

#include "Common.h"
#include "BaseConnection.h"
#include "EventConnection.h"
#include "ParameterConnection.h"
#include "StateConnection.h"
#include "ProjectLoader.h"
#include "DataPanel.h"
#include "Utils.h"

#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySystem/XML/IXml.h>

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

//////////////////////////////////////////////////////////////////////////
void CountConnections(EAssetType const assetType)
{
	switch (assetType)
	{
	case EAssetType::Trigger:
		{
			++g_connections.events;
			break;
		}
	case EAssetType::Parameter:
		{
			++g_connections.parameters;
			break;
		}
	case EAssetType::State:
		{
			++g_connections.switchStates;
			break;
		}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_pDataPanel(nullptr)
	, m_assetAndProjectPath(AUDIO_SYSTEM_DATA_ROOT "/" +
	                        string(CryAudio::Impl::SDL_mixer::g_szImplFolderName) +
	                        "/"
	                        + string(CryAudio::s_szAssetsFolderName))
	, m_localizedAssetsPath(m_assetAndProjectPath)
{
}

//////////////////////////////////////////////////////////////////////////
CImpl::~CImpl()
{
	Clear();
	DestroyDataPanel();

	CItem::FreeMemoryPool();
	CEventConnection::FreeMemoryPool();
	CParameterConnection::FreeMemoryPool();
	CStateConnection::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Initialize(SImplInfo& implInfo, Platforms const& platforms)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer ACE Item Pool");
	CItem::CreateAllocator(g_itemPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer ACE Event Connection Pool");
	CEventConnection::CreateAllocator(g_eventConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer ACE Parameter Connection Pool");
	CParameterConnection::CreateAllocator(g_parameterConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer ACE State Connection Pool");
	CStateConnection::CreateAllocator(g_stateConnectionPoolSize);

	CryAudio::SImplInfo systemImplInfo;
	gEnv->pAudioSystem->GetImplInfo(systemImplInfo);
	m_implName = systemImplInfo.name.c_str();

	SetImplInfo(implInfo);
}

//////////////////////////////////////////////////////////////////////////
QWidget* CImpl::CreateDataPanel()
{
	m_pDataPanel = new CDataPanel(*this);
	return m_pDataPanel;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestroyDataPanel()
{
	if (m_pDataPanel != nullptr)
	{
		delete m_pDataPanel;
		m_pDataPanel = nullptr;
	}
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

	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->Reset();
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
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Trigger:   // Intentional fall-through.
		case EAssetType::Parameter: // Intentional fall-through.
		case EAssetType::State:
			isCompatible = (pItem->GetType() == EItemType::Event);
			break;
		default:
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
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (implType)
		{
		case EItemType::Event:
			assetType = EAssetType::Trigger;
			break;
		default:
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

	if (pIItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Parameter:
			pIConnection = static_cast<IConnection*>(new CParameterConnection(pIItem->GetId()));
			break;
		case EAssetType::State:
			pIConnection = static_cast<IConnection*>(new CStateConnection(pIItem->GetId()));
			break;
		default:
			pIConnection = static_cast<IConnection*>(new CEventConnection(pIItem->GetId()));
			break;
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType)
{
	IConnection* pIConnection = nullptr;

	if (pNode != nullptr)
	{
		char const* const szTag = pNode->getTag();

		if ((_stricmp(szTag, CryAudio::Impl::SDL_mixer::g_szEventTag) == 0) ||
		    (_stricmp(szTag, CryAudio::Impl::SDL_mixer::g_szSampleTag) == 0) ||
		    (_stricmp(szTag, "SDLMixerEvent") == 0) || // Backwards compatibility.
		    (_stricmp(szTag, "SDLMixerSample") == 0))  // Backwards compatibility.
		{
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string path = pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute);
			// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && pNode->haveAttr("sdl_name"))
			{
				name = pNode->getAttr("sdl_name");
			}

			if (path.IsEmpty() && pNode->haveAttr("sdl_path"))
			{
				path = pNode->getAttr("sdl_path");
			}
#endif      // USE_BACKWARDS_COMPATIBILITY
			string const localizedAttribute = pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute);
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
						string actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (actionType.IsEmpty() && pNode->haveAttr("event_type"))
						{
							actionType = pNode->getAttr("event_type");
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
							pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szFadeInTimeAttribute, fadeInTime);
							pEventConnection->SetFadeInTime(fadeInTime);

							float fadeOutTime = CryAudio::Impl::SDL_mixer::g_defaultFadeOutTime;
							pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szFadeOutTimeAttribute, fadeOutTime);
							pEventConnection->SetFadeOutTime(fadeOutTime);
						}

						string const enablePanning = pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szPanningEnabledAttribute);
						pEventConnection->SetPanningEnabled(enablePanning.compareNoCase(CryAudio::Impl::SDL_mixer::g_szTrueValue) == 0);

						string enableDistAttenuation = pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationEnabledAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (enableDistAttenuation.IsEmpty() && pNode->haveAttr("enable_distance_attenuation"))
						{
							enableDistAttenuation = pNode->getAttr("enable_distance_attenuation");
						}
#endif            // USE_BACKWARDS_COMPATIBILITY
						pEventConnection->SetAttenuationEnabled(enableDistAttenuation.compareNoCase(CryAudio::Impl::SDL_mixer::g_szTrueValue) == 0);

						float minAttenuation = CryAudio::Impl::SDL_mixer::g_defaultMinAttenuationDist;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMinDistanceAttribute, minAttenuation);
						pEventConnection->SetMinAttenuation(minAttenuation);

						float maxAttenuation = CryAudio::Impl::SDL_mixer::g_defaultMaxAttenuationDist;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMaxDistanceAttribute, maxAttenuation);
						pEventConnection->SetMaxAttenuation(maxAttenuation);

						float volume = pEventConnection->GetVolume();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szVolumeAttribute, volume);
						pEventConnection->SetVolume(volume);

						int loopCount = pEventConnection->GetLoopCount();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, loopCount);
						loopCount = std::max(0, loopCount); // Delete this when backwards compatibility gets removed and use uint32 directly.
						pEventConnection->SetLoopCount(static_cast<uint32>(loopCount));

						if (pEventConnection->GetLoopCount() == 0)
						{
							pEventConnection->SetInfiniteLoop(true);
						}

						pIConnection = static_cast<IConnection*>(pEventConnection);
					}
					break;
				case EAssetType::Parameter:
					{
						float mult = CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szMutiplierAttribute, mult);

						float shift = CryAudio::Impl::SDL_mixer::g_defaultParamShift;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szShiftAttribute, shift);

						pIConnection = static_cast<IConnection*>(new CParameterConnection(pItem->GetId(), mult, shift));
					}
					break;
				case EAssetType::State:
					{
						float value = CryAudio::Impl::SDL_mixer::g_defaultStateValue;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::g_szValueAttribute, value);

						pIConnection = static_cast<IConnection*>(new CStateConnection(pItem->GetId(), value));
					}
					break;
				}
			}
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::CreateXMLNodeFromConnection(IConnection const* const pIConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pItem = static_cast<CItem const*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Trigger:
			{
				auto const pEventConnection = static_cast<CEventConnection const*>(pIConnection);

				if (pEventConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, pItem->GetPath());
					}

					CEventConnection::EActionType const actionType = pEventConnection->GetActionType();

					switch (actionType)
					{
					case CEventConnection::EActionType::Start:
						{
							pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szStartValue);
							pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szVolumeAttribute, pEventConnection->GetVolume());

							if (pEventConnection->IsPanningEnabled())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szPanningEnabledAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
							}

							if (pEventConnection->IsAttenuationEnabled())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationEnabledAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMinDistanceAttribute, pEventConnection->GetMinAttenuation());
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szAttenuationMaxDistanceAttribute, pEventConnection->GetMaxAttenuation());
							}

							if (pEventConnection->GetFadeInTime() != CryAudio::Impl::SDL_mixer::g_defaultFadeInTime)
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szFadeInTimeAttribute, pEventConnection->GetFadeInTime());
							}

							if (pEventConnection->GetFadeOutTime() != CryAudio::Impl::SDL_mixer::g_defaultFadeOutTime)
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szFadeOutTimeAttribute, pEventConnection->GetFadeOutTime());
							}

							if (pEventConnection->IsInfiniteLoop())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, 0);
							}
							else
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szLoopCountAttribute, pEventConnection->GetLoopCount());
							}
						}
						break;
					case CEventConnection::EActionType::Stop:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szStopValue);
						break;
					case CEventConnection::EActionType::Pause:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szPauseValue);
						break;
					case CEventConnection::EActionType::Resume:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::g_szResumeValue);
						break;
					}

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}
			}
			break;
		case EAssetType::Parameter:
			{
				auto const pParameterConnection = static_cast<CParameterConnection const*>(pIConnection);

				if (pParameterConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, pItem->GetPath());
					}

					if (pParameterConnection->GetMultiplier() != CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szMutiplierAttribute, pParameterConnection->GetMultiplier());
					}

					if (pParameterConnection->GetShift() != CryAudio::Impl::SDL_mixer::g_defaultParamShift)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szShiftAttribute, pParameterConnection->GetShift());
					}

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}
			}
			break;
		case EAssetType::State:
			{
				auto const pStateConnection = static_cast<CStateConnection const*>(pIConnection);

				if (pStateConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::Impl::SDL_mixer::g_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const& path = pItem->GetPath();

					if (!path.IsEmpty())
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szPathAttribute, pItem->GetPath());
					}

					pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szValueAttribute, pStateConnection->GetValue());

					if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szLocalizedAttribute, CryAudio::Impl::SDL_mixer::g_szTrueValue);
					}
				}
			}
			break;
		}
		CountConnections(assetType);
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::SetDataNode(char const* const szTag)
{
	XmlNodeRef pNode = GetISystem()->CreateXmlNode(szTag);
	bool hasConnections = false;

	if (g_connections.events > 0)
	{
		pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szEventsAttribute, g_connections.events);
		hasConnections = true;
	}

	if (g_connections.parameters > 0)
	{
		pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szParametersAttribute, g_connections.parameters);
		hasConnections = true;
	}

	if (g_connections.switchStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::SDL_mixer::g_szSwitchStatesAttribute, g_connections.switchStates);
		hasConnections = true;
	}

	if (hasConnections)
	{
		// Reset connection count for next library.
		ZeroStruct(g_connections);
	}
	else
	{
		pNode = nullptr;
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::EnableConnection(IConnection const* const pIConnection, bool const isLoading)
{
	auto const pItem = static_cast<CItem*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		++m_connectionsByID[pItem->GetId()];
		pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsConnected);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DisableConnection(IConnection const* const pIConnection, bool const isLoading)
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
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnBeforeReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterReload()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnAfterReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnSelectConnectedItem(ControlId const id) const
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnSelectConnectedItem(id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterOpened()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnFileImporterOpened();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterClosed()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnFileImporterClosed();
	}
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

	implInfo.name = m_implName.c_str();
	implInfo.folderName = CryAudio::Impl::SDL_mixer::g_szImplFolderName;
	implInfo.projectPath = m_assetAndProjectPath.c_str();
	implInfo.assetsPath = m_assetAndProjectPath.c_str();
	implInfo.localizedAssetsPath = m_localizedAssetsPath.c_str();
	implInfo.flags = (
		EImplInfoFlags::SupportsTriggers |
		EImplInfoFlags::SupportsParameters |
		EImplInfoFlags::SupportsSwitches |
		EImplInfoFlags::SupportsStates);
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
			m_localizedAssetsPath += AUDIO_SYSTEM_DATA_ROOT;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::Impl::SDL_mixer::g_szImplFolderName;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::s_szAssetsFolderName;
		}
	}
}
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
