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
CImplSettings::CImplSettings()
	: m_assetAndProjectPath(
	  PathUtil::GetGameFolder() +
	  CRY_NATIVE_PATH_SEPSTR
	  AUDIO_SYSTEM_DATA_ROOT
	  CRY_NATIVE_PATH_SEPSTR +
	  CryAudio::Impl::SDL_mixer::s_szImplFolderName +
	  CRY_NATIVE_PATH_SEPSTR +
	  CryAudio::s_szAssetsFolderName)
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
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Event:
		szIconPath = "icons:audio/sdlmixer/event.ico";
		break;
	case EImpltemType::Folder:
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
	auto const implType = static_cast<EImpltemType>(pImplItem->GetType());

	if (systemType == ESystemItemType::Trigger)
	{
		isCompatible = (implType == EImpltemType::Event);
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(IImplItem const* const pImplItem) const
{
	ESystemItemType systemType = ESystemItemType::Invalid;
	auto const implType = static_cast<EImpltemType>(pImplItem->GetType());

	switch (implType)
	{
	case EImpltemType::Event:
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
		pConnection = std::make_shared<CConnection>(pImplItem->GetId());
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
				id = GetId(path + CRY_NATIVE_PATH_SEPSTR + name);
			}

			auto pImplItem = static_cast<CImplItem*>(GetImplItem(id));

			if (pImplItem == nullptr)
			{
				pImplItem = new CImplItem(name, id, static_cast<ItemType>(EImpltemType::Event), EImplItemFlags::IsPlaceHolder);
				m_itemCache[id] = pImplItem;
			}

			if (pImplItem != nullptr)
			{
				auto const pConnection = std::make_shared<CConnection>(pImplItem->GetId());
				string connectionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (connectionType.IsEmpty() && pNode->haveAttr("event_type"))
				{
					connectionType = pNode->getAttr("event_type");
				}
#endif      // USE_BACKWARDS_COMPATIBILITY
				if (connectionType == CryAudio::Impl::SDL_mixer::s_szStopValue)
				{
					pConnection->m_type = EConnectionType::Stop;
				}
				else if (connectionType == CryAudio::Impl::SDL_mixer::s_szPauseValue)
				{
					pConnection->m_type = EConnectionType::Pause;
				}
				else if (connectionType == CryAudio::Impl::SDL_mixer::s_szResumeValue)
				{
					pConnection->m_type = EConnectionType::Resume;
				}
				else
				{
					pConnection->m_type = EConnectionType::Start;
					pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, pConnection->m_fadeInTime);
					pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, pConnection->m_fadeOutTime);
				}

				string const enablePanning = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute);
				pConnection->m_isPanningEnabled = enablePanning == CryAudio::Impl::SDL_mixer::s_szTrueValue ? true : false;

				string enableDistAttenuation = pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (enableDistAttenuation.IsEmpty() && pNode->haveAttr("enable_distance_attenuation"))
				{
					enableDistAttenuation = pNode->getAttr("enable_distance_attenuation");
				}
#endif      // USE_BACKWARDS_COMPATIBILITY
				pConnection->m_isAttenuationEnabled = enableDistAttenuation == CryAudio::Impl::SDL_mixer::s_szTrueValue ? true : false;

				pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, pConnection->m_minAttenuation);
				pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMaxDistanceAttribute, pConnection->m_maxAttenuation);
				pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szVolumeAttribute, pConnection->m_volume);

				int loopCount = 0;
				pNode->getAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, loopCount);
				loopCount = std::max(0, loopCount);
				pConnection->m_loopCount = static_cast<uint32>(loopCount);

				if (pConnection->m_loopCount == 0)
				{
					pConnection->m_isInfiniteLoop = true;
				}

				pConnectionPtr = pConnection;
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	XmlNodeRef pConnectionNode = nullptr;

	std::shared_ptr<CConnection const> const pImplConnection = std::static_pointer_cast<CConnection const>(pConnection);
	auto const pImplItem = static_cast<CImplItem const* const>(GetImplItem(pConnection->GetID()));

	if ((pImplItem != nullptr) && (pImplConnection != nullptr) && (controlType == ESystemItemType::Trigger))
	{
		pConnectionNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
		pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

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
					path = parentName + CRY_NATIVE_PATH_SEPSTR + path;
				}
			}

			pParent = pParent->GetParent();
		}

		pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPathAttribute, path);

		EConnectionType const type = pImplConnection->m_type;

		switch (type)
		{
		case EConnectionType::Start:
			{
				pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStartValue);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szPanningEnabledAttribute, pImplConnection->m_isPanningEnabled ? CryAudio::Impl::SDL_mixer::s_szTrueValue : CryAudio::Impl::SDL_mixer::s_szFalseValue);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationEnabledAttribute, pImplConnection->m_isAttenuationEnabled ? CryAudio::Impl::SDL_mixer::s_szTrueValue : CryAudio::Impl::SDL_mixer::s_szFalseValue);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMinDistanceAttribute, pImplConnection->m_minAttenuation);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szAttenuationMaxDistanceAttribute, pImplConnection->m_maxAttenuation);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szVolumeAttribute, pImplConnection->m_volume);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeInTimeAttribute, pImplConnection->m_fadeInTime);
				pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szFadeOutTimeAttribute, pImplConnection->m_fadeOutTime);

				if (pImplConnection->m_isInfiniteLoop)
				{
					pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, 0);
				}
				else
				{
					pConnectionNode->setAttr(CryAudio::Impl::SDL_mixer::s_szLoopCountAttribute, pImplConnection->m_loopCount);
				}
			}
			break;
		case EConnectionType::Stop:
			pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szStopValue);
			break;
		case EConnectionType::Pause:
			pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szPauseValue);
			break;
		case EConnectionType::Resume:
			pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::SDL_mixer::s_szResumeValue);
			break;
		}
	}

	return pConnectionNode;
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
