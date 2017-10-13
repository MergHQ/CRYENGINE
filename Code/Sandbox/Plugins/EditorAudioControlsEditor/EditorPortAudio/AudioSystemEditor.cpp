// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor.h"

#include "ProjectLoader.h"
#include "AudioSystemControl.h"

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
string const CAudioSystemEditor::s_eventConnectionTag = "PortAudioEvent";
string const CAudioSystemEditor::s_sampleConnectionTag = "PortAudioSample";
string const CAudioSystemEditor::s_itemNameTag = "portaudio_name";
string const CAudioSystemEditor::s_pathNameTag = "portaudio_path";
string const CAudioSystemEditor::s_panningEnabledTag = "enable_panning";
string const CAudioSystemEditor::s_attenuationEnabledTag = "enable_distance_attenuation";
string const CAudioSystemEditor::s_attenuationDistMin = "attenuation_dist_min";
string const CAudioSystemEditor::s_attenuationDistMax = "attenuation_dist_max";
string const CAudioSystemEditor::s_volumeTag = "volume";
string const CAudioSystemEditor::s_loopCountTag = "loop_count";
string const CAudioSystemEditor::s_connectionTypeTag = "event_type";

string const g_userSettingsFile = "%USER%/audiocontrolseditor_portaudio.user";

//////////////////////////////////////////////////////////////////////////
void CImplementationSettings::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor::CAudioSystemEditor()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor::~CAudioSystemEditor()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor::Reload(bool const preserveConnectionStatus)
{
	Clear();

	CProjectLoader(m_settings.GetProjectPath(), m_root);
	CreateControlCache(&m_root);

	for (auto const controlPair : m_connectionsByID)
	{
		if (controlPair.second.size() > 0)
		{
			IAudioSystemItem* const pControl = GetControl(controlPair.first);

			if (pControl != nullptr)
			{
				pControl->SetConnected(true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
IAudioSystemItem* CAudioSystemEditor::GetControl(CID const id) const
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
TImplControlTypeMask CAudioSystemEditor::GetCompatibleTypes(EItemType const controlType) const
{
	switch (controlType)
	{
	case EItemType::Trigger:
		return ePortAudioTypes_Event;
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

//////////////////////////////////////////////////////////////////////////
char const* CAudioSystemEditor::GetTypeIcon(ItemType const type) const
{
	switch (type)
	{
	case ePortAudioTypes_Event:
		return "Editor/Icons/audio/portaudio/Audio_Event.png";
	case ePortAudioTypes_Folder:
		return "Editor/Icons/audio/portaudio/Folder.ico";
	}
	return "Editor/Icons/audio/portaudio/Audio_Event.png";
}

//////////////////////////////////////////////////////////////////////////
string CAudioSystemEditor::GetName() const
{
	return gEnv->pAudioSystem->GetProfileData()->GetImplName();
}

//////////////////////////////////////////////////////////////////////////
EItemType CAudioSystemEditor::ImplTypeToSystemType(ItemType const itemType) const
{
	switch (itemType)
	{
	case ePortAudioTypes_Event:
		return EItemType::Trigger;
	}
	return EItemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor::CreateConnectionToControl(EItemType const controlType, IAudioSystemItem* const pMiddlewareControl)
{
	std::shared_ptr<CConnection> const pConnection = std::make_shared<CConnection>(pMiddlewareControl->GetId());
	pMiddlewareControl->SetConnected(true);
	m_connectionsByID[pMiddlewareControl->GetId()].push_back(pConnection);
	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor::CreateConnectionFromXMLNode(XmlNodeRef pNode, EItemType const controlType)
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

			IAudioSystemItem* pControl = GetControl(id);

			if (pControl == nullptr)
			{
				pControl = new IAudioSystemControl(name, id, ePortAudioTypes_Event);
				pControl->SetPlaceholder(true);
				m_controlsCache.push_back(pControl);
			}

			if (pControl != nullptr)
			{
				PortAudioConnectionPtr const pConnection = std::make_shared<CConnection>(pControl->GetId());
				pControl->SetConnected(true);
				m_connectionsByID[pControl->GetId()].push_back(pConnection);
				string const connectionType = pNode->getAttr(s_connectionTypeTag);
				pConnection->m_type = connectionType == "stop" ? ePortAudioConnectionType_Stop : ePortAudioConnectionType_Start;

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
XmlNodeRef CAudioSystemEditor::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EItemType const controlType)
{
	std::shared_ptr<const CConnection> const pSDLMixerConnection = std::static_pointer_cast<const CConnection>(pConnection);
	IAudioSystemItem const* const pControl = GetControl(pConnection->GetID());
	if ((pControl != nullptr) && (pSDLMixerConnection != nullptr) && (controlType == EItemType::Trigger))
	{
		XmlNodeRef const pConnectionNode = GetISystem()->CreateXmlNode(s_eventConnectionTag);
		pConnectionNode->setAttr(s_itemNameTag, pControl->GetName());

		string path;
		IAudioSystemItem const* pParent = pControl->GetParent();

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

		if (pSDLMixerConnection->m_type == ePortAudioConnectionType_Start)
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
CID CAudioSystemEditor::GetId(const string& name) const
{
	return CCrc32::ComputeLowercase(name);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor::CreateControlCache(IAudioSystemItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->ChildCount();

		for (size_t i = 0; i < count; ++i)
		{
			IAudioSystemItem* const pChild = pParent->GetChildAt(i);

			if (pChild != nullptr)
			{
				m_controlsCache.push_back(pChild);
				CreateControlCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor::Clear()
{
	// Delete all the controls
	for (IAudioSystemItem const* const pControl : m_controlsCache)
	{
		delete pControl;
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_root = IAudioSystemItem();
}

SERIALIZATION_ENUM_BEGIN(EPortAudioConnectionType, "Event Type")
SERIALIZATION_ENUM(ePortAudioConnectionType_Start, "start", "Start")
SERIALIZATION_ENUM(ePortAudioConnectionType_Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace ACE
