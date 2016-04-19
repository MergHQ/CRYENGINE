// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MFXAudioEffect.h"

#include <CryAudio/IAudioSystem.h>

namespace MaterialEffectsUtils
{
struct SAudio1pOr3pSwitch
{
	static const SAudio1pOr3pSwitch& Instance()
	{
		static SAudio1pOr3pSwitch theInstance;
		return theInstance;
	}

	AudioControlId GetSwitchId() const
	{
		return m_switchID;
	}

	AudioSwitchStateId Get1pStateId() const
	{
		return m_1pStateID;
	}

	AudioSwitchStateId Get3pStateId() const
	{
		return m_3pStateID;
	}

	bool IsValid() const
	{
		return m_isValid;
	}

private:

	SAudio1pOr3pSwitch()
		: m_switchID(INVALID_AUDIO_CONTROL_ID)
		, m_1pStateID(INVALID_AUDIO_SWITCH_STATE_ID)
		, m_3pStateID(INVALID_AUDIO_SWITCH_STATE_ID)
		, m_isValid(false)
	{
		Initialize();
	}

	void Initialize()
	{
		gEnv->pAudioSystem->GetAudioSwitchId("1stOr3rdP", m_switchID);
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_switchID, "1stP", m_1pStateID);
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_switchID, "3rdP", m_3pStateID);

		m_isValid = (m_switchID != INVALID_AUDIO_CONTROL_ID) &&
		            (m_1pStateID != INVALID_AUDIO_SWITCH_STATE_ID) && (m_3pStateID != INVALID_AUDIO_SWITCH_STATE_ID);
	}

	AudioControlId     m_switchID;
	AudioSwitchStateId m_1pStateID;
	AudioSwitchStateId m_3pStateID;

	bool               m_isValid;
};

template<typename AudioProxyType>
void PrepareForAudioTriggerExecution(AudioProxyType* pIAudioProxy, const SMFXAudioEffectParams& audioParams, const SMFXRunTimeEffectParams& runtimeParams)
{
	const MaterialEffectsUtils::SAudio1pOr3pSwitch& audio1pOr3pSwitch = MaterialEffectsUtils::SAudio1pOr3pSwitch::Instance();

	if (audio1pOr3pSwitch.IsValid())
	{
		pIAudioProxy->SetSwitchState(
		  audio1pOr3pSwitch.GetSwitchId(),
		  runtimeParams.playSoundFP ? audio1pOr3pSwitch.Get1pStateId() : audio1pOr3pSwitch.Get3pStateId());
	}

	for (SMFXAudioEffectParams::TSwitches::const_iterator it = audioParams.triggerSwitches.begin(), itEnd = audioParams.triggerSwitches.end(); it != itEnd; ++it)
	{
		const SAudioSwitchWrapper& switchWrapper = *it;
		pIAudioProxy->SetSwitchState(switchWrapper.GetSwitchId(), switchWrapper.GetSwitchStateId());
	}

	REINST("IEntityAudioProxy needs to support multiple IAudioProxy objects to properly handle Rtpcs tied to specific events");

	//Note: Rtpcs are global for the audio proxy object.
	//      This can be a problem if the sound is processed through IEntityAudioProxy, where the object is shared for all audio events triggered through it!
	//TODO: Add support to IEntityAudioProxy to handle multiple audio proxy objects
	for (int i = 0; i < runtimeParams.numAudioRtpcs; ++i)
	{
		const char* rtpcName = runtimeParams.audioRtpcs[i].rtpcName;
		if (rtpcName && *rtpcName)
		{
			AudioControlId rtpcId = INVALID_AUDIO_CONTROL_ID;
			if (gEnv->pAudioSystem->GetAudioRtpcId(rtpcName, rtpcId))
			{
				pIAudioProxy->SetRtpcValue(rtpcId, runtimeParams.audioRtpcs[i].rtpcValue);
			}
		}
	}
}

}

//////////////////////////////////////////////////////////////////////////

void SAudioTriggerWrapper::Init(const char* triggerName)
{
	CRY_ASSERT(triggerName != NULL);

	gEnv->pAudioSystem->GetAudioTriggerId(triggerName, m_triggerID);

#if defined(MATERIAL_EFFECTS_DEBUG)
	m_triggerName = triggerName;
#endif
}

void SAudioSwitchWrapper::Init(const char* switchName, const char* switchStateName)
{
	CRY_ASSERT(switchName != NULL);
	CRY_ASSERT(switchStateName != NULL);

	gEnv->pAudioSystem->GetAudioSwitchId(switchName, m_switchID);
	gEnv->pAudioSystem->GetAudioSwitchStateId(m_switchID, switchStateName, m_switchStateID);

#if defined(MATERIAL_EFFECTS_DEBUG)
	m_switchName = switchName;
	m_switchStateName = switchStateName;
#endif
}

