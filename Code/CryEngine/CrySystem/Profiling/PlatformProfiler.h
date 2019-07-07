// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryProfilingSystemSharedImpl.h"

// add all implemented platforms here
#if ALLOW_PLATFORM_PROFILER && (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO || CRY_PLATFORM_ORBIS)
#	define USE_PLATFORM_PROFILER 1
#else
#	define USE_PLATFORM_PROFILER 0
#endif

#if USE_PLATFORM_PROFILER

class CPlatformProfiler : public CCryProfilingSystemImpl
{
public:
	CPlatformProfiler();
	~CPlatformProfiler();

	void PauseRecording(bool pause) final;
	bool IsPaused() const final { return m_paused; }

	void StartFrame() final;
	void EndFrame() final;

	void RegisterCVars() final {}
	void UnregisterCVars() final {}

	static SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSectionStatic(SProfilingSection* p);
	static void RecordMarkerStatic(SProfilingMarker* p);

private:
	static void EndSectionStatic(SProfilingSection* p);
	static CPlatformProfiler* s_pInstance;

	bool m_paused = false;
};

#endif