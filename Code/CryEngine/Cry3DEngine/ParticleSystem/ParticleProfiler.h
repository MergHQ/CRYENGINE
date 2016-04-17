// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleCommon.h"

namespace pfx2
{

class CParticleProfiler;
class CParticleComponentRuntime;

enum EProfileStat
{
	EPST_Int,    // integer entries
	EPS_Jobs,
	EPS_AllocatedParticles,
	EPS_ActiveParticles,
	EPS_RendereredParticles,

	EPST_Float,  // float entries
	EPS_NewBornTime,
	EPS_UpdateTime,
	EPS_ComputeVerticesTime,

	EPST_Count,
};

class CTimeProfiler
{
public:
	CTimeProfiler(CParticleProfiler& profiler, CParticleComponentRuntime* pRuntime, EProfileStat stat);
	~CTimeProfiler();

private:
	CParticleProfiler&         m_profiler;
	CParticleComponentRuntime* m_pRuntime;
	int64                      m_startTicks;
	EProfileStat               m_stat;
};

struct IStatOutput;

class CParticleProfiler
{
public:
	union SValue
	{
		float m_float;
		uint  m_int;
	};

	struct SStatDisplay
	{
		float        m_offsetX;
		EProfileStat m_stat;
		const char*  m_statName;
	};

	struct SStatistics
	{
		SStatistics();
		SValue m_values[EPST_Count];
	};

	struct SEntry
	{
		CParticleComponentRuntime* m_pRuntime;
		EProfileStat               m_type;
		SValue                     m_value;
	};
	typedef std::vector<SEntry> TEntries;

public:
	CParticleProfiler();

	void Reset();
	void Display();
	void SaveToFile(cstr fileName = nullptr);

	void AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, uint value = 1);
	void AddEntry(CParticleComponentRuntime* pRuntime, EProfileStat type, float value);

private:
	void SortEntries();
	void WriteEntries(IStatOutput& output) const;
	void WriteStats(IStatOutput& output, cstr emitterName, cstr componentName, const SStatistics& stats) const;
	void WriteStat(IStatOutput& output, SStatDisplay stat, SValue value) const;

	std::vector<TEntries> m_entries;
};

}

#include "ParticleProfilerImpl.h"
