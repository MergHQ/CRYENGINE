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
		DRS::SignalInstanceId id = pResponseInstance->GetCurrentActor()->QueueSignal(m_responseID, pResponseInstance->GetContextVariables(), pInstance);
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
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CActionExecuteResponseInstance::~CActionExecuteResponseInstance()
{
	CResponseSystem::GetInstance()->GetResponseManager()->RemoveListener(this);
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionExecuteResponseInstance::Update()
{
	return m_state;
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteResponseInstance::Cancel()
{
	if (m_state == CS_RUNNING)
	{
		if (m_pStartedResponse)
		{
			m_pStartedResponse->Cancel();
		}
		m_state = CS_CANCELED;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionExecuteResponseInstance::OnSignalProcessingFinished(SSignalInfos& signal, DRS::IResponseInstance* pFinishedResponse, eProcessingResult outcome)
{
	m_state = CS_FINISHED;
}

//--------------------------------------------------------------------------------------------------
void CryDRS::CActionExecuteResponseInstance::OnSignalProcessingStarted(SSignalInfos& signal, DRS::IResponseInstance* pStartedResponse)
{
	m_pStartedResponse = static_cast<CResponseInstance*>(pStartedResponse);
}


REGISTER_DRS_ACTION(CActionExecuteResponse, "ExecuteResponse", DEFAULT_DRS_ACTION_COLOR);
