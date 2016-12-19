// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ActionExecuteAudioTrigger.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

using namespace CryAudio;

static const char* ActionPlaySoundId = "ExecuteAudioTriggerAction";

DRS::IResponseActionInstanceUniquePtr CActionExecuteAudioTrigger::Execute(DRS::IResponseInstance* pResponseInstance)
{
	ControlId audioStartTriggerID = InvalidControlId;
	if (gEnv->pAudioSystem->GetAudioTriggerId(m_AudioTriggerName.c_str(), audioStartTriggerID))
	{
		IEntity* pEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();
		if (pEntity)
		{
			IEntityAudioComponent* pEntityAudioProxy = pEntity->GetOrCreateComponent<IEntityAudioComponent>();

			DRS::IResponseActionInstanceUniquePtr pActionInstance(new CActionExecuteAudioTriggerInstance(pEntityAudioProxy, audioStartTriggerID));

			if (m_bWaitToBeFinished)
			{
				SRequestUserData const userData(eRequestFlags_PriorityNormal | eRequestFlags_SyncFinishedCallback, (void* const)pActionInstance.get(), (void* const)ActionPlaySoundId, (void* const)pActionInstance.get());
				if (pEntityAudioProxy->ExecuteTrigger(audioStartTriggerID, DefaultAuxObjectId, userData))
				{
					return pActionInstance;
				}
			}
			else
			{
				pEntityAudioProxy->ExecuteTrigger(audioStartTriggerID);
			}
		}
	}

	return nullptr;
}

//--------------------------------------------------------------------------------------------------
string CActionExecuteAudioTrigger::GetVerboseInfo() const
{
	return string("'") + m_AudioTriggerName + "'";
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteAudioTrigger::Serialize(Serialization::IArchive& ar)
{
	ar(m_AudioTriggerName, "AudioTriggerName", "^ TriggerName");
	ar(m_bWaitToBeFinished, "WaitForFinish", "^ WaitForTriggerToFinish");
}

//--------------------------------------------------------------------------------------------------
CActionExecuteAudioTriggerInstance::CActionExecuteAudioTriggerInstance(IEntityAudioComponent* pAudioProxy, ControlId audioStartTriggerID)
	: m_pEntityAudioProxy(pAudioProxy)
	, m_audioStartTriggerID(audioStartTriggerID)
{
	gEnv->pAudioSystem->AddRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this, eSystemEvent_TriggerExecuted | eSystemEvent_TriggerFinished);
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
void CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished(const SRequestInfo* const pInfo)
{
	if (pInfo->pUserData == ActionPlaySoundId)
	{
		if (pInfo->requestResult == eRequestResult_Failure &&
		    (pInfo->audioSystemEvent == eSystemEvent_TriggerExecuted ||
		     pInfo->audioSystemEvent == eSystemEvent_TriggerFinished))
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
