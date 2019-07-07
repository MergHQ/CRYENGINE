// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryProfilingSystemSharedImpl.h"
#include "CryCore/RingBuffer.h"
#include <CrySystem/Profilers/ILegacyProfiler.h>
#include <CryThreading/CryThread.h>

class CBootProfiler;

//! the system which does the gathering of stats
class CCryProfilingSystem final : public CCryProfilingSystemImpl, public ILegacyProfiler
{
public:
	enum EProfiledValue
	{
		TIME,
		MEMORY,
	};

	CCryProfilingSystem();
	~CCryProfilingSystem();

	//////////////////////////////////////////////////////////
	// ICryProfilingSystem
	void PauseRecording(bool pause) final;
	//! Are we collecting profiling data?
	bool IsPaused() const final;
	
	void AddFrameListener(ICryProfilerFrameListener*) final;
	void RemoveFrameListener(ICryProfilerFrameListener*) final;

	void DescriptionDestroyed(SProfilingDescription*) final;

	void StartFrame() final;
	void EndFrame() final;

	using CCryProfilingSystemImpl::GetModuleName;
	//////////////////////////////////////////////////////////

	SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSection(SProfilingSection*);
	void EndSection(SProfilingSection*);
	void RecordMarker(SProfilingMarker*);

	// ILegacyProfiler
	const TrackerList* GetActiveTrackers() const final;
	const PeakList* GetPeakRecords() const final;

	void AcquireReadAccess() final;
	void ReleaseReadAccess() final;

	const CSamplerStats<TProfilingCount>& PageFaultsPerFrame() const final;
	const char* GetModuleName(const SProfilingSectionTracker*) const final;
	// ~ILegacyProfiler

	// CCryProfilingSystemImpl
	void RegisterCVars() final;
	void UnregisterCVars() final;
	// ~CCryProfilingSystemImpl

	void SetBootProfiler(CBootProfiler*);
	
	// callbacks to register in gEnv
	static SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSectionStatic(SProfilingSection*);
	static void RecordMarkerStatic(SProfilingMarker*);

	static void OnThreadEntry();
	static void OnThreadExit();

	// used as default profiler setup
	static Cry::ProfilerRegistry::SEntry MakeRegistryEntry();

	//! trackers with verbosity above this level are excluded
	//! exposed so that it can be initialized form CSystem
	static int s_verbosity;

private:
	struct AutoTimer;
	typedef CryAutoReadLock<CryRWLock> AutoReadLock;
	typedef CryAutoWriteLock<CryRWLock> AutoWriteLock;

	SProfilingSectionTracker* AddTracker(const SProfilingDescription* pDesc, threadID tid);

	void CheckForPeak(int64, SProfilingSectionTracker* pTracker);
	void CommitFrame(SProfilingSectionTracker* pTracker, float blendFactor);

	bool IsExcludedByFilter(const SProfilingSectionTracker* pTracker) const;
	bool IsExcludedByFilter(const SProfilingMarker* pMarker) const;
	void ReapplyTrackerFilter();

	bool m_trackingPaused = false;
	bool m_willPause      = false;

	CBootProfiler* m_pBootProfiler = nullptr;
	std::vector<ICryProfilerFrameListener*> m_FrameListeners;

	uint32 m_lastTotalPageFaults = 0;

	//! trackers can be filtered by thread and subsystem
	TrackerList m_activeTrackers;
	const float m_MsPerTick;
	PeakList m_peaks;
	
	// trackers currently filtered out
	TrackerList m_excludedTrackers;

	// putting these into separate cache lines to avoid false sharing
	CRY_ALIGN(64) CryRWLock m_readWriteLock;
	CRY_ALIGN(64) char __padding;
	
	//////////////////////////////////////////////////////////
	// CVars and callbacks
	static void EndSectionStatic(SProfilingSection*);

	static void ProfilingSystemFilterCvarChangedCallback(struct ICVar* pCvar);
	static void FilterSubsystemCommand(struct IConsoleCmdArgs* args);
	static void FilterThreadCommand(struct IConsoleCmdArgs* args);
	
	static CCryProfilingSystem* s_pInstance;
	static volatile int         s_instanceLock;
	//! if non-zero only profile this thread
	static threadID s_exclusiveThread;
	//! the bit (1 << ProfiledSubsystem) is set, if the subsystem should be excluded
	static int64 s_subsystemFilterMask;
	static_assert(EProfiledSubsystem::PROFILE_LAST_SUBSYSTEM < 64, "Subsystems no longer fit in bitmask!");

	static float profile_peak_tolerance;
	static int profile_value;
};
