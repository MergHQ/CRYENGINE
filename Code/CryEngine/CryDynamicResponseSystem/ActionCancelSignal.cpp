// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionCancelSignal.h"

#include "ResponseInstance.h"
#include "ResponseSystem.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionCancelSignal::Execute(DRS::IResponseInstance* pResponseInstance)
{
	static const CHashedString s_allKeyWord = "All";
	CResponseInstance* pInstance = static_cast<CResponseInstance*>(pResponseInstance);
	const CHashedString& usedSignal = (m_signalName.IsValid()) ? m_signalName : pInstance->GetSignalName();

	if (m_signalName == s_allKeyWord && m_bOnAllActors)  //special case: stop all signals on all actors -> reset
	{
		CResponseSystem::GetInstance()->Reset(DRS::IDynamicResponseSystem::eResetHint_StopRunningResponses);
		return nullptr;
	}
	else
	{
		const bool bSomethingCanceled = CResponseSystem::GetInstance()->CancelSignalProcessing(usedSignal, (m_bOnAllActors) ? nullptr : pInstance->GetCurrentActor(), pInstance->GetSignalInstanceId());
		if (!bSomethingCanceled || pInstance->GetCurrentSegment() == nullptr)
		{
			return nullptr;  //seems like we canceled our own response-instance, in this case we don`t start a new instance here.
		}
		return DRS::IResponseActionInstanceUniquePtr(new CActionCancelSignalInstance(usedSignal, (m_bOnAllActors) ? nullptr : pInstance->GetCurrentActor(), pInstance->GetSignalInstanceId()));
	}
}

//--------------------------------------------------------------------------------------------------
void CActionCancelSignal::Serialize(Serialization::IArchive& ar)
{
	ar(m_signalName, "signal", "^ Signal");
	ar(m_bOnAllActors, "onAllActors", "^OnAllActors");

#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	if (ar.isEdit())
	{
		if (!m_signalName.IsValid())
		{
			ar.warning(m_signalName.m_textCopy, "No signal to cancel specified");
		}
		else if (CResponseSystem::GetInstance()->GetResponseManager()->GetResponse(m_signalName) == nullptr)
		{
			ar.warning(m_signalName.m_textCopy, "No response exists for the specified signal");
		}
	}
#endif
}

REGISTER_DRS_ACTION(CActionCancelSignal, "CancelSignal", DEFAULT_DRS_ACTION_COLOR);

//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
void CryDRS::CActionCancelSignalInstance::Cancel()
{
	m_signal.m_signalName.Clear();
	m_signal.m_pSender = nullptr;
}

//--------------------------------------------------------------------------------------------------
CryDRS::CActionCancelSignalInstance::CActionCancelSignalInstance(const CHashedString& signalName, CResponseActor* pSender /*= nullptr*/, DRS::SignalInstanceId instanceToSkip /*= DRS::s_InvalidSignalId*/)
	: m_signal(signalName, pSender, nullptr)
{
	m_signal.m_id = instanceToSkip;
}
