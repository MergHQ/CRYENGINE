// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor_sdlmixer.h"

#include "SDLMixerProjectLoader.h"
#include "AudioSystemControl_sdlmixer.h"

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
string const CAudioSystemEditor_sdlmixer::s_eventConnectionTag = "SDLMixerEvent";
string const CAudioSystemEditor_sdlmixer::s_sampleConnectionTag = "SDLMixerSample";
string const CAudioSystemEditor_sdlmixer::s_itemNameTag = "sdl_name";
string const CAudioSystemEditor_sdlmixer::s_pathNameTag = "sdl_path";
string const CAudioSystemEditor_sdlmixer::s_panningEnabledTag = "enable_panning";
string const CAudioSystemEditor_sdlmixer::s_attenuationEnabledTag = "enable_distance_attenuation";
string const CAudioSystemEditor_sdlmixer::s_attenuationDistMin = "attenuation_dist_min";
string const CAudioSystemEditor_sdlmixer::s_attenuationDistMax = "attenuation_dist_max";
string const CAudioSystemEditor_sdlmixer::s_volumeTag = "volume";
string const CAudioSystemEditor_sdlmixer::s_loopCountTag = "loop_count";
string const CAudioSystemEditor_sdlmixer::s_connectionTypeTag = "event_type";

string const g_userSettingsFile = "%USER%/audiocontrolseditor_sdlmixer.user";

