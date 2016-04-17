// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionExecuteResponse.h"
#include "ResponseInstance.h"
#include "ResponseManager.h"

using namespace CryDRS;

DRS::IResponseActionInstanceUniquePtr CActionExecuteResponse::Execute(DRS::IResponseInstance* pResponseInstance)
{
	if (m_responseID.IsValid())
	{
		CActionExecuteResponseInstance* pInstance = new CActionExecuteResponseInstance();
		DRS::SignalId id = pResponseInstance->GetCurrentActor()->QueueSignal(m_responseID, pResponseInstance->GetContextVariables(), pInstance);
		return DRS::IResponseActionInstanceUniquePtr(pInstance);
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteResponse::Serialize(Serialization::IArchive& ar)
{
	ar(m_responseID, "response", "^ Response");
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionExecuteResponseInstance::Update()
{
	if (m_state == CActionExecuteResponseInstance::eCurrentState_WaitingForResponseToFinish)
	{
		return IResponseActionInstance::CS_RUNNING;
	}
	return IResponseActionInstance::CS_FINISHED;
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteResponseInstance::Cancel()
{
	if (m_state == eCurrentState_WaitingForResponseToFinish)
	{
		CResponseSystem::GetInstance()->GetResponseManager()->RemoveListener(this);
		m_state = eCurrentState_Canceled;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteResponseInstance::OnSignalProcessingFinished(DRS::IResponseManager::IListener::SSignalInfos& signal, DRS::IResponseInstance* pFinishedResponse, DRS::IResponseManager::IListener::eProcessingResult outcome)
{
	m_state = eCurrentState_Done;
}

//--------------------------------------------------------------------------------------------------
CActionExecuteResponseInstance::~CActionExecuteResponseInstance()
{
	if (m_state == eCurrentState_WaitingForResponseToFinish)
	{
		CResponseSystem::GetInstance()->GetResponseManager()->RemoveListener(this);
	}
}

REGISTER_DRS_ACTION(CActionExecuteResponse, "ExecuteResponse", DEFAULT_DRS_ACTION_COLOR);
