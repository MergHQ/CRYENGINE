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
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
const string CAudioSystemEditor_sdlmixer::ms_eventConnectionTag = "SDLMixerEvent";
const string CAudioSystemEditor_sdlmixer::ms_sampleConnectionTag = "SDLMixerSample";
const string CAudioSystemEditor_sdlmixer::ms_controlNameTag = "sdl_name";
const string CAudioSystemEditor_sdlmixer::ms_panningEnabledTag = "enable_panning";
const string CAudioSystemEditor_sdlmixer::ms_attenuationEnabledTag = "enable_distance_attenuation";
const string CAudioSystemEditor_sdlmixer::ms_attenuationDistMin = "attenuation_dist_min";
const string CAudioSystemEditor_sdlmixer::ms_attenuationDistMax = "attenuation_dist_max";
const string CAudioSystemEditor_sdlmixer::ms_volumeTag = "volume";
const string CAudioSystemEditor_sdlmixer::ms_loopCountTag = "loop_count";
const string CAudioSystemEditor_sdlmixer::ms_connectionTypeTag = "event_type";

const string g_userSettingsFile = "%USER%/audiocontrolseditor_sdlmixer.user";

CAudioSystemEditor_sdlmixer::CAudioSystemEditor_sdlmixer()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
	Reload();
}

CAudioSystemEditor_sdlmixer::~CAudioSystemEditor_sdlmixer()
{
}

void CAudioSystemEditor_sdlmixer::Reload(bool bPreserveConnectionStatus)
{
	m_controls.clear();
	m_root = IAudioSystemItem();
	ACE::CSDLMixerProjectLoader(m_settings.GetProjectPath(), this);

	TSDLMixerConnections::const_iterator it = m_connectionsByID.begin();
	TSDLMixerConnections::const_iterator end = m_connectionsByID.end();
	for (; it != end; ++it)
	{
		if (it->second.size() > 0)
		{
			IAudioSystemItem* pControl = GetControl(it->first);
			if (pControl)
			{
				pControl->SetConnected(true);
			}
		}
	}
}

IAudioSystemItem* CAudioSystemEditor_sdlmixer::CreateControl(const SControlDef& controlDefinition)
{
	const CID nID = GetId(controlDefinition.name);
	IAudioSystemControl_sdlmixer* pControl = new IAudioSystemControl_sdlmixer(controlDefinition.name, nID, controlDefinition.type);
	if (pControl)
	{
		IAudioSystemItem* pParent = controlDefinition.pParent;
		if (pParent == nullptr)
		{
			pParent = &m_root;
		}

		pParent->AddChild(pControl);
		m_controls.push_back(pControl);
	}
	return pControl;
}

IAudioSystemItem* CAudioSystemEditor_sdlmixer::GetControl(CID id) const
{
	if (id >= 0)
	{
		size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (m_controls[i]->GetId() == id)
			{
				return m_controls[i];
			}
		}
	}
	return nullptr;
}

ConnectionPtr CAudioSystemEditor_sdlmixer::CreateConnectionToControl(EACEControlType eATLControlType, IAudioSystemItem* pMiddlewareControl)
{
	std::shared_ptr<CSDLMixerConnection> pConnection = std::make_shared<CSDLMixerConnection>(pMiddlewareControl->GetId());
	pMiddlewareControl->SetConnected(true);
	m_connectionsByID[pMiddlewareControl->GetId()].push_back(pConnection);
	return pConnection;
}

