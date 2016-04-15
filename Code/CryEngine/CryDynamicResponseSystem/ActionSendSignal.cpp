// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ResponseInstance.h"
#include "ResponseManager.h"
#include "VariableCollection.h"
#include "ActionSendSignal.h"
#include "ResponseSystem.h"
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionSendSignal::Execute(DRS::IResponseInstance* pResponseInstance)
{
	pResponseInstance->GetCurrentActor()->QueueSignal(m_signalName, (m_bCopyContextVariables) ? pResponseInstance->GetContextVariables() : nullptr);

	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CActionSendSignal::Serialize(Serialization::IArchive& ar)
{
	ar(m_signalName, "signal", "^ Signal");
#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	if (ar.isEdit() && !m_signalName.IsValid())
	{
		ar.warning(m_signalName.m_textCopy, "No signal to send specified");
	}
#endif
	ar(m_bCopyContextVariables, "copyContextVar", "^Copy Context Variable");
}

//--------------------------------------------------------------------------------------------------

REGISTER_DRS_ACTION(CActionSendSignal, "SendSignal", DEFAULT_DRS_ACTION_COLOR);
