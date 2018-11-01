// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "TriggerConnection.h"
#include "Common.h"
#include "Common/IEvent.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CTriggerConnection::~CTriggerConnection()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->DestructTrigger(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTriggerConnection::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	return pImplObject->ExecuteTrigger(m_pImplData, pImplEvent);
}
} // namespace CryAudio
