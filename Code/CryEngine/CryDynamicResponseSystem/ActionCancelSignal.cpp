// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionCancelSignal.h"

#include "ResponseInstance.h"
#include "ResponseSystem.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionCancelSignal::Execute(DRS::IResponseInstance* pResponseInstance)
{
	const CHashedString& usedSignal = (m_signalName.IsValid()) ? m_signalName : pResponseInstance->GetSignalName();

	CResponseSystem::GetInstance()->CancelSignalProcessing(usedSignal, (m_bOnAllActors) ? nullptr : pResponseInstance->GetCurrentActor());

	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CActionCancelSignal::Serialize(Serialization::IArchive& ar)
{
	ar(m_signalName, "signal", "^ Signal");
	ar(m_bOnAllActors, "OnAllActors", "^OnAllActors");
#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	if (ar.isEdit() && !m_signalName.IsValid())
	{
		ar.warning(m_signalName.m_textCopy, "No signal to cancel specified");
	}
#endif
}

REGISTER_DRS_ACTION(CActionCancelSignal, "CancelSignal", DEFAULT_DRS_ACTION_COLOR);
