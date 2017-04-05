// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionWait.h"
#include "ResponseSystem.h"

#include <CryString/StringUtils.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

using namespace CryDRS;
//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionWait::Execute(DRS::IResponseInstance* pResponseInstance)
{
	return DRS::IResponseActionInstanceUniquePtr(new CActionWaitInstance(m_timeToWait));
}

//--------------------------------------------------------------------------------------------------
string CActionWait::GetVerboseInfo() const
{
	return CryStringUtils::toString(m_timeToWait) + "s";
}

//--------------------------------------------------------------------------------------------------
void CActionWait::Serialize(Serialization::IArchive& ar)
{
	ar(m_timeToWait, "Time", "^Time (in seconds)");
}

//--------------------------------------------------------------------------------------------------
CActionWaitInstance::CActionWaitInstance(float timeToWait)
{
	m_finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + timeToWait;
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionWaitInstance::Update()
{
	if (CResponseSystem::GetInstance()->GetCurrentDrsTime() > m_finishTime)
	{
		return DRS::IResponseActionInstance::CS_FINISHED;
	}
	return DRS::IResponseActionInstance::CS_RUNNING;
}
