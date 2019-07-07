// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryBrofiler.h"

#if ALLOW_BROFILER
#include "Cry_Brofiler.h"
#include <CryThreading/IThreadManager.h>

thread_local Profiler::ThreadDescription* tls_pDescription = nullptr;

void CBrofiler_OnStartThread()
{
	CRY_ASSERT(tls_pDescription == nullptr);
	tls_pDescription = new Profiler::ThreadDescription(gEnv->pThreadManager->GetThreadName(CryGetCurrentThreadId()));
}

void CBrofiler_OnEndThread()
{
	CRY_ASSERT(tls_pDescription != nullptr);
	delete tls_pDescription;
}

REGISTER_PROFILER(CBrofiler, "Brofiler", "-brofiler", &CBrofiler_OnStartThread, &CBrofiler_OnEndThread);

void CBrofiler::DescriptionCreated(SProfilingDescription* pDesc)
{
	CCryProfilingSystemImpl::DescriptionCreated(pDesc);
	pDesc->customData = (uintptr_t) Profiler::EventDescription::Create(pDesc->szEventname, pDesc->szFilename, pDesc->line, GenerateColorBasedOnName(pDesc->szEventname));
}

void CBrofiler::DescriptionDestroyed(SProfilingDescription* pDesc)
{
	CCryProfilingSystemImpl::DescriptionDestroyed(pDesc);
	Profiler::EventDescription::DestroyEventDescription((Profiler::EventDescription*) pDesc->customData);
	pDesc->customData = 0;
}

SSystemGlobalEnvironment::TProfilerSectionEndCallback CBrofiler::StartSectionStatic(SProfilingSection* pSection)
{
	pSection->customData = (uintptr_t)Profiler::Event::Start(*(Profiler::EventDescription*) pSection->pDescription->customData);
	return &EndSectionStatic;
}

void CBrofiler::EndSectionStatic(SProfilingSection* pSection)
{
	if (pSection->customData)
		Profiler::Event::Stop(*(Profiler::EventData*) pSection->customData);
}

#endif