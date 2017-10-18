// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ProjectLoader.h"
#include "ImplControl.h"

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <CryString/CryPath.h>
#include <CryCore/StlUtils.h>
#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IProfileData.h>
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
namespace PortAudio
{
string const CEditorImpl::s_eventConnectionTag = "PortAudioEvent";
string const CEditorImpl::s_sampleConnectionTag = "PortAudioSample";
string const CEditorImpl::s_itemNameTag = "portaudio_name";
string const CEditorImpl::s_pathNameTag = "portaudio_path";
string const CEditorImpl::s_panningEnabledTag = "enable_panning";
string const CEditorImpl::s_attenuationEnabledTag = "enable_distance_attenuation";
string const CEditorImpl::s_attenuationDistMin = "attenuation_dist_min";
string const CEditorImpl::s_attenuationDistMax = "attenuation_dist_max";
string const CEditorImpl::s_volumeTag = "volume";
string const CEditorImpl::s_loopCountTag = "loop_count";
string const CEditorImpl::s_connectionTypeTag = "event_type";

string const g_userSettingsFile = "%USER%/audiocontrolseditor_portaudio.user";

//////////////////////////////////////////////////////////////////////////
void CImplSettings::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
{
	Serialization::LoadJsonFile(m_implSettings, g_userSettingsFile.c_str());
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

	CProjectLoader(m_implSettings.GetProjectPath(), m_root);
	CreateControlCache(&m_root);

	for (auto const controlPair : m_connectionsByID)
	{
		if (controlPair.second.size() > 0)
		{
			CImplItem* const pImplControl = GetControl(controlPair.first);

			if (pImplControl != nullptr)
			{
				pImplControl->SetConnected(true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::GetControl(CID const id) const
{
	if (id >= 0)
	{
		size_t const size = m_controlsCache.size();

		for (size_t i = 0; i < size; ++i)
		{
			if (m_controlsCache[i]->GetId() == id)
			{
				return m_controlsCache[i];
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
TImplControlTypeMask CEditorImpl::GetCompatibleTypes(ESystemItemType const systemType) const
{
	switch (systemType)
	{
	case ESystemItemType::Trigger:
		return static_cast<TImplControlTypeMask>(EImpltemType::Event);
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(CImplItem const* const pImplItem) const
{
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Event:
		return ":Icons/portaudio/event.ico";
	case EImpltemType::Folder:
		return "icons:General/Folder.ico";
	}
	return "icons:Dialogs/dialog-error.ico";
}

//////////////////////////////////////////////////////////////////////////
string CEditorImpl::GetName() const
{
	return gEnv->pAudioSystem->GetProfileData()->GetImplName();
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
{
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Event:
		return ESystemItemType::Trigger;
	}
	return ESystemItemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem)
{
	std::shared_ptr<CConnection> const pConnection = std::make_shared<CConnection>(pImplItem->GetId());
	pImplItem->SetConnected(true);
	m_connectionsByID[pImplItem->GetId()].push_back(pConnection);
	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType)
{
	if (pNode != nullptr)
	{
		string const nodeTag = pNode->getTag();

		if ((nodeTag == s_eventConnectionTag) || (nodeTag == s_sampleConnectionTag))
		{
			string const name = pNode->getAttr(s_itemNameTag);
			string const path = pNode->getAttr(s_pathNameTag);

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
				m_controlsCache.push_back(pImplControl);
			}

			if (pImplControl != nullptr)
			{
				PortAudioConnectionPtr const pConnection = std::make_shared<CConnection>(pImplControl->GetId());
				pImplControl->SetConnected(true);
				m_connectionsByID[pImplControl->GetId()].push_back(pConnection);
				string const connectionType = pNode->getAttr(s_connectionTypeTag);
				pConnection->m_type = connectionType == "stop" ? EConnectionType::Stop : EConnectionType::Start;

				string const enablePanning = pNode->getAttr(s_panningEnabledTag);
				pConnection->m_isPanningEnabled = enablePanning == "true" ? true : false;

				string const enableDistAttenuation = pNode->getAttr(s_attenuationEnabledTag);
				pConnection->m_isAttenuationEnabled = enableDistAttenuation == "true" ? true : false;

				pNode->getAttr(s_attenuationDistMin, pConnection->m_minAttenuation);
				pNode->getAttr(s_attenuationDistMax, pConnection->m_maxAttenuation);
				pNode->getAttr(s_volumeTag, pConnection->m_volume);
				pNode->getAttr(s_loopCountTag, pConnection->m_loopCount);

				if (pConnection->m_loopCount == -1)
				{
					pConnection->m_isInfiniteLoop = true;
				}

				return pConnection;
			}

		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	std::shared_ptr<const CConnection> const pSDLMixerConnection = std::static_pointer_cast<const CConnection>(pConnection);
	CImplItem const* const pImplControl = GetControl(pConnection->GetID());
	if ((pImplControl != nullptr) && (pSDLMixerConnection != nullptr) && (controlType == ESystemItemType::Trigger))
	{
		XmlNodeRef const pConnectionNode = GetISystem()->CreateXmlNode(s_eventConnectionTag);
		pConnectionNode->setAttr(s_itemNameTag, pImplControl->GetName());

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

		pConnectionNode->setAttr(s_pathNameTag, path);

		if (pSDLMixerConnection->m_type == EConnectionType::Start)
		{
			pConnectionNode->setAttr(s_connectionTypeTag, "start");
			pConnectionNode->setAttr(s_panningEnabledTag, pSDLMixerConnection->m_isPanningEnabled ? "true" : "false");
			pConnectionNode->setAttr(s_attenuationEnabledTag, pSDLMixerConnection->m_isAttenuationEnabled ? "true" : "false");
			pConnectionNode->setAttr(s_attenuationDistMin, pSDLMixerConnection->m_minAttenuation);
			pConnectionNode->setAttr(s_attenuationDistMax, pSDLMixerConnection->m_maxAttenuation);
			pConnectionNode->setAttr(s_volumeTag, pSDLMixerConnection->m_volume);

			if (pSDLMixerConnection->m_isInfiniteLoop)
			{
				pConnectionNode->setAttr(s_loopCountTag, -1);
			}
			else
			{
				pConnectionNode->setAttr(s_loopCountTag, pSDLMixerConnection->m_loopCount);
			}
		}
		else
		{
			pConnectionNode->setAttr(s_connectionTypeTag, "stop");
		}

		return pConnectionNode;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GetId(const string& name) const
{
	return CCrc32::ComputeLowercase(name);
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
				m_controlsCache.push_back(pChild);
				CreateControlCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	// Delete all the controls
	for (CImplItem const* const pControl : m_controlsCache)
	{
		delete pControl;
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_root = CImplItem();
}

SERIALIZATION_ENUM_BEGIN(EConnectionType, "Event Type")
SERIALIZATION_ENUM(EConnectionType::Start, "start", "Start")
SERIALIZATION_ENUM(EConnectionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace PortAudio
} // namespace ACE
