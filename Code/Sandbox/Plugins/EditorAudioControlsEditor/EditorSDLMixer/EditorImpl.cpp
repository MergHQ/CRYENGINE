// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "Connection.h"
#include "ProjectLoader.h"

#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/ClassFactory.h>

namespace ACE
{
namespace SDLMixer
{
//////////////////////////////////////////////////////////////////////////
string GetPath(CItem const* const pItem)
{
	string path;
	IImplItem const* pParent = pItem->GetParent();

	while (pParent != nullptr)
	{
		string const parentName = pParent->GetName();

		if (!parentName.empty())
		{
			if (path.empty())
			{
				path = parentName;
			}
			else
			{
				path = parentName + "/" + path;
			}
		}

		pParent = pParent->GetParent();
	}

	return path;
}

//////////////////////////////////////////////////////////////////////////
CSettings::CSettings()
	: m_assetAndProjectPath(AUDIO_SYSTEM_DATA_ROOT "/" + string(CryAudio::Impl::SDL_mixer::s_szImplFolderName) + "/" + string(CryAudio::s_szAssetsFolderName))
{}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
{
	gEnv->pAudioSystem->GetImplInfo(m_implInfo);
	m_implName = m_implInfo.name.c_str();
	m_implFolderName = CryAudio::Impl::SDL_mixer::s_szImplFolderName;
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::~CEditorImpl()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Reload(bool const preserveConnectionStatus)
{
	Clear();

	CProjectLoader(GetSettings()->GetProjectPath(), m_rootItem);

	CreateItemCache(&m_rootItem);

	if (preserveConnectionStatus)
	{
		for (auto const& connection : m_connectionsByID)
		{
			if (connection.second > 0)
			{
				auto const pItem = static_cast<CItem* const>(GetItem(connection.first));

				if (pItem != nullptr)
				{
					pItem->SetConnected(true);
				}
			}
		}
	}
	else
	{
		m_connectionsByID.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
IImplItem* CEditorImpl::GetItem(ControlId const id) const
{
	IImplItem* pIImplItem = nullptr;

	if (id >= 0)
	{
		pIImplItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pIImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(IImplItem const* const pIImplItem) const
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		EItemType const type = pItem->GetType();

		switch (type)
		{
		case EItemType::Event:
			szIconPath = "icons:audio/sdlmixer/event.ico";
			break;
		case EItemType::Folder:
			szIconPath = "icons:General/Folder.ico";
			break;
		default:
			szIconPath = "icons:Dialogs/dialog-error.ico";
			break;
		}
	}

	return szIconPath;
}

//////////////////////////////////////////////////////////////////////////
string const& CEditorImpl::GetName() const
{
	return m_implName;
}

//////////////////////////////////////////////////////////////////////////
string const& CEditorImpl::GetFolderName() const
{
	return m_implFolderName;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorImpl::IsSystemTypeSupported(EAssetType const assetType) const
{
	bool isSupported = false;

	switch (assetType)
	{
	case EAssetType::Trigger:
	case EAssetType::Parameter:
	case EAssetType::Switch:
	case EAssetType::State:
	case EAssetType::Folder:
	case EAssetType::Library:
		isSupported = true;
		break;
	default:
		isSupported = false;
		break;
	}

	return isSupported;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorImpl::IsTypeCompatible(EAssetType const assetType, IImplItem const* const pIImplItem) const
{
	bool isCompatible = false;
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Trigger:
		case EAssetType::Parameter:
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
EAssetType CEditorImpl::ImplTypeToAssetType(IImplItem const* const pIImplItem) const
{
	EAssetType assetType = EAssetType::None;
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

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
ConnectionPtr CEditorImpl::CreateConnectionToControl(EAssetType const assetType, IImplItem const* const pIImplItem)
{
	ConnectionPtr pConnection = nullptr;

	if (pIImplItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Parameter:
			pConnection = std::make_shared<CParameterConnection>(pIImplItem->GetId());
			break;
		case EAssetType::State:
			pConnection = std::make_shared<CStateConnection>(pIImplItem->GetId());
			break;
		default:
			pConnection = std::make_shared<CEventConnection>(pIImplItem->GetId());
			break;
		}
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType)
{
	ConnectionPtr pConnectionPtr = nullptr;

	if (pNode != nullptr)
	{
		char const* const szTag = pNode->getTag();

		if ((_stricmp(szTag, CryAudio::s_szEventTag) == 0) ||
		    (_stricmp(szTag, CryAudio::Impl::SDL_mixer::s_szFileTag) == 0) ||
		    (_stricmp(szTag, "SDLMixerEvent") == 0) || // Backwards compatibility.
		    (_stricmp(szTag, "SDLMixerSample") == 0))  // Backwards compatibility.
		{
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string path = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute);
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
#endif    // USE_BACKWARDS_COMPATIBILITY
			ControlId id;

			if (path.empty())
			{
				id = GetId(name);
			}
			else
			{
				id = GetId(path + "/" + name);
			}

			auto pItem = static_cast<CItem*>(GetItem(id));

			if (pItem == nullptr)
			{
				pItem = new CItem(name, id, EItemType::Event, EItemFlags::IsPlaceHolder);
				m_itemCache[id] = pItem;
			}

			if (pItem != nullptr)
			{
				switch (assetType)
				{
				case EAssetType::Trigger:
					{
						auto const pEventConnection = std::make_shared<CEventConnection>(pItem->GetId());
						string actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (actionType.IsEmpty() && pNode->haveAttr("event_type"))
						{
							actionType = pNode->getAttr("event_type");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY
						if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::s_szStopValue) == 0)
						{
							pEventConnection->SetActionType(EEventActionType::Stop);
						}
						else if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::s_szPauseValue) == 0)
						{
							pEventConnection->SetActionType(EEventActionType::Pause);
						}
						else if (actionType.compareNoCase(CryAudio::Impl::SDL_mixer::s_szResumeValue) == 0)
						{
							pEventConnection->SetActionType(EEventActionType::Resume);
						}
						else
						{
							pEventConnection->SetActionType(EEventActionType::Start);

							float fadeInTime = CryAudio::Impl::SDL_mixer::s_defaultFadeInTime;
							pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, fadeInTime);
							pEventConnection->SetFadeInTime(fadeInTime);

							float fadeOutTime = CryAudio::Impl::SDL_mixer::s_defaultFadeOutTime;
							pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, fadeOutTime);
							pEventConnection->SetFadeOutTime(fadeOutTime);
						}

						string const enablePanning = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute);
						pEventConnection->SetPanningEnabled(enablePanning.compareNoCase(CryAudio::Impl::SDL_mixer::s_szTrueValue) == 0);

						string enableDistAttenuation = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (enableDistAttenuation.IsEmpty() && pNode->haveAttr("enable_distance_attenuation"))
						{
							enableDistAttenuation = pNode->getAttr("enable_distance_attenuation");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY
						pEventConnection->SetAttenuationEnabled(enableDistAttenuation.compareNoCase(CryAudio::Impl::SDL_mixer::s_szTrueValue) == 0);

						float minAttenuation = CryAudio::Impl::SDL_mixer::s_defaultMinAttenuationDist;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, minAttenuation);
						pEventConnection->SetMinAttenuation(minAttenuation);

						float maxAttenuation = CryAudio::Impl::SDL_mixer::s_defaultMaxAttenuationDist;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMaxDistanceAttribute, maxAttenuation);
						pEventConnection->SetMaxAttenuation(maxAttenuation);

						float volume = pEventConnection->GetVolume();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szVolumeAttribute, volume);
						pEventConnection->SetVolume(volume);

						int loopCount = pEventConnection->GetLoopCount();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, loopCount);
						loopCount = std::max(0, loopCount); // Delete this when backwards compatibility gets removed and use uint32 directly.
						pEventConnection->SetLoopCount(static_cast<uint32>(loopCount));

						if (pEventConnection->GetLoopCount() == 0)
						{
							pEventConnection->SetInfiniteLoop(true);
						}

						pConnectionPtr = pEventConnection;
					}
					break;
				case EAssetType::Parameter:
					{
						float mult = CryAudio::Impl::SDL_mixer::s_defaultParamMultiplier;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szMutiplierAttribute, mult);

						float shift = CryAudio::Impl::SDL_mixer::s_defaultParamShift;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szShiftAttribute, shift);

						auto const pParameterConnection = std::make_shared<CParameterConnection>(pItem->GetId(), mult, shift);
						pConnectionPtr = pParameterConnection;
					}
					break;
				case EAssetType::State:
					{
						float value = CryAudio::Impl::SDL_mixer::s_defaultStateValue;
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szValueAttribute, value);

