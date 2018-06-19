// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FallbackBackend.h
//  Version:     v1.00
//  Created:     07/05/2011 by Christopher Bolte
//  Compilers:   Visual Studio.NET
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "FallBackBackend.h"
#include "../JobManager.h"

JobManager::FallBackBackEnd::CFallBackBackEnd::CFallBackBackEnd()
{

}

JobManager::FallBackBackEnd::CFallBackBackEnd::~CFallBackBackEnd()
{

}

void JobManager::FallBackBackEnd::CFallBackBackEnd::AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock)
{
	CJobManager* __restrict pJobManager = CJobManager::Instance();

	Invoker delegator = crJob.GetGenericDelegator();
	const void* pParamMem = crJob.GetJobParamData();

	// execute job function
	if (crJob.GetLambda())
	{
		crJob.GetLambda()();
	}
	else
	{
		(*delegator)((void*)pParamMem);
	}

	IF (rInfoBlock.GetJobState(), 1)
	{
		SJobState* pJobState = rInfoBlock.GetJobState();
		pJobState->SetStopped();
	}
}
