// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*
   definitions for job manager
   singleton implementation
 */

#ifndef __JOB_MANAGER_H__
#define __JOB_MANAGER_H__
#pragma once

#define JOBSYSTEM_INVOKER_COUNT (128)

#include <CryCore/Platform/platform.h>
#include <CryThreading/CryAtomics.h>

#include <CryThreading/IJobManager.h>
#include "JobStructs.h"

#include <map>
#include <CryInput/IInput.h>
#include <CryMath/Cry_Color.h>

///////////////////////////////////////////////////////////////////////////////
namespace JobManager
{
/////////////////////////////////////////////////////////////////////////////
// Util Functions to get access Thread local data (needs to be unique per worker thread)
namespace detail {

// functions to access the per thread worker thread id
void   SetWorkerThreadId(uint32 nWorkerThreadId);
uint32 GetWorkerThreadId();

} // namespace detail

namespace ThreadBackEnd   {
class CThreadBackEnd;
}
namespace BlockingBackEnd {
class CBlockingBackEnd;
}

// Tracks CPU/PPU worker thread(s) utilization and job execution time per frame
class CWorkerBackEndProfiler : public IWorkerBackEndProfiler
{
public:
	CWorkerBackEndProfiler();
	virtual ~CWorkerBackEndProfiler();

	virtual void Init(const uint16 numWorkers) override;

	// Update the profiler at the beginning of the sample period
	virtual void Update() override;
	virtual void Update(const uint32 curTimeSample) override;

	// Register a job with the profiler
	virtual void RegisterJob(const uint32 jobId, const char* jobName) override;

	// Record execution information for a registered job
	virtual void RecordJob(const uint16 profileIndex, const uint8 workerId, const uint32 jobId, const uint32 runTimeMicroSec) override;

	// Get worker frame stats for the JobManager::detail::eJOB_FRAME_STATS - 1 frame
	virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats) const override;
	virtual void GetFrameStats(TJobFrameStatsContainer& rJobStats, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const override;
	virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats, TJobFrameStatsContainer& rJobStats, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const override;

	// Get worker frame stats summary
	virtual void GetFrameStatsSummary(SWorkerFrameStatsSummary& rStats) const override;
	virtual void GetFrameStatsSummary(SJobFrameStatsSummary& rStats) const override;

	// Returns the index of the active multi-buffered profile data
	virtual uint16 GetProfileIndex() const override;

	// Get the number of workers tracked
	virtual uint32 GetNumWorkers() const override;

protected:
	void GetWorkerStats(const uint8 nBufferIndex, JobManager::CWorkerFrameStats& rWorkerStats) const;
	void GetJobStats(const uint8 nBufferIndex, TJobFrameStatsContainer& rJobStatsContainer, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const;
	void ResetWorkerStats(const uint8 nBufferIndex, const uint32 curTimeSample);
	void ResetJobStats(const uint8 nBufferIndex);

protected:
	struct SJobStatsInfo
	{
		JobManager::SJobFrameStats m_pJobStats[JobManager::detail::eJOB_FRAME_STATS* JobManager::detail::eJOB_FRAME_STATS_MAX_SUPP_JOBS];     // Array of job stats (multi buffered)
	};

	struct SWorkerStatsInfo
	{
		uint32                    m_nStartTime[JobManager::detail::eJOB_FRAME_STATS]; // Start Time of sample period (multi buffered)
		uint32                    m_nEndTime[JobManager::detail::eJOB_FRAME_STATS];   // End Time of sample period (multi buffered)
		uint16                    m_nNumWorkers;                                      // Number of workers tracked
		JobManager::SWorkerStats* m_pWorkerStats;                                     // Array of worker stats for each worker (multi buffered)
	};

protected:
	uint8            m_nCurBufIndex;      // Current buffer index [0,(JobManager::detail::eJOB_FRAME_STATS-1)]
	SJobStatsInfo    m_JobStatsInfo;      // Information about all job activities
	SWorkerStatsInfo m_WorkerStatsInfo;   // Information about each worker's utilization
};

// singleton managing the job queues
class CRY_ALIGN(128) CJobManager final : public IJobManager, public IInputEventListener
{
public:
	// singleton stuff
	static CJobManager* Instance();

	//destructor
	virtual ~CJobManager();

	virtual void Init(uint32 nSysMaxWorker) override;

	// wait for a job, preempt the calling thread if the job is not done yet
	virtual const bool WaitForJob(JobManager::SJobState& rJobState) const override;

	//adds a job
	virtual void AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle) override;

	//obtain job handle from name
	virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, const uint32 cStrLen, JobManager::Invoker pInvoker) override;
	virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, JobManager::Invoker pInvoker) override
	{
		return GetJobHandle(cpJobName, strlen(cpJobName), pInvoker);
	}

	virtual JobManager::IBackend* GetBackEnd(JobManager::EBackEndType backEndType) override;

	//shuts down job manager
	virtual void ShutDown() override;

	virtual bool InvokeAsJob(const char* cpJobName) const override;
	virtual bool InvokeAsJob(const JobManager::TJobHandle cJobHandle) const override;

	virtual void SetJobFilter(const char* pFilter) override
	{
		m_pJobFilter = pFilter;
	}

	virtual void SetJobSystemEnabled(int nEnable) override
	{
		m_nJobSystemEnabled = nEnable;
	}

	//copy the job parameter into the jobinfo  structure
	static void CopyJobParameter(const uint32 cJobParamSize, void* pDest, const void* pSrc);

	uint32 GetWorkerThreadId() const override;

	virtual JobManager::SJobProfilingData* GetProfilingData(uint16 nProfilerIndex) override;
	virtual uint16 ReserveProfilingData() override;

	void Update(int nJobSystemProfiler) override;

	void SetFrameStartTime(const CTimeValue &rFrameStartTime) override;
	void SetMainDoneTime(const CTimeValue&) override;
	void SetRenderDoneTime(const CTimeValue&) override;

	JobManager::Invoker GetJobInvoker(uint32 nIdx)
	{
		assert(nIdx < m_nJobInvokerIdx);
		assert(nIdx < JOBSYSTEM_INVOKER_COUNT);
		return m_arrJobInvokers[nIdx];
	}
	virtual uint32 GetNumWorkerThreads() const override;

	// get a free semaphore from the jobmanager pool
	virtual JobManager::TSemaphoreHandle AllocateSemaphore(volatile const void* pOwner) override;

	// return a semaphore to the jobmanager pool
	virtual void DeallocateSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) override;

	// increase the refcounter of a semaphore, but only if it is > 0, else returns false
	virtual bool AddRefSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) override;

	// 'allocate' a semaphore in the jobmanager, and return the index of it
	virtual SJobFinishedConditionVariable* GetSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) override;

	virtual void DumpJobList() override;

	virtual bool OnInputEvent(const SInputEvent &event) override;

	void IncreaseRunJobs();
	void IncreaseRunFallbackJobs();

	const char* GetJobName(JobManager::Invoker invoker);

