// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ActionExecuteAudioTrigger.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CrySerialization/Decorators/ResourcesAudio.h>

static const char* ActionPlaySoundId = "ExecuteAudioTriggerAction";

DRS::IResponseActionInstanceUniquePtr CActionExecuteAudioTrigger::Execute(DRS::IResponseInstance* pResponseInstance)
{
	CryAudio::ControlId audioStartTriggerID;
	if (gEnv->pAudioSystem->GetAudioTriggerId(m_AudioTriggerName.c_str(), audioStartTriggerID))
	{
		IEntity* pEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();
		if (pEntity)
		{
			IEntityAudioComponent* pEntityAudioProxy = pEntity->GetOrCreateComponent<IEntityAudioComponent>();

			CryAudio::AuxObjectId auxProxyId = pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID();
			if (auxProxyId == CryAudio::InvalidAuxObjectId)
				auxProxyId = CryAudio::DefaultAuxObjectId;

			if (m_bWaitToBeFinished)
			{
				DRS::IResponseActionInstanceUniquePtr pActionInstance(new CActionExecuteAudioTriggerInstance(pEntityAudioProxy, audioStartTriggerID));
				CryAudio::SRequestUserData const userData(CryAudio::eRequestFlags_DoneCallbackOnExternalThread, (void* const)pActionInstance.get(), (void* const)ActionPlaySoundId, (void* const)pActionInstance.get());

				if (pEntityAudioProxy->ExecuteTrigger(audioStartTriggerID, auxProxyId, userData))
				{
					return pActionInstance;
				}
			}
			else  //Fire and forget sound, no need to listen to the 'finished'-callback
			{
				pEntityAudioProxy->ExecuteTrigger(audioStartTriggerID, auxProxyId);
			}
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
string CActionExecuteAudioTrigger::GetVerboseInfo() const
{
	return string().Format("'%s'", m_AudioTriggerName.c_str());
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTrigger::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::AudioTrigger(m_AudioTriggerName), "audioTriggerName", "^ TriggerName");
	ar(m_bWaitToBeFinished, "WaitForFinish", "^ WaitForTriggerToFinish");
}

//--------------------------------------------------------------------------------------------------
CActionExecuteAudioTriggerInstance::CActionExecuteAudioTriggerInstance(IEntityAudioComponent* pAudioProxy, CryAudio::ControlId audioStartTriggerID)
	: m_pEntityAudioProxy(pAudioProxy)
	, m_audioStartTriggerID(audioStartTriggerID)
{
	gEnv->pAudioSystem->AddRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this, CryAudio::eSystemEvent_TriggerExecuted | CryAudio::eSystemEvent_TriggerFinished);
}

//--------------------------------------------------------------------------------------------------
CActionExecuteAudioTriggerInstance::~CActionExecuteAudioTriggerInstance()
{
	gEnv->pAudioSystem->RemoveRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this);
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionExecuteAudioTriggerInstance::Update()
{
	if (m_pEntityAudioProxy)
	{
		return DRS::IResponseActionInstance::CS_RUNNING;
	}
	else
	{
		return DRS::IResponseActionInstance::CS_FINISHED;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished(const CryAudio::SRequestInfo* const pInfo)
{
	if (pInfo->pUserData == ActionPlaySoundId)
	{
		if (pInfo->requestResult == CryAudio::eRequestResult_Failure &&
			(pInfo->audioSystemEvent == CryAudio::eSystemEvent_TriggerExecuted ||
				pInfo->audioSystemEvent == CryAudio::eSystemEvent_TriggerFinished))
		{
			CRY_ASSERT(pInfo->pUserDataOwner != nullptr);
			CActionExecuteAudioTriggerInstance* pEndedInstance = reinterpret_cast<CActionExecuteAudioTriggerInstance*>(pInfo->pUserDataOwner);
			pEndedInstance->SetFinished();
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTriggerInstance::Cancel()
{
	if (m_pEntityAudioProxy)
	{
		m_pEntityAudioProxy->StopTrigger(m_audioStartTriggerID);
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTriggerInstance::SetFinished()
{
	m_pEntityAudioProxy = nullptr;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

DRS::IResponseActionInstanceUniquePtr CActionSetAudioSwitch::Execute(DRS::IResponseInstance* pResponseInstance)
{
	CryAudio::ControlId switchID;
	if (gEnv->pAudioSystem->GetAudioSwitchId(m_switchName.c_str(), switchID))
	{
		CryAudio::SwitchStateId switchStateID;
		if (gEnv->pAudioSystem->GetAudioSwitchStateId(switchID, m_stateName.c_str(), switchStateID))
		{
			IEntity* pEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();
			if (pEntity)
			{
				IEntityAudioComponent* pEntityAudioProxy = pEntity->GetOrCreateComponent<IEntityAudioComponent>();
				pEntityAudioProxy->SetSwitchState(switchID, switchStateID, pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID());
			}
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
string CActionSetAudioSwitch::GetVerboseInfo() const
{
	return string().Format(" '%s' to state '%s'", m_switchName.c_str(), m_stateName.c_str());
}

//--------------------------------------------------------------------------------------------------
void CActionSetAudioSwitch::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::AudioSwitch(m_switchName), "switch", "^ Switch");
	ar(Serialization::AudioSwitchState(m_stateName), "switchState", "^ SwitchState");
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

DRS::IResponseActionInstanceUniquePtr CActionSetAudioParameter::Execute(DRS::IResponseInstance* pResponseInstance)
{
	CryAudio::ControlId parameterId;
	if (gEnv->pAudioSystem->GetAudioParameterId(m_audioParameter.c_str(), parameterId))
	{
		IEntity* pEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();
		if (pEntity)
		{
			IEntityAudioComponent* pEntityAudioProxy = pEntity->GetOrCreateComponent<IEntityAudioComponent>();
			pEntityAudioProxy->SetParameter(parameterId, m_valueToSet, pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID());
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
string CActionSetAudioParameter::GetVerboseInfo() const
{
	return string().Format(" '%s' to value '%f'", m_audioParameter.c_str(), m_valueToSet);
}

//--------------------------------------------------------------------------------------------------
void CActionSetAudioParameter::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::AudioRTPC(m_audioParameter), "audioParameter", "^ AudioParameter");
	ar(m_valueToSet, "value", "^ Value");
}

