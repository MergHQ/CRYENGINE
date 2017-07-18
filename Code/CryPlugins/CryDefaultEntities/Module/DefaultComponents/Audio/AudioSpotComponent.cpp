// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioSpotComponent.h"

#include <CryAudio/IAudioSystem.h>

#include <CryMath/Random.h>

namespace Cry
{
namespace DefaultComponents
{
SERIALIZATION_ENUM_BEGIN_NESTED(CEntityAudioSpotComponent, EPlayMode, "DefaultTriggerPlayMode")
SERIALIZATION_ENUM(CEntityAudioSpotComponent::EPlayMode::None, "None", "Not started automatically")
SERIALIZATION_ENUM(CEntityAudioSpotComponent::EPlayMode::TriggerOnce, "TriggerOnce", "Once")
SERIALIZATION_ENUM(CEntityAudioSpotComponent::EPlayMode::ReTriggerConstantly, "ReTriggerConstantly", "Trigger Rate")
SERIALIZATION_ENUM(CEntityAudioSpotComponent::EPlayMode::ReTriggerWhenDone, "ReTriggerWhenDone", "Delay")
SERIALIZATION_ENUM_END()

static void ReflectType(Schematyc::CTypeDesc<CEntityAudioSpotComponent::SAudioTriggerFinishedSignal>& desc)
{
	desc.SetGUID("A16A29CB-8E39-42C0-88C2-33FED1680545"_cry_guid);
	desc.SetLabel("AudioTriggerFinishedSignal");
	desc.AddMember(&CEntityAudioSpotComponent::SAudioTriggerFinishedSignal::m_instanceId, 'inst', "instanceId", "InstanceId", "TriggerId", 0);
	desc.AddMember(&CEntityAudioSpotComponent::SAudioTriggerFinishedSignal::m_triggerId, 'id', "triggerId", "TriggerId", "TriggerId", 0);
	desc.AddMember(&CEntityAudioSpotComponent::SAudioTriggerFinishedSignal::m_bSuccess, 'res', "bSuccess", "Result", "Result", false);
}

void CEntityAudioSpotComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{		
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioSpotComponent::ExecuteTrigger, "0D58AF22-775A-4FBE-BC5C-3A7CE250EF98"_cry_guid, "ExecuteTrigger");
		pFunction->SetDescription("Executes a trigger");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'sta', "StartTrigger");
		pFunction->BindOutput(2, 'inst', "InstanceId");
		pFunction->BindOutput(3, 'id', "TriggerId");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioSpotComponent::StopTrigger, "E4016C26-87E9-4880-8BAE-D8D39E974AFC"_cry_guid, "StopTrigger");
		pFunction->SetDescription("Stops a trigger");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'sto', "StopTrigger");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioSpotComponent::SetParameter, "FBE1DD7C-57C1-46CE-89A1-3612CFD017E4"_cry_guid, "SetParameter");
		pFunction->SetDescription("Sets a parameter to a specific value");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'par', "Parameter");
		pFunction->BindInput(2, 'val', "Value");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioSpotComponent::SetSwitchState, "7ABA1505-527C-4882-9399-716C0E43FFCD"_cry_guid, "SetSwitch");
		pFunction->SetDescription("Sets a switch to a specific state");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'swi', "SwitchAndState");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioSpotComponent::Enable, "8a59ffd5-23e2-4959-9468-6d87c44ed0f5"_cry_guid, "Enable");
		pFunction->SetDescription("Enables/Disables the looping of the default Trigger");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'val', "Value");
		componentScope.Register(pFunction);
	}
	// Signals
	componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(CEntityAudioSpotComponent::SAudioTriggerFinishedSignal));
}

void CEntityAudioSpotComponent::Initialize()
{
	m_pAudioComp = m_pEntity->GetOrCreateComponent<IEntityAudioComponent>();
	CRY_ASSERT(m_pAudioComp);

	if (m_auxAudioObjectId != CryAudio::InvalidAuxObjectId && m_auxAudioObjectId != CryAudio::DefaultAuxObjectId)
	{
		m_pAudioComp->RemoveAudioAuxObject(m_auxAudioObjectId);      //#TODO: for now this is a workaround, because there are scenarios where 'Init' is called twice without a 'Shutdown' in between.
	}

	const Vec3 offset = GetTransformMatrix().GetTranslation();
	m_auxAudioObjectId = m_pAudioComp->CreateAudioAuxObject();
	m_pAudioComp->SetAudioAuxObjectOffset(Matrix34(IDENTITY, offset), m_auxAudioObjectId);
	m_pAudioComp->SetObstructionCalcType(m_occlusionType, m_auxAudioObjectId);
}

void CEntityAudioSpotComponent::OnShutDown()
{
	if (m_pAudioComp && m_auxAudioObjectId != CryAudio::InvalidAuxObjectId && m_auxAudioObjectId != CryAudio::DefaultAuxObjectId)
	{
		m_pAudioComp->RemoveAudioAuxObject(m_auxAudioObjectId);
	}
	m_auxAudioObjectId = CryAudio::InvalidAuxObjectId;
}

uint64 CEntityAudioSpotComponent::GetEventMask() const
{
	return ENTITY_EVENT_BIT(ENTITY_EVENT_RESET) | ENTITY_EVENT_BIT(ENTITY_EVENT_START_GAME) | ENTITY_EVENT_BIT(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED) | ENTITY_EVENT_BIT(ENTITY_EVENT_TIMER) | ENTITY_EVENT_BIT(ENTITY_EVENT_AUDIO_TRIGGER_ENDED);
}