//////////////////////////////////////////////////////////////////////////

CMFXAudioEffect::CMFXAudioEffect()
	: CMFXEffectBase(eMFXPF_Audio)
{

}

void CMFXAudioEffect::Execute(const SMFXRunTimeEffectParams& params)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	IF_UNLIKELY (!m_audioParams.trigger.IsValid())
		return;

	IEntity* pOwnerEntity = (params.audioProxyEntityId != 0) ? gEnv->pEntitySystem->GetEntity(params.audioProxyEntityId) : NULL;
	if (pOwnerEntity)
	{
		IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pOwnerEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		CRY_ASSERT(pIEntityAudioProxy);

		MaterialEffectsUtils::PrepareForAudioTriggerExecution<IEntityAudioProxy>(pIEntityAudioProxy.get(), m_audioParams, params);

		pIEntityAudioProxy->ExecuteTrigger(m_audioParams.trigger.GetTriggerId(), params.audioProxyId);
	}
	else
	{
		IAudioProxy* pIAudioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();
		if (pIAudioProxy != NULL)
		{
			pIAudioProxy->Initialize("MFXAudioEffect");

			MaterialEffectsUtils::PrepareForAudioTriggerExecution<IAudioProxy>(pIAudioProxy, m_audioParams, params);

			pIAudioProxy->SetPosition(params.pos);
			pIAudioProxy->SetOcclusionType(eAudioOcclusionType_SingleRay);
			pIAudioProxy->SetCurrentEnvironments();
			pIAudioProxy->ExecuteTrigger(m_audioParams.trigger.GetTriggerId());
		}
		SAFE_RELEASE(pIAudioProxy);
	}
}

void CMFXAudioEffect::GetResources(SMFXResourceList& resourceList) const
{
	SMFXAudioListNode* pListNode = SMFXAudioListNode::Create();

	pListNode->m_audioParams.triggerName = m_audioParams.trigger.GetTriggerName();

	const size_t switchesCount = MIN(m_audioParams.triggerSwitches.size(), pListNode->m_audioParams.triggerSwitches.max_size());

	for (size_t i = 0; i < switchesCount; ++i)
	{
		IMFXAudioParams::SSwitchData switchData;
		switchData.switchName = m_audioParams.triggerSwitches[i].GetSwitchName();
		switchData.switchStateName = m_audioParams.triggerSwitches[i].GetSwitchStateName();

		pListNode->m_audioParams.triggerSwitches.push_back(switchData);
	}

	SMFXAudioListNode* pNextNode = resourceList.m_audioList;

	if (pNextNode == NULL)
	{
		resourceList.m_audioList = pListNode;
	}
	else
	{
		while (pNextNode->pNext)
			pNextNode = pNextNode->pNext;

		pNextNode->pNext = pListNode;
	}
}

void CMFXAudioEffect::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
	// Xml data format
	/*
	   <Audio trigger=”footstep”>
	   <Switch name=”Switch1” state=”switch1_state” />
	   <Switch name=”Switch2” state=”swtich2_state” />
	   <Switch ... />
	   </Audio>
	 */

	m_audioParams.trigger.Init(paramsNode->getAttr("trigger"));

	const int childCount = paramsNode->getChildCount();
	m_audioParams.triggerSwitches.reserve(childCount);

	for (int i = 0; i < childCount; ++i)
	{
		const XmlNodeRef& childNode = paramsNode->getChild(i);

		if (strcmp(childNode->getTag(), "Switch") == 0)
		{
			SAudioSwitchWrapper switchWrapper;
			switchWrapper.Init(childNode->getAttr("name"), childNode->getAttr("state"));
			if (switchWrapper.IsValid())
			{
				m_audioParams.triggerSwitches.push_back(switchWrapper);
			}
#if defined(MATERIAL_EFFECTS_DEBUG)
			else
			{
				GameWarning("[MFX] AudioEffect (at line %d) : Switch '%s' or SwitchState '%s' not valid", paramsNode->getLine(), switchWrapper.GetSwitchName(), switchWrapper.GetSwitchStateName());
			}
#endif
		}
	}

#if defined(MATERIAL_EFFECTS_DEBUG)
	if (!m_audioParams.trigger.IsValid())
	{
		GameWarning("[MFX] AudioEffect (at line %d) : Trigger '%s'not valid", paramsNode->getLine(), m_audioParams.trigger.GetTriggerName());
	}
#endif
}

void CMFXAudioEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(&m_audioParams);
}
