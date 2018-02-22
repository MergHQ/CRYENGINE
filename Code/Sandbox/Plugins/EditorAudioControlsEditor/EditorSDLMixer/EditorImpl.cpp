// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ImplConnections.h"
#include "ProjectLoader.h"

#include <CryAudioImplSDLMixer/GlobalData.h>
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
string GetPath(CImplItem const* const pImplItem)
{
	string path;
	IImplItem const* pParent = pImplItem->GetParent();

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
CImplSettings::CImplSettings()
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
				auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(connection.first));

				if (pImplItem != nullptr)
				{
					pImplItem->SetConnected(true);
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
IImplItem* CEditorImpl::GetImplItem(CID const id) const
{
	IImplItem* pImplItem = nullptr;

	if (id >= 0)
	{
		pImplItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(IImplItem const* const pImplItem) const
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";
	auto const type = static_cast<EImplItemType>(pImplItem->GetType());

	switch (type)
	{
	case EImplItemType::Event:
		szIconPath = "icons:audio/sdlmixer/event.ico";
		break;
	case EImplItemType::Folder:
		szIconPath = "icons:General/Folder.ico";
		break;
	default:
		szIconPath = "icons:Dialogs/dialog-error.ico";
		break;
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
bool CEditorImpl::IsSystemTypeSupported(ESystemItemType const systemType) const
{
	bool isSupported = false;

	switch (systemType)
	{
	case ESystemItemType::Trigger:
	case ESystemItemType::Parameter:
	case ESystemItemType::Switch:
	case ESystemItemType::State:
	case ESystemItemType::Folder:
	case ESystemItemType::Library:
		isSupported = true;
		break;
	default:
		isSupported = false;
		break;
	}

	return isSupported;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorImpl::IsTypeCompatible(ESystemItemType const systemType, IImplItem const* const pImplItem) const
{
	bool isCompatible = false;
	auto const implType = static_cast<EImplItemType>(pImplItem->GetType());

	switch (systemType)
	{
	case ESystemItemType::Trigger:
	case ESystemItemType::Parameter:
	case ESystemItemType::State:
		isCompatible = (implType == EImplItemType::Event);
		break;
	default:
		isCompatible = false;
		break;
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(IImplItem const* const pImplItem) const
{
	ESystemItemType systemType = ESystemItemType::Invalid;
	auto const implType = static_cast<EImplItemType>(pImplItem->GetType());

	switch (implType)
	{
	case EImplItemType::Event:
		systemType = ESystemItemType::Trigger;
		break;
	default:
		systemType = ESystemItemType::Invalid;
		break;
	}

	return systemType;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, IImplItem* const pImplItem)
{
	ConnectionPtr pConnection = nullptr;

	if (pImplItem != nullptr)
	{
		switch (controlType)
		{
		case ESystemItemType::Parameter:
			pConnection = std::make_shared<CParameterConnection>(pImplItem->GetId());
			break;
		case ESystemItemType::State:
			pConnection = std::make_shared<CStateConnection>(pImplItem->GetId());
			break;
		default:
			pConnection = std::make_shared<CEventConnection>(pImplItem->GetId());
			break;
		}
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType)
{
	ConnectionPtr pConnectionPtr = nullptr;

	if (pNode != nullptr)
	{
		string const nodeTag = pNode->getTag();

		if ((nodeTag == CryAudio::s_szEventTag) || (nodeTag == CryAudio::Impl::SDL_mixer::s_szFileTag) || (nodeTag == "SDLMixerEvent") || (nodeTag == "SDLMixerSample"))
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
			CID id;

			if (path.empty())
			{
				id = GetId(name);
			}
			else
			{
				id = GetId(path + "/" + name);
			}

			auto pImplItem = static_cast<CImplItem*>(GetImplItem(id));

			if (pImplItem == nullptr)
			{
				pImplItem = new CImplItem(name, id, static_cast<ItemType>(EImplItemType::Event), EImplItemFlags::IsPlaceHolder);
				m_itemCache[id] = pImplItem;
			}

			if (pImplItem != nullptr)
			{
				switch (controlType)
				{
				case ESystemItemType::Trigger:
					{
						auto const pEventConnection = std::make_shared<CEventConnection>(pImplItem->GetId());
						string connectionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (connectionType.IsEmpty() && pNode->haveAttr("event_type"))
						{
							connectionType = pNode->getAttr("event_type");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY
						if (connectionType == CryAudio::Impl::SDL_mixer::s_szStopValue)
						{
							pEventConnection->SetType(EEventType::Stop);
						}
						else if (connectionType == CryAudio::Impl::SDL_mixer::s_szPauseValue)
						{
							pEventConnection->SetType(EEventType::Pause);
						}
						else if (connectionType == CryAudio::Impl::SDL_mixer::s_szResumeValue)
						{
							pEventConnection->SetType(EEventType::Resume);
						}
						else
						{
							pEventConnection->SetType(EEventType::Start);

							float fadeInTime = pEventConnection->GetFadeInTime();
							pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, fadeInTime);
							pEventConnection->SetFadeInTime(fadeInTime);

							float fadeOutTime = pEventConnection->GetFadeOutTime();
							pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, fadeOutTime);
							pEventConnection->SetFadeOutTime(fadeOutTime);
						}

						string const enablePanning = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute);
						pEventConnection->SetPanningEnabled(enablePanning == CryAudio::Impl::SDL_mixer::s_szTrueValue ? true : false);

						string enableDistAttenuation = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (enableDistAttenuation.IsEmpty() && pNode->haveAttr("enable_distance_attenuation"))
						{
							enableDistAttenuation = pNode->getAttr("enable_distance_attenuation");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY
						pEventConnection->SetAttenuationEnabled(enableDistAttenuation == CryAudio::Impl::SDL_mixer::s_szTrueValue ? true : false);

						float minAttenuation = pEventConnection->GetMinAttenuation();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, minAttenuation);
						pEventConnection->SetMinAttenuation(minAttenuation);

						float maxAttenuation = pEventConnection->GetMaxAttenuation();
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
				case ESystemItemType::Parameter:
					{
						auto const pParameterConnection = std::make_shared<CParameterConnection>(pImplItem->GetId());

						float mult = pParameterConnection->GetMultiplier();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szMutiplierAttribute, mult);
						pParameterConnection->SetMultiplier(mult);

						float shift = pParameterConnection->GetShift();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szShiftAttribute, shift);
						pParameterConnection->SetShift(shift);

						pConnectionPtr = pParameterConnection;
					}
					break;
				case ESystemItemType::State:
					{
						auto const pStateConnection = std::make_shared<CStateConnection>(pImplItem->GetId());

						float value = pStateConnection->GetValue();
						pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szValueAttribute, value);
						pStateConnection->SetValue(value);

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
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	XmlNodeRef pNode = nullptr;

	auto const pImplItem = static_cast<CImplItem const* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		switch (controlType)
		{
		case ESystemItemType::Trigger:
			{
				std::shared_ptr<CEventConnection const> const pEventConnection = std::static_pointer_cast<CEventConnection const>(pConnection);

				if (pEventConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

					string const path = GetPath(pImplItem);
					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

					EEventType const type = pEventConnection->GetType();

					switch (type)
					{
					case EEventType::Start:
						{
							pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStartValue);
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute, pEventConnection->IsPanningEnabled() ? CryAudio::Impl::SDL_mixer::s_szTrueValue : CryAudio::Impl::SDL_mixer::s_szFalseValue);
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute, pEventConnection->IsAttenuationEnabled() ? CryAudio::Impl::SDL_mixer::s_szTrueValue : CryAudio::Impl::SDL_mixer::s_szFalseValue);
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, pEventConnection->GetMinAttenuation());
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMaxDistanceAttribute, pEventConnection->GetMaxAttenuation());
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szVolumeAttribute, pEventConnection->GetVolume());
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, pEventConnection->GetFadeInTime());
							pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, pEventConnection->GetFadeOutTime());

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
					case EEventType::Stop:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStopValue);
						break;
					case EEventType::Pause:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szPauseValue);
						break;
					case EEventType::Resume:
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szResumeValue);
						break;
					}
				}
			}
			break;
		case ESystemItemType::Parameter:
			{
				std::shared_ptr<CParameterConnection const> const pParameterConnection = std::static_pointer_cast<CParameterConnection const>(pConnection);

				if (pParameterConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

					string const path = GetPath(pImplItem);
					pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

					if (pParameterConnection->GetMultiplier() != 1.0f)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szMutiplierAttribute, pParameterConnection->GetMultiplier());
					}

					if (pParameterConnection->GetShift() != 0.0f)
					{
						pNode->setAttr(CryAudio::Impl::SDL_mixer::s_szShiftAttribute, pParameterConnection->GetShift());
					}
				}
			}
			break;
		case ESystemItemType::State:
			{
				std::shared_ptr<CStateConnection const> const pStateConnection = std::static_pointer_cast<CStateConnection const>(pConnection);

				if (pStateConnection != nullptr)
				{
					pNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
					pNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

					string const path = GetPath(pImplItem);
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
	auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		++m_connectionsByID[pImplItem->GetId()];
		pImplItem->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pImplItem->GetId()] - 1;

		if (connectionCount <= 0)
		{
			connectionCount = 0;
			pImplItem->SetConnected(false);
		}

		m_connectionsByID[pImplItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	// Delete all the items
	for (auto const& itemPair : m_itemCache)
	{
		CImplItem const* const pImplItem = itemPair.second;

		if (pImplItem != nullptr)
		{
			delete pImplItem;
		}
	}

	m_itemCache.clear();

	// Clean up the root item
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::CreateItemCache(CImplItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->GetNumChildren();

		for (size_t i = 0; i < count; ++i)
		{
			auto const pChild = static_cast<CImplItem* const>(pParent->GetChildAt(i));

			if (pChild != nullptr)
			{
				m_itemCache[pChild->GetId()] = pChild;
				CreateItemCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GetId(string const& name) const
{
	return CryAudio::StringToId(name);
}
} // namespace SDLMixer
} // namespace ACE
