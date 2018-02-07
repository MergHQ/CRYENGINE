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
{
}

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

	CProjectLoader(GetSettings()->GetProjectPath(), m_rootControl);

	CreateControlCache(&m_rootControl);

	if (preserveConnectionStatus)
	{
		for (auto const& connection : m_connectionsByID)
		{
			if (connection.second > 0)
			{
				CImplItem* const pImplControl = GetControl(connection.first);

				if (pImplControl != nullptr)
				{
					pImplControl->SetConnected(true);
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
CImplItem* CEditorImpl::GetControl(CID const id) const
{
	CImplItem* pImplItem = nullptr;

	if (id >= 0)
	{
		pImplItem = stl::find_in_map(m_controlsCache, id, nullptr);
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(CImplItem const* const pImplItem) const
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
bool CEditorImpl::IsTypeCompatible(ESystemItemType const systemType, CImplItem const* const pImplItem) const
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
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
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
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem)
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

			CImplItem* pImplControl = GetControl(id);

			if (pImplControl == nullptr)
			{
				pImplControl = new CImplControl(name, id, static_cast<ItemType>(EImpltemType::Event));
				pImplControl->SetPlaceholder(true);
				m_controlsCache[id] = pImplControl;
			}

			if (pImplControl != nullptr)
			{
				auto const pConnection = std::make_shared<CConnection>(pImplControl->GetId());
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
	CImplItem const* const pImplControl = GetControl(pConnection->GetID());

	if ((pImplControl != nullptr) && (pImplConnection != nullptr) && (controlType == ESystemItemType::Trigger))
	{
		pConnectionNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
		pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

		string path;
		CImplItem const* pParent = pImplControl->GetParent();

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
	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		++m_connectionsByID[pImplControl->GetId()];
		pImplControl->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		int connectionCount = m_connectionsByID[pImplControl->GetId()] - 1;

		if (connectionCount <= 0)
		{
			connectionCount = 0;
			pImplControl->SetConnected(false);
		}

		m_connectionsByID[pImplControl->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	// Delete all the controls
	for (auto const& controlPair : m_controlsCache)
	{
		CImplItem const* const pImplControl = controlPair.second;

		if (pImplControl != nullptr)
		{
			delete pImplControl;
		}
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_rootControl = CImplItem();
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::CreateControlCache(CImplItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->ChildCount();

		for (size_t i = 0; i < count; ++i)
		{
			CImplItem* const pChild = pParent->GetChildAt(i);

			if (pChild != nullptr)
			{
				m_controlsCache[pChild->GetId()] = pChild;
				CreateControlCache(pChild);
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