ConnectionPtr CAudioSystemEditor_sdlmixer::CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType)
{
	if (pNode)
	{
		const string sTag = pNode->getTag();
		if (sTag == ms_eventConnectionTag || sTag == ms_sampleConnectionTag)
		{
			const string sName = pNode->getAttr(ms_controlNameTag);
			CID id = GetId(sName);
			if (id != ACE_INVALID_ID)
			{
				IAudioSystemItem* pControl = GetControl(id);
				if (pControl == nullptr)
				{
					pControl = CreateControl(SControlDef(sName, eSDLMixerTypes_Event));
					if (pControl)
					{
						pControl->SetPlaceholder(true);
					}
				}

				if (pControl)
				{
					TSDLConnectionPtr pConnection = std::make_shared<CSDLMixerConnection>(pControl->GetId());
					pControl->SetConnected(true);
					m_connectionsByID[pControl->GetId()].push_back(pConnection);

					const string sConnectionType = pNode->getAttr(ms_connectionTypeTag);
					pConnection->eType = sConnectionType == "stop" ? eSDLMixerConnectionType_Stop : eSDLMixerConnectionType_Start;

					const string sEnablePanning = pNode->getAttr(ms_panningEnabledTag);
					pConnection->bPanningEnabled = sEnablePanning == "true" ? true : false;

					const string sEnableDistAttenuation = pNode->getAttr(ms_attenuationEnabledTag);
					pConnection->bAttenuationEnabled = sEnableDistAttenuation == "true" ? true : false;

					const string sAttenuationMin = pNode->getAttr(ms_attenuationDistMin);
					pConnection->fMinAttenuation = (float)std::atof(sAttenuationMin.c_str());

					const string sAttenuationMax = pNode->getAttr(ms_attenuationDistMax);
					pConnection->fMaxAttenuation = (float)std::atof(sAttenuationMax.c_str());

					const string sVolume = pNode->getAttr(ms_volumeTag);
					pConnection->fVolume = (float)std::atof(sVolume.c_str());

					const string sLoopCount = pNode->getAttr(ms_loopCountTag);
					pConnection->nLoopCount = std::atoi(sLoopCount.c_str());
					if (pConnection->nLoopCount == -1)
					{
						pConnection->bInfiniteLoop = true;
					}

					return pConnection;
				}
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [SDL Mixer] Error reading connection to %s", sName);
			}
		}
	}
	return nullptr;
}

XmlNodeRef CAudioSystemEditor_sdlmixer::CreateXMLNodeFromConnection(const ConnectionPtr pConnection, const EACEControlType eATLControlType)
{
	std::shared_ptr<const CSDLMixerConnection> pSDLMixerConnection = std::static_pointer_cast<const CSDLMixerConnection>(pConnection);
	const IAudioSystemItem* pControl = GetControl(pConnection->GetID());
	if (pControl && pSDLMixerConnection && eATLControlType == eACEControlType_Trigger)
	{
		XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(ms_eventConnectionTag);
		pConnectionNode->setAttr(ms_controlNameTag, pControl->GetName());

		if (pSDLMixerConnection->eType == eSDLMixerConnectionType_Start)
		{
			pConnectionNode->setAttr(ms_connectionTypeTag, "start");
			pConnectionNode->setAttr(ms_panningEnabledTag, pSDLMixerConnection->bPanningEnabled ? "true" : "false");
			pConnectionNode->setAttr(ms_attenuationEnabledTag, pSDLMixerConnection->bAttenuationEnabled ? "true" : "false");
			pConnectionNode->setAttr(ms_attenuationDistMin, pSDLMixerConnection->fMinAttenuation);
			pConnectionNode->setAttr(ms_attenuationDistMax, pSDLMixerConnection->fMaxAttenuation);
			pConnectionNode->setAttr(ms_volumeTag, pSDLMixerConnection->fVolume);
			if (pSDLMixerConnection->bInfiniteLoop)
			{
				pConnectionNode->setAttr(ms_loopCountTag, -1);
			}
			else
			{
				pConnectionNode->setAttr(ms_loopCountTag, pSDLMixerConnection->nLoopCount);
			}
		}
		else
		{
			pConnectionNode->setAttr(ms_connectionTypeTag, "stop");
		}

		return pConnectionNode;
	}
	return nullptr;
}

ACE::CID CAudioSystemEditor_sdlmixer::GetId(const string& sName) const
{
	return CCrc32::Compute(sName);
}

const char* CAudioSystemEditor_sdlmixer::GetTypeIcon(ItemType type) const
{
	return "Editor/Icons/audio/sdl_mixer/Audio_Event.png";
}

ACE::EACEControlType CAudioSystemEditor_sdlmixer::ImplTypeToATLType(ItemType type) const
{
	return eACEControlType_Trigger;
}

ACE::TImplControlTypeMask CAudioSystemEditor_sdlmixer::GetCompatibleTypes(EACEControlType eATLControlType) const
{
	switch (eATLControlType)
	{
	case eACEControlType_Trigger:
		return eSDLMixerTypes_Event;
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

string CAudioSystemEditor_sdlmixer::GetName() const
{
	return "SDL Mixer";
}

void CImplementationSettings_sdlmixer::SetProjectPath(const char* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

SERIALIZATION_ENUM_BEGIN(ESDLMixerConnectionType, "Event Type")
SERIALIZATION_ENUM(eSDLMixerConnectionType_Start, "start", "Start")
SERIALIZATION_ENUM(eSDLMixerConnectionType_Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()

}