						auto const pStateConnection = std::make_shared<CStateConnection>(pItem->GetId(), value);
						pConnectionPtr = pStateConnection;
					}
					break;
				}
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pItem = static_cast<CItem const* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		switch (assetType)
		{
		case EAssetType::Trigger:
			{
				std::shared_ptr<CEventConnection const> const pEventConnection = std::static_pointer_cast<CEventConnection const>(pConnection);

				if (pEventConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const path = GetPath(pItem);
					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

					EEventActionType const type = pEventConnection->GetActionType();

					switch (type)
					{
					case EEventActionType::Start:
						{
							pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStartValue);
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szVolumeAttribute, pEventConnection->GetVolume());

							if (pEventConnection->IsPanningEnabled())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute, CryAudio::Impl::SDL_mixer::s_szTrueValue);
							}

							if (pEventConnection->IsAttenuationEnabled())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute, CryAudio::Impl::SDL_mixer::s_szTrueValue);
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, pEventConnection->GetMinAttenuation());
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMaxDistanceAttribute, pEventConnection->GetMaxAttenuation());
							}

							if (pEventConnection->GetFadeInTime() != CryAudio::Impl::SDL_mixer::s_defaultFadeInTime)
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, pEventConnection->GetFadeInTime());
							}

							if (pEventConnection->GetFadeOutTime() != CryAudio::Impl::SDL_mixer::s_defaultFadeOutTime)
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, pEventConnection->GetFadeOutTime());
							}

							if (pEventConnection->IsInfiniteLoop())
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, 0);
							}
							else
							{
								pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, pEventConnection->GetLoopCount());
							}
						}
						break;
					case EEventActionType::Stop:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStopValue);
						break;
					case EEventActionType::Pause:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szPauseValue);
						break;
					case EEventActionType::Resume:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szResumeValue);
						break;
					}
				}
			}
			break;
		case EAssetType::Parameter:
			{
				std::shared_ptr<CParameterConnection const> const pParameterConnection = std::static_pointer_cast<CParameterConnection const>(pConnection);

				if (pParameterConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const path = GetPath(pItem);
					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

					if (pParameterConnection->GetMultiplier() != CryAudio::Impl::SDL_mixer::s_defaultParamMultiplier)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szMutiplierAttribute, pParameterConnection->GetMultiplier());
					}

					if (pParameterConnection->GetShift() != CryAudio::Impl::SDL_mixer::s_defaultParamShift)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szShiftAttribute, pParameterConnection->GetShift());
					}
				}
			}
			break;
		case EAssetType::State:
			{
				std::shared_ptr<CStateConnection const> const pStateConnection = std::static_pointer_cast<CStateConnection const>(pConnection);

				if (pStateConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

					string const path = GetPath(pItem);
					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szValueAttribute, pStateConnection->GetValue());
				}
			}
			break;
		}
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::EnableConnection(ConnectionPtr const pConnection)
{
	auto const pItem = static_cast<CItem* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		++m_connectionsByID[pItem->GetId()];
		pItem->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	auto const pItem = static_cast<CItem* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pItem->GetId()] - 1;

		if (connectionCount < 1)
		{
			CRY_ASSERT_MESSAGE(connectionCount >= 0, "Connection count is < 0");
			connectionCount = 0;
			pItem->SetConnected(false);
		}

		m_connectionsByID[pItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	for (auto const& itemPair : m_itemCache)
	{
		delete itemPair.second;
	}

	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::CreateItemCache(CItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->GetNumChildren();

		for (size_t i = 0; i < count; ++i)
		{
			auto const pChild = static_cast<CItem* const>(pParent->GetChildAt(i));

			if (pChild != nullptr)
			{
				m_itemCache[pChild->GetId()] = pChild;
				CreateItemCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ControlId CEditorImpl::GetId(string const& name) const
{
	return CryAudio::StringToId(name);
}
} // namespace SDLMixer
} // namespace ACE

