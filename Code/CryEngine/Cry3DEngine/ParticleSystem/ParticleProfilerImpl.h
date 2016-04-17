// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

namespace pfx2
{

#if !defined(_RELEASE)

ILINE void CParticleProfiler::AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, uint value)
{
	CRY_PFX2_ASSERT(uint(type) > EPST_Int && uint(type) < EPST_Float);
	const uint32 threadId = JobManager::GetWorkerThreadId();
	SEntry entry;
	entry.m_pRuntime = pRuntime;
	entry.m_type = type;
	entry.m_value.m_int = value;
	m_entries[threadId + 1].push_back(entry);
}

ILINE void CParticleProfiler::AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, float value)
{
	CRY_PFX2_ASSERT(uint(type) > EPST_Float && uint(type) < EPST_Count);
	const uint32 threadId = JobManager::GetWorkerThreadId();
	SEntry entry;
	entry.m_pRuntime = pRuntime;
	entry.m_type = type;
	entry.m_value.m_float = value;
	m_entries[threadId + 1].push_back(entry);
}

ILINE CTimeProfiler::CTimeProfiler(CParticleProfiler& profiler, CParticleComponentRuntime* pRuntime, EProfileStat stat)
	: m_profiler(profiler)
	, m_pRuntime(pRuntime)
	, m_stat(stat)
	, m_startTicks(CryGetTicks())
{
}

ILINE CTimeProfiler::~CTimeProfiler()
{
	int64 endTicks = CryGetTicks();
	m_profiler.AddEntry(
	  m_pRuntime, m_stat,
	  gEnv->pTimer->TicksToSeconds(endTicks - m_startTicks));
}

#else

ILINE void CParticleProfiler::AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, uint value)  {}
ILINE void CParticleProfiler::AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, float value) {}
ILINE CTimeProfiler::CTimeProfiler(CParticleProfiler& profiler, CParticleComponentRuntime* pRuntime, EProfileStat stat) : m_profiler(profiler) {}
ILINE CTimeProfiler::~CTimeProfiler() {}

#endif

}