void CEntityAudioSpotComponent::ProcessEvent(SEntityEvent& event)
{
	if (!m_pAudioComp)      //otherwise initialize has not been called yet
		return;

	switch (event.event)
	{
	case ENTITY_EVENT_START_GAME:
		m_bActive = true;
		ExecuteDefaultTrigger();
		break;
	case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
		m_pAudioComp->SetObstructionCalcType(m_occlusionType, m_auxAudioObjectId);
		ExecuteDefaultTrigger();
	case ENTITY_EVENT_RESET:
		if (event.nParam[0] == 0)     //leaving game
		{
			m_bActive = false;
			GetEntity()->KillTimer(m_timerId);
			m_pAudioComp->StopTrigger(m_defaultTrigger.m_triggerId, m_auxAudioObjectId);     //we will still receive a finished callback for this trigger, therefore we have to store (in m_bActive) that we dont want to re-trigger
		}
		break;
	case ENTITY_EVENT_TIMER:
		if (event.nParam[0] == m_timerId)
		{
			ExecuteDefaultTrigger();
		}
		break;
	case ENTITY_EVENT_AUDIO_TRIGGER_ENDED:
		if (m_bActive && m_bEnabled)
		{
			const CryAudio::SRequestInfo* const pAudioCallbackData = reinterpret_cast<const CryAudio::SRequestInfo* const>(event.nParam[0]);
			if (m_pAudioComp->GetAuxObjectIdFromAudioObject(pAudioCallbackData->pAudioObject) == m_auxAudioObjectId)
			{
				uint32 instanceId = (uint32) reinterpret_cast<uintptr_t>(pAudioCallbackData->pUserData);

				Schematyc::IObject* pSchematycObject = GetEntity()->GetSchematycObject();
				if (pSchematycObject)
				{
					pSchematycObject->ProcessSignal(SAudioTriggerFinishedSignal(instanceId, pAudioCallbackData->audioControlId, pAudioCallbackData->requestResult == CryAudio::ERequestResult::Success), GetGUID());
				}
				if (instanceId == 0 && pAudioCallbackData->audioControlId == m_defaultTrigger.m_triggerId && m_playMode == EPlayMode::ReTriggerWhenDone)
				{
					m_timerId = GetEntity()->SetTimer(m_timerId, (int)(1000.0f * cry_random(m_minDelay, m_maxDelay)));
				}
			}
		}
		break;
	}
}

bool CEntityAudioSpotComponent::ExecuteDefaultTrigger()
{
	if (m_bActive && m_bEnabled && m_playMode != EPlayMode::None && m_pAudioComp && m_defaultTrigger.m_triggerId != CryAudio::InvalidControlId)
	{
		if (!m_randomOffset.IsZero())
		{
			Vec3 randomOffset = cry_random(-m_randomOffset, m_randomOffset);
			m_pAudioComp->SetAudioAuxObjectOffset(Matrix34(IDENTITY, randomOffset), m_auxAudioObjectId); //Remark: won't work for 'overlapping' sounds
		}

		CryAudio::SRequestUserData const userData(CryAudio::ERequestFlags::DoneCallbackOnExternalThread | CryAudio::ERequestFlags::CallbackOnExternalOrCallingThread, this);
		if (m_pAudioComp->ExecuteTrigger(m_defaultTrigger.m_triggerId, m_auxAudioObjectId, userData))
		{
			if (m_playMode == EPlayMode::ReTriggerConstantly)
			{
				m_timerId = GetEntity()->SetTimer(m_timerId, (int)(1000.0f * cry_random(m_minDelay, m_maxDelay)));
			}
			return true;
		}
	}
	return false;
}

void CEntityAudioSpotComponent::SetDefaultTrigger(const char* szName)
{
	m_defaultTrigger.m_triggerName = szName;
}

CEntityAudioSpotComponent::SAudioTriggerFinishedSignal::SAudioTriggerFinishedSignal(uint32 instanceId, uint32 triggerId, bool bSuccess)
	: m_instanceId(instanceId)
	, m_triggerId(triggerId)
	, m_bSuccess(bSuccess)
{
}

void SAudioTriggerSerializeHelper::Serialize(Serialization::IArchive& archive)
{
	archive(Serialization::AudioTrigger<string>(m_triggerName), "triggerName", "^Name");

	if (archive.isInput())
	{
		gEnv->pAudioSystem->GetTriggerId(m_triggerName.c_str(), m_triggerId);
	}
}

void SAudioParameterSerializeHelper::Serialize(Serialization::IArchive& archive)
{
	archive(Serialization::AudioRTPC<string>(m_parameterName), "parameter", "^Name");

	if (archive.isInput())
	{
		gEnv->pAudioSystem->GetParameterId(m_parameterName.c_str(), m_parameterId);
	}
}

void SAudioSwitchWithStateSerializeHelper::Serialize(Serialization::IArchive& archive)
{
	archive(Serialization::AudioSwitch<string>(m_switchName), "switchName", "SwitchName");
	archive(Serialization::AudioSwitchState<string>(m_switchStateName), "stateName", "StateName");

	if (archive.isInput())
	{
		gEnv->pAudioSystem->GetSwitchId(m_switchName.c_str(), m_switchId);
		gEnv->pAudioSystem->GetSwitchStateId(m_switchId, m_switchStateName.c_str(), m_switchStateId);
	}
}
} //DefaultComponents
} //Cry
