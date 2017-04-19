// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MFXAudioEffect.h"
#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IObject.h>

using namespace CryAudio;

namespace MaterialEffectsUtils
{
struct SAudio1pOr3pSwitch
{
	static const SAudio1pOr3pSwitch& Instance()
	{
		static SAudio1pOr3pSwitch theInstance;
		return theInstance;
	}

	ControlId GetSwitchId() const
	{
		return m_switchID;
	}

	SwitchStateId Get1pStateId() const
	{
		return m_1pStateID;
	}

	SwitchStateId Get3pStateId() const
	{
		return m_3pStateID;
	}

	bool IsValid() const
	{
		return m_isValid;
	}

private:

	SAudio1pOr3pSwitch()
		: m_switchID(InvalidControlId)
		, m_1pStateID(InvalidSwitchStateId)
		, m_3pStateID(InvalidSwitchStateId)
		, m_isValid(false)
	{
		Initialize();
	}

	void Initialize()
	{
		gEnv->pAudioSystem->GetAudioSwitchId("1stOr3rdP", m_switchID);
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_switchID, "1stP", m_1pStateID);
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_switchID, "3rdP", m_3pStateID);

		m_isValid = (m_switchID != InvalidControlId) &&
		            (m_1pStateID != InvalidSwitchStateId) && (m_3pStateID != InvalidSwitchStateId);
	}

	ControlId     m_switchID;
	SwitchStateId m_1pStateID;
	SwitchStateId m_3pStateID;

	bool          m_isValid;
};

template<typename AudioObjectType>
void PrepareForAudioTriggerExecution(AudioObjectType* pIAudioObject, const SMFXAudioEffectParams& audioParams, const SMFXRunTimeEffectParams& runtimeParams)
{
	const MaterialEffectsUtils::SAudio1pOr3pSwitch& audio1pOr3pSwitch = MaterialEffectsUtils::SAudio1pOr3pSwitch::Instance();

	if (audio1pOr3pSwitch.IsValid())
	{
		pIAudioObject->SetSwitchState(
		  audio1pOr3pSwitch.GetSwitchId(),
		  runtimeParams.playSoundFP ? audio1pOr3pSwitch.Get1pStateId() : audio1pOr3pSwitch.Get3pStateId());
	}

	for (SMFXAudioEffectParams::TSwitches::const_iterator it = audioParams.triggerSwitches.begin(), itEnd = audioParams.triggerSwitches.end(); it != itEnd; ++it)
	{
		const SAudioSwitchWrapper& switchWrapper = *it;
		pIAudioObject->SetSwitchState(switchWrapper.GetSwitchId(), switchWrapper.GetSwitchStateId());
	}

	for (int i = 0; i < runtimeParams.numAudioRtpcs; ++i)
	{
		const char* rtpcName = runtimeParams.audioRtpcs[i].rtpcName;
		if (rtpcName && *rtpcName)
		{
			ControlId rtpcId = InvalidControlId;
			if (gEnv->pAudioSystem->GetAudioParameterId(rtpcName, rtpcId))
			{
				pIAudioObject->SetParameter(rtpcId, runtimeParams.audioRtpcs[i].rtpcValue);
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
		IEntityAudioComponent* pIEntityAudioComponent = pOwnerEntity->GetOrCreateComponent<IEntityAudioComponent>();
		CRY_ASSERT(pIEntityAudioComponent);

		MaterialEffectsUtils::PrepareForAudioTriggerExecution<IEntityAudioComponent>(pIEntityAudioComponent, m_audioParams, params);

		pIEntityAudioComponent->ExecuteTrigger(m_audioParams.trigger.GetTriggerId(), params.audioProxyId);
	}
	else
	{
		SCreateObjectData const objectData("MFXAudioEffect", EOcclusionType::Low, params.pos, INVALID_ENTITYID, true);
		IObject* const pIObject = gEnv->pAudioSystem->CreateObject(objectData);

		MaterialEffectsUtils::PrepareForAudioTriggerExecution<IObject>(pIObject, m_audioParams, params);

		pIObject->ExecuteTrigger(m_audioParams.trigger.GetTriggerId());
		gEnv->pAudioSystem->ReleaseObject(pIObject);
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
	   <Audio trigger="footstep">
	   <Switch name="Switch1" state="switch1_state" />
	   <Switch name="Switch2" state="swtich2_state" />
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