private:
	static ColorB GenerateColorBasedOnName(const char* name);

	void KickTempWorker() const;
	void StopTempWorker() const;

	CryCriticalSection m_JobManagerLock;                             // lock to protect non-performance critical parts of the jobmanager
	JobManager::Invoker m_arrJobInvokers[JOBSYSTEM_INVOKER_COUNT];   // support 128 jobs for now
	uint32 m_nJobInvokerIdx;

	const char* m_pJobFilter;
	int m_nJobSystemEnabled = 1;                                // should the job system be used
	int m_bJobSystemProfilerEnabled = false;                    // should the job system profiler be enabled
	bool m_bJobSystemProfilerPaused = false;                    // should the job system profiler be paused

	bool m_Initialized = false;                                     //true if JobManager have been initialized

	ThreadBackEnd::CThreadBackEnd* m_pThreadBackEnd = nullptr;       // Backend for regular jobs, available on PC/XBOX. on Xbox threads are polling with a low priority
	BlockingBackEnd::CBlockingBackEnd* m_pBlockingBackEnd = nullptr; // Backend for tasks which can block to prevent stalling regular jobs in this case

	uint16 m_nJobIdCounter = 0;                     // JobId counter for jobs dynamically allocated at runtime

	std::set<JobManager::SJobStringHandle> m_registeredJobs;

	enum { nSemaphorePoolSize = 16 };
	SJobFinishedConditionVariable m_JobSemaphorePool[nSemaphorePoolSize];
	uint32 m_nCurrentSemaphoreIndex;

	// per frame counter for jobs run/fallback jobs
	uint32 m_nJobsRunCounter = 0;
	uint32 m_nFallbackJobsRunCounter = 0;

	JobManager::SInfoBlock** m_pRegularWorkerFallbacks;
	uint32 m_nRegularWorkerThreads;

	bool m_bSuspendWorkerForMP = false;
	CryMutex m_SuspendWorkerForMPLock;
	CryConditionVariable m_SuspendWorkerForMPCondion;
#if defined(JOBMANAGER_SUPPORT_PROFILING)
	SJobProfilingDataContainer m_profilingData;
	std::map<JobManager::SJobStringHandle, ColorB> m_JobColors;
	CTimeValue m_FrameStartTime[SJobProfilingDataContainer::nCapturedFrames];
	CTimeValue m_mainDoneTime[SJobProfilingDataContainer::nCapturedFrames];
	CTimeValue m_renderDoneTime[SJobProfilingDataContainer::nCapturedFrames];
#endif

	// singleton stuff
	CJobManager();
	// disable copy and assignment
	CJobManager(const CJobManager&) = delete;
	CJobManager& operator=(const CJobManager&) = delete; 
};
}//JobManager

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CJobManager::CopyJobParameter(const uint32 cJobParamSize, void* pDestParam, const void* pSrcParam)
{
	assert(IsAligned(cJobParamSize, 16) && "JobParameter Size needs to be a multiple of 16");
	assert(cJobParamSize <= JobManager::SInfoBlock::scAvailParamSize && "JobParameter Size larger than available storage");
	memcpy(pDestParam, pSrcParam, cJobParamSize);
}

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CJobManager::IncreaseRunJobs()
{
	CryInterlockedIncrement((int volatile*)&m_nJobsRunCounter);
}

#endif //__JOB_MANAGER_H__
