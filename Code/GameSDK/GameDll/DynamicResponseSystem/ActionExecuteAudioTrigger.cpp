// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ActionExecuteAudioTrigger.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

static const char* ActionPlaySoundId = "ExecuteAudioTriggerAction";

DRS::IResponseActionInstanceUniquePtr CActionExecuteAudioTrigger::Execute(DRS::IResponseInstance* pResponseInstance)
{
	AudioControlId audioStartTriggerID = INVALID_AUDIO_CONTROL_ID;
	if (gEnv->pAudioSystem->GetAudioTriggerId(m_AudioTriggerName.c_str(), audioStartTriggerID))
	{
		IEntity* pEntity = pResponseInstance->GetCurrentActor()->GetLinkedEntity();
		if (pEntity)
		{
			const IEntityAudioProxyPtr pEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

			DRS::IResponseActionInstanceUniquePtr pActionInstance(new CActionExecuteAudioTriggerInstance(pEntityAudioProxy, audioStartTriggerID));

			if (m_bWaitToBeFinished)
			{
				SAudioCallBackInfo const callbackInfo((void* const)pActionInstance.get(), (void* const)ActionPlaySoundId, (void* const)pActionInstance.get(), eAudioRequestFlags_PriorityNormal | eAudioRequestFlags_SyncFinishedCallback);
				if (pEntityAudioProxy->ExecuteTrigger(audioStartTriggerID, DEFAULT_AUDIO_PROXY_ID, callbackInfo))
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
CActionExecuteAudioTriggerInstance::CActionExecuteAudioTriggerInstance(IEntityAudioProxyPtr pAudioProxy, AudioControlId audioStartTriggerID)
	: m_pEntityAudioProxy(pAudioProxy)
	, m_audioStartTriggerID(audioStartTriggerID)
{
	gEnv->pAudioSystem->AddRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance);
	gEnv->pAudioSystem->AddRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this, eAudioRequestType_AudioObjectRequest, eAudioObjectRequestType_ExecuteTrigger);
}

//--------------------------------------------------------------------------------------------------
CActionExecuteAudioTriggerInstance::~CActionExecuteAudioTriggerInstance()
{
	gEnv->pAudioSystem->RemoveRequestListener(&CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished, this);
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
void CActionExecuteAudioTriggerInstance::OnAudioTriggerFinished(const SAudioRequestInfo* const pInfo)
{
	if (pInfo->pUserData == ActionPlaySoundId)
	{
		if ((pInfo->audioRequestType == eAudioRequestType_AudioObjectRequest && pInfo->specificAudioRequest == eAudioObjectRequestType_ExecuteTrigger && pInfo->requestResult == eAudioRequestResult_Failure)
		    || (pInfo->audioRequestType == eAudioRequestType_AudioCallbackManagerRequest && pInfo->specificAudioRequest == eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance))
		{
			CRY_ASSERT(pInfo->pUserDataOwner);
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
