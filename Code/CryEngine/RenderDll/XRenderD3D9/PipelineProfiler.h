// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
* Created by Nicolas Schulz, February 2012

   =============================================================================*/

#pragma once

#ifndef _PIPELINEPROFILER_H
	#define _PIPELINEPROFILER_H

	#include "GPUTimer.h"

struct SStaticElementInfo
{
	SStaticElementInfo(uint _nPos) : nPos(_nPos), bUsed(false) {}
	uint nPos;
	bool bUsed;
};

struct RPProfilerSection
{
	char            name[31];
	int8            recLevel;   // Negative value means error in stack
	CCryNameTSCRC   path;
	uint32          numDIPs, numPolys;
	CTimeValue      startTimeCPU, endTimeCPU;
	CSimpleGPUTimer gpuTimer;
};

struct RPPSectionsFrame
{
	// Note: Use of m_sections is guarded by m_numSections, initialization of m_sections skipped for performance reasons
	// cppcheck-suppress uninitMemberVar
	RPPSectionsFrame() : m_numSections(0) {}

	static const uint32 MaxNumSections = 256;
	RPProfilerSection   m_sections[MaxNumSections];
	uint32              m_numSections;
};

struct RPThreadTimings
{
	float waitForMain;
	float waitForRender;
	float waitForGPU;
	float gpuIdlePerc;
	float gpuFrameTime;
	float frameTime;
	float renderTime;

	RPThreadTimings()
		: waitForMain(0)
		, waitForRender(0)
		, waitForGPU(0)
		, gpuIdlePerc(0)
		, gpuFrameTime(33.0f)
		, frameTime(33.0f)
		, renderTime(0)
	{
	}
};

class CRenderPipelineProfiler
{
public:
	CRenderPipelineProfiler();

	void                   BeginFrame();
	void                   EndFrame();
	void                   BeginSection(const char* name, uint32 eProfileLabelFlags = 0);
	void                   EndSection(const char* name);

	void                   DisplayUI();

	bool                   IsEnabled();
	void                   SetEnabled(bool enabled)                                        { m_enabled = enabled; }
	void                   SetWaitForGPUTimers(bool wait)                                  { m_waitForGPUTimers = wait; }

	const RPProfilerStats& GetBasicStats(ERenderPipelineProfilerStats stat, int nThreadID) { assert((uint32)stat < RPPSTATS_NUM); return m_basicStats[nThreadID][stat]; }
	const RPProfilerStats* GetBasicStatsArray(int nThreadID)                               { return m_basicStats[nThreadID]; }

	void                   ReleaseGPUTimers();

protected:
	bool FilterLabel(const char* name);
	void UpdateThreadTimings();
	void ResetBasicStats(RPProfilerStats* pBasicStats, bool bResetAveragedStats);
	void ComputeAverageStats();
	void UpdateBasicStats();

	void DisplayBasicStats();
	void DisplayAdvancedStats();
	void DisplayExtensiveStats();

	bool WaitForTimers() { return m_waitForGPUTimers || (CRenderer::CV_r_profiler < 0); }

protected:
	#if OPENGL
	static const uint32 NumSectionsFrames = 4;
	#else
	static const uint32 NumSectionsFrames = 2;
	#endif

	std::vector<uint32> m_stack;
	RPPSectionsFrame    m_sectionsFrames[NumSectionsFrames];
	uint32              m_sectionsFrameIdx;
	float               m_gpuSyncTime;
	float               m_avgFrameTime;
	bool                m_enabled;
	bool                m_recordData;
	bool                m_waitForGPUTimers;

	RPProfilerStats     m_basicStats[RT_COMMAND_BUF_COUNT][RPPSTATS_NUM];
	RPThreadTimings     m_threadTimings;

	// we take a snapshot every now and then and store it in here to prevent the text from jumping too much
	std::multimap<CCryNameTSCRC, SStaticElementInfo> m_staticNameList;
};

#endif
