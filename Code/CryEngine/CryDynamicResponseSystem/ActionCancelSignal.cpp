// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionCancelSignal.h"

#include "ResponseInstance.h"
#include "ResponseSystem.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionCancelSignal::Execute(DRS::IResponseInstance* pResponseInstance)
{
	CResponseInstance* pInstance = static_cast<CResponseInstance*>(pResponseInstance);
	const CHashedString& usedSignal = (m_signalName.IsValid()) ? m_signalName : pInstance->GetSignalName();

	if (!usedSignal.IsValid() && m_bOnAllActors)  //special case: stop all signals on all actors -> reset
	{
		CResponseSystem::GetInstance()->Reset(DRS::IDynamicResponseSystem::eResetHint_StopRunningResponses);
		return nullptr;
	}
	else
	{
		CResponseSystem::GetInstance()->CancelSignalProcessing(usedSignal, (m_bOnAllActors) ? nullptr : pInstance->GetCurrentActor(), pInstance->GetSignalInstanceId());
		return DRS::IResponseActionInstanceUniquePtr(new CActionCancelSignalInstance(usedSignal, (m_bOnAllActors) ? nullptr : pInstance->GetCurrentActor(), pInstance->GetSignalInstanceId()));
	}
}

//--------------------------------------------------------------------------------------------------
void CActionCancelSignal::Serialize(Serialization::IArchive& ar)
{
	ar(m_signalName, "signal", "^ Signal");
	ar(m_bOnAllActors, "OnAllActors", "^OnAllActors");
}

REGISTER_DRS_ACTION(CActionCancelSignal, "CancelSignal", DEFAULT_DRS_ACTION_COLOR);

DRS::IResponseActionInstance::eCurrentState CryDRS::CActionCancelSignalInstance::Update()
{
	if (m_signal.m_signalName.IsValid() || m_signal.m_pSender)
	{
		if (CResponseSystem::GetInstance()->GetResponseManager()->IsSignalProcessed(m_signal))
		{
			return IResponseActionInstance::CS_RUNNING;
		}
	}
	return IResponseActionInstance::CS_FINISHED;
}

void CryDRS::CActionCancelSignalInstance::Cancel()
{
	m_signal.m_signalName.Clear();
	m_signal.m_pSender = nullptr;
}

CryDRS::CActionCancelSignalInstance::CActionCancelSignalInstance(const CHashedString& signalName, CResponseActor* pSender /*= nullptr*/, DRS::SignalInstanceId instanceToSkip /*= DRS::s_InvalidSignalId*/)
	: m_signal(signalName, pSender, nullptr)
{
	m_signal.m_id = instanceToSkip;
}
