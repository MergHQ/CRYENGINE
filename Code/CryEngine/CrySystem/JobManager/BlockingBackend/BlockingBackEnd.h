// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   ThreadBackEnd.h
//  Version:     v1.00
//  Created:     07/05/2011 by Christopher Bolte
//  Compilers:   Visual Studio.NET
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef BLOCKING_BACKEND_H_
#define BLOCKING_BACKEND_H_

#include <CryThreading/IJobManager.h>
#include "../JobStructs.h"

#include <CryThreading/IThreadManager.h>

namespace JobManager
{
class CJobManager;
class CWorkerBackEndProfiler;
}

namespace JobManager {
namespace BlockingBackEnd {

// forward declarations
class CBlockingBackEnd;

// class to represent a worker thread for the PC backend
class CBlockingBackEndWorkerThread : public IThread
{
public:
	CBlockingBackEndWorkerThread(CBlockingBackEnd* pBlockingBackend, CryFastSemaphore& rSemaphore, JobManager::SJobQueue_BlockingBackEnd& rJobQueue, JobManager::SInfoBlock** pRegularWorkerFallbacks, uint32 nRegularWorkerThreads, uint32 nID);
	~CBlockingBackEndWorkerThread();

	// Start accepting work on thread
	virtual void ThreadEntry();

	// Signals to the worker thread that is should not accept anymore work and exit
	void SignalStopWork();

private:
	void DoWork();
	void DoWorkProducerConsumerQueue(SInfoBlock& rInfoBlock);

	uint32                                 m_nId;                   // id of the worker thread
	volatile bool                          m_bStop;
	CryFastSemaphore&                      m_rSemaphore;
	JobManager::SJobQueue_BlockingBackEnd& m_rJobQueue;
	CBlockingBackEnd*                      m_pBlockingBackend;

	// members used for special blocking backend fallback handling
	JobManager::SInfoBlock** m_pRegularWorkerFallbacks;
	uint32                   m_nRegularWorkerThreads;
};

bool   IsBlockingWorkerId(uint32 workerId);
// maps the workerId of a blocking worker to a unique index in [0, GetNumWorkerThreads()[
uint32 GetIndexFromWorkerId(uint32 workerId);

// the implementation of the PC backend
// has n-worker threads which use atomic operations to pull from the job queue
// and uses a semaphore to signal the workers if there is work required
class CBlockingBackEnd final : public IBackend
{
public:
	CBlockingBackEnd(JobManager::SInfoBlock** pRegularWorkerFallbacks, uint32 nRegularWorkerThreads);
	virtual ~CBlockingBackEnd();

	bool           Init(uint32 nSysMaxWorker) override;
	bool           ShutDown() override;
	void           Update() override {}

	virtual void   AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock) override;

	virtual uint32 GetNumWorkerThreads() const override { return m_nNumWorker; }

	void           AddBlockingFallbackJob(JobManager::SInfoBlock* pInfoBlock, uint32 nWorkerThreadID);

	virtual bool   KickTempWorker() override { return false; }
	virtual bool   StopTempWorker() override { return false; }

	virtual bool   AddTempWorkerUntilJobStateIsComplete(JobManager::SJobState& pJobState) { return false; }

#if defined(JOBMANAGER_SUPPORT_STATOSCOPE)
	JobManager::IWorkerBackEndProfiler* GetBackEndWorkerProfiler() const override { return m_pBackEndWorkerProfiler; }
#endif

private:
	friend class JobManager::CJobManager;

	JobManager::SJobQueue_BlockingBackEnd m_JobQueue;                   // job queue node where jobs are pushed into and from
	CryFastSemaphore                      m_Semaphore;                  // semaphore to count available jobs, to allow the workers to go sleeping instead of spinning when no work is requiered
	CBlockingBackEndWorkerThread**        m_pWorkerThreads = nullptr;   // worker threads for blocking backend
	uint8 m_nNumWorker = 0;                                             // number of allocated worker threads

	// members used for special blocking backend fallback handling
	JobManager::SInfoBlock** m_pRegularWorkerFallbacks;
	uint32                   m_nRegularWorkerThreads;

	// members required for profiling jobs in the frame profiler
#if defined(JOBMANAGER_SUPPORT_STATOSCOPE)
	JobManager::IWorkerBackEndProfiler* m_pBackEndWorkerProfiler = nullptr;
#endif
};

} // namespace BlockingBackEnd
} // namespace JobManager

#endif // BLOCKING_BACKEND_H_
