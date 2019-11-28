// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ActionExecuteAudioTrigger.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CrySerialization/Decorators/ResourcesAudio.h>

static const char* ActionPlaySoundId = "ExecuteAudioTriggerAction";

Cry::DRS::IResponseActionInstanceUniquePtr CActionExecuteAudioTrigger::Execute(Cry::DRS::IResponseInstance* pResponseInstance)
{
	Cry::Audio::ControlId const startTriggerId = Cry::Audio::StringToId(m_AudioTriggerName.c_str());
	IEntity* const pIEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();

	if (pIEntity != nullptr)
	{
		Cry::Audio::AuxObjectId auxObjectId = pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID();

		if (auxObjectId == Cry::Audio::InvalidAuxObjectId)
		{
			auxObjectId = Cry::Audio::DefaultAuxObjectId;
		}

		IEntityAudioComponent* const pEntityAudioProxy = pIEntity->GetOrCreateComponent<IEntityAudioComponent>();

		if (m_bWaitToBeFinished)
		{
			Cry::DRS::IResponseActionInstanceUniquePtr pActionInstance(new CActionExecuteAudioTriggerInstance(pResponseInstance->GetCurrentActor(), startTriggerId));
			Cry::Audio::SRequestUserData const userData(Cry::Audio::ERequestFlags::SubsequentCallbackOnExternalThread, (void* const)pActionInstance.get(), (void* const)ActionPlaySoundId, (void* const)pActionInstance.get());

			if (pEntityAudioProxy->ExecuteTrigger(startTriggerId, auxObjectId, INVALID_ENTITYID, userData))
			{
				return pActionInstance;
			}
		}
		else  //Fire and forget sound, no need to listen to the 'finished'-callback
		{
			pEntityAudioProxy->ExecuteTrigger(startTriggerId, auxObjectId);
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
CActionExecuteAudioTriggerInstance::CActionExecuteAudioTriggerInstance(Cry::DRS::IResponseActor* pActor, Cry::Audio::ControlId audioStartTriggerID)
	: m_pActor(pActor)
	, m_audioStartTriggerID(audioStartTriggerID)
{
	gEnv->pAudioSystem->AddRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this, Cry::Audio::ESystemEvents::TriggerExecuted | Cry::Audio::ESystemEvents::TriggerFinished);
}

//--------------------------------------------------------------------------------------------------
CActionExecuteAudioTriggerInstance::~CActionExecuteAudioTriggerInstance()
{
	gEnv->pAudioSystem->RemoveRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this);
}

//--------------------------------------------------------------------------------------------------
Cry::DRS::IResponseActionInstance::eCurrentState CActionExecuteAudioTriggerInstance::Update()
{
	if (m_pActor)
	{
		return Cry::DRS::IResponseActionInstance::CS_RUNNING;
	}
	else
	{
		return Cry::DRS::IResponseActionInstance::CS_FINISHED;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished(const Cry::Audio::SRequestInfo* const pInfo)
{
	if (pInfo->pUserData == ActionPlaySoundId)
	{
		if (pInfo->requestResult == Cry::Audio::ERequestResult::Failure &&
		    (pInfo->systemEvent == Cry::Audio::ESystemEvents::TriggerExecuted ||
		     pInfo->systemEvent == Cry::Audio::ESystemEvents::TriggerFinished))
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
	if (m_pActor)
	{
		if (IEntity* pEntity = m_pActor->GetLinkedEntity())
		{
			if (IEntityAudioComponent* pEntityAudioProxy = pEntity->GetComponent<IEntityAudioComponent>())  //we refetch the audio component here, because during entity-destruction it is not guaranteed that the DRS component is deleted first.
			{
				pEntityAudioProxy->StopTrigger(m_audioStartTriggerID);
				return;
			}
		}
	}
	m_pActor = nullptr;  //we failed to stop the trigger, therefore we hard-cancel this action instance
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTriggerInstance::SetFinished()
{
	m_pActor = nullptr;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

Cry::DRS::IResponseActionInstanceUniquePtr CActionSetAudioSwitch::Execute(Cry::DRS::IResponseInstance* pResponseInstance)
{
	IEntity* const pIEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();

	if (pIEntity != nullptr)
	{
		Cry::Audio::ControlId const switchID = Cry::Audio::StringToId(m_switchName.c_str());
		Cry::Audio::ControlId const switchStateID = Cry::Audio::StringToId(m_stateName.c_str());
		IEntityAudioComponent* const pEntityAudioProxy = pIEntity->GetOrCreateComponent<IEntityAudioComponent>();
		pEntityAudioProxy->SetSwitchState(switchID, switchStateID, pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID());
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
	ar(Serialization::AudioState(m_stateName), "switchState", "^ SwitchState");
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

Cry::DRS::IResponseActionInstanceUniquePtr CActionSetAudioParameter::Execute(Cry::DRS::IResponseInstance* pResponseInstance)
{
	IEntity* const pIEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();

	if (pIEntity != nullptr)
	{
		Cry::Audio::ControlId const parameterId = Cry::Audio::StringToId(m_audioParameter.c_str());
		IEntityAudioComponent* const pEntityAudioProxy = pIEntity->GetOrCreateComponent<IEntityAudioComponent>();
		pEntityAudioProxy->SetParameter(parameterId, m_valueToSet, pResponseInstance->GetCurrentActor()->GetAuxAudioObjectID());
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
	ar(Serialization::AudioParameter(m_audioParameter), "audioParameter", "^ AudioParameter");
	ar(m_valueToSet, "value", "^ Value");
}