//////////////////////////////////////////////////////////////////////////
void CImplementationSettings_sdlmixer::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor_sdlmixer::CAudioSystemEditor_sdlmixer()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
	m_controlsCache.reserve(1024);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor_sdlmixer::~CAudioSystemEditor_sdlmixer()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_sdlmixer::Reload(bool const preserveConnectionStatus)
{
	Clear();

	CSdlMixerProjectLoader(m_settings.GetProjectPath(), m_root);
	CreateControlCache(&m_root);

	for (auto const& controlPair : m_connectionsByID)
	{
		if (!controlPair.second.empty())
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
IAudioSystemItem* CAudioSystemEditor_sdlmixer::GetControl(CID const id) const
{
	for (auto const pItem : m_controlsCache)
	{
		if (pItem->GetId() == id)
		{
			return pItem;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
TImplControlTypeMask CAudioSystemEditor_sdlmixer::GetCompatibleTypes(EItemType const controlType) const
{
	switch (controlType)
	{
	case EItemType::Trigger:
		return eSdlMixerTypes_Event;
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

//////////////////////////////////////////////////////////////////////////
char const* CAudioSystemEditor_sdlmixer::GetTypeIcon(ItemType const type) const
{
	switch (type)
	{
	case eSdlMixerTypes_Event:
		return "Editor/Icons/audio/sdl_mixer/Audio_Event.png";
	case eSdlMixerTypes_Folder:
		return "Editor/Icons/audio/sdl_mixer/Folder.ico";
	}
	return "Editor/Icons/audio/sdl_mixer/Audio_Event.png";
}

//////////////////////////////////////////////////////////////////////////
string CAudioSystemEditor_sdlmixer::GetName() const
{
	return gEnv->pAudioSystem->GetProfileData()->GetImplName();
}

//////////////////////////////////////////////////////////////////////////
EItemType CAudioSystemEditor_sdlmixer::ImplTypeToSystemType(ItemType const itemType) const
{
	switch (itemType)
	{
	case eSdlMixerTypes_Event:
		return EItemType::Trigger;
	}
	return EItemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor_sdlmixer::CreateConnectionToControl(EItemType const controlType, IAudioSystemItem* const pMiddlewareControl)
{
	std::shared_ptr<CSdlMixerConnection> const pConnection = std::make_shared<CSdlMixerConnection>(pMiddlewareControl->GetId());
	pMiddlewareControl->SetConnected(true);
	m_connectionsByID[pMiddlewareControl->GetId()].push_back(pConnection);
	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor_sdlmixer::CreateConnectionFromXMLNode(XmlNodeRef pNode, EItemType const controlType)
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

			IAudioSystemItem* pItem = GetControl(id);

			if (pItem == nullptr)
			{
				pItem = new IAudioSystemControl_sdlmixer(name, id, eSdlMixerTypes_Event);
				pItem->SetPlaceholder(true);
				m_controlsCache.push_back(pItem);
			}

			if (pItem != nullptr)
			{
				SdlConnectionPtr const pConnection = std::make_shared<CSdlMixerConnection>(pItem->GetId());
				pItem->SetConnected(true);
				m_connectionsByID[pItem->GetId()].push_back(pConnection);
				string const connectionType = pNode->getAttr(s_connectionTypeTag);
				pConnection->m_type = connectionType == "stop" ? eSdlMixerConnectionType_Stop : eSdlMixerConnectionType_Start;

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
XmlNodeRef CAudioSystemEditor_sdlmixer::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EItemType const controlType)
{
	std::shared_ptr<CSdlMixerConnection const> pSDLMixerConnection = std::static_pointer_cast<CSdlMixerConnection const>(pConnection);
	const IAudioSystemItem* const pControl = GetControl(pConnection->GetID());
	if ((pControl != nullptr) && (pSDLMixerConnection != nullptr) && (controlType == EItemType::Trigger))
	{
		XmlNodeRef const pConnectionNode = GetISystem()->CreateXmlNode(s_eventConnectionTag);
		pConnectionNode->setAttr(s_itemNameTag, pControl->GetName());

		string path;
		IAudioSystemItem const* pParent = pControl->GetParent();

		while (pParent)
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

		if (pSDLMixerConnection->m_type == eSdlMixerConnectionType_Start)
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
void CAudioSystemEditor_sdlmixer::EnableConnection(ConnectionPtr const pConnection)
{
	IAudioSystemItem* const pItem = GetControl(pConnection->GetID());

	if (pItem != nullptr)
	{
		CSdlMixerConnection const* const pConn = static_cast<CSdlMixerConnection*>(pConnection.get());
		pItem->SetRadius(pConn->m_maxAttenuation);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_sdlmixer::DisableConnection(ConnectionPtr const pConnection)
{
	IAudioSystemItem* const pItem = GetControl(pConnection->GetID());

	if (pItem != nullptr)
	{
		CSdlMixerConnection const* const pConn = static_cast<CSdlMixerConnection*>(pConnection.get());
		pItem->SetRadius(0.0f);
	}
}

//////////////////////////////////////////////////////////////////////////
CID CAudioSystemEditor_sdlmixer::GetId(string const& sName) const
{
	return CCrc32::ComputeLowercase(sName);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_sdlmixer::CreateControlCache(IAudioSystemItem const* const pParent)
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
void CAudioSystemEditor_sdlmixer::Clear()
{
	// Delete all the controls
	for (auto const pControl : m_controlsCache)
	{
		delete pControl;
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_root = IAudioSystemItem();
}

SERIALIZATION_ENUM_BEGIN(ESdlMixerConnectionType, "Event Type")
SERIALIZATION_ENUM(eSdlMixerConnectionType_Start, "start", "Start")
SERIALIZATION_ENUM(eSdlMixerConnectionType_Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()

//////////////////////////////////////////////////////////////////////////
void CSdlMixerConnection::Serialize(Serialization::IArchive& ar)
{
	ar(m_type, "action", "Action");
	ar(m_isPanningEnabled, "panning", "Enable Panning");

	if (ar.openBlock("DistanceAttenuation", "+Distance Attenuation"))
	{
		ar(m_isAttenuationEnabled, "attenuation", "Enable");

		if (m_isAttenuationEnabled)
		{
			if (ar.isInput())
			{
				float minAtt = m_minAttenuation;
				float maxAtt = m_maxAttenuation;
				ar(minAtt, "min_att", "Min Distance");
				ar(maxAtt, "max_att", "Max Distance");

				if (minAtt > maxAtt)
				{
					if (minAtt != m_minAttenuation)
					{
						maxAtt = minAtt;
					}
					else
					{
						minAtt = maxAtt;
					}
				}

				m_minAttenuation = minAtt;
				m_maxAttenuation = maxAtt;

				IAudioSystemItem* const pItem = s_pSdlMixerInterface->GetControl(GetID());

				if (pItem != nullptr)
				{
					pItem->SetRadius(m_maxAttenuation);
				}
			}
			else
			{
				ar(m_minAttenuation, "min_att", "Min Distance");
				ar(m_maxAttenuation, "max_att", "Max Distance");
			}
		}
		else
		{
			ar(m_minAttenuation, "min_att", "!Min Distance");
			ar(m_maxAttenuation, "max_att", "!Max Distance");
		}
		ar.closeBlock();
	}

	ar(Serialization::Range(m_volume, -96.0f, 0.0f), "vol", "Volume (dB)");

	if (ar.openBlock("Looping", "+Looping"))
	{
		ar(m_isInfiniteLoop, "infinite", "Infinite");

		if (m_isInfiniteLoop)
		{
			ar(m_loopCount, "loop_count", "!Count");
		}
		else
		{
			ar(m_loopCount, "loop_count", "Count");
		}
		ar.closeBlock();
	}

	if (ar.isInput())
	{
		signalConnectionChanged();
	}
}
} // namespace ACE
