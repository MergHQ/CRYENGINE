// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#ifndef __CCRYDX12SUBMISSIONTHREAD_
	#define __CCRYDX12SUBMISSIONTHREAD_

	#include <CryThreading/IThreadManager.h>
	#include "DX12/Includes/concqueue.hpp"
	#include "DX12Base.hpp"

struct IDXGISwapChain3;

namespace NCryDX12
{
class CCommandListPool;

class CAsyncCommandQueue : public IThread
{
public:
	static const int MAX_FRAMES_GPU_LAG = 1;   // Maximum number of frames GPU can lag behind CPU. TODO: tie to cvar

	CAsyncCommandQueue();
	~CAsyncCommandQueue();

	template<typename TaskType>
	static bool IsSynchronous()
	{
		return CRenderer::CV_r_D3D12SubmissionThread == 0;
	}

	void Init(CCommandListPool* pCommandListPool);
	void Flush(UINT64 lowerBoundFenceValue = ~0ULL);
	void FlushNextPresent();
	void SignalStop() { m_bStopRequested = true; }

	// Equates to the number of pending Present() calls
	int    GetQueuedFramesCount() const   { return m_QueuedFramesCounter; }
	UINT64 GetSignalledFenceValue() const { return m_SignalledFenceValue; }

	void   Present(IDXGISwapChain3* pSwapChain, HRESULT* pPresentResult, UINT SyncInterval, UINT Flags, UINT bufferIndex);
	void   ResetCommandList(CCommandList* pCommandList);
	void   ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
	void   Signal(ID3D12Fence* pFence, const UINT64 Value);
	void   Wait(ID3D12Fence* pFence, const UINT64 Value);
	void   Wait(ID3D12Fence** pFences, const UINT64 (&Values)[CMDQUEUE_NUM]);
private:

	enum eTaskType
	{
		eTT_ExecuteCommandList,
		eTT_ResetCommandList,
		eTT_PresentBackbuffer,
		eTT_SignalFence,
		eTT_WaitForFence,
		eTT_WaitForFences
	};

	struct STaskArgs
	{
		ID3D12CommandQueue* pCommandQueue;
		volatile int*       QueueFramesCounter;
		volatile UINT64*    SignalledFenceValue;
	};

	struct SExecuteCommandlist
	{
		ID3D12CommandList* pCommandList;

		void               Process(const STaskArgs& args);
	};

	struct SResetCommandlist
	{
		CCommandList* pCommandList;

		void          Process(const STaskArgs& args);
	};

	struct SSignalFence
	{
		ID3D12Fence* pFence;
		UINT64       FenceValue;

		void         Process(const STaskArgs& args);
	};

	struct SWaitForFence
	{
		ID3D12Fence* pFence;
		UINT64       FenceValue;

		void         Process(const STaskArgs& args);
	};

	struct SWaitForFences
	{
		ID3D12Fence** pFences;
		UINT64        FenceValues[CMDQUEUE_NUM];

		void          Process(const STaskArgs& args);
	};

	struct SPresentBackbuffer
	{
		IDXGISwapChain3* pSwapChain;
		HRESULT*         pPresentResult;
		UINT             SyncInterval;
		UINT             Flags;

		void             Process(const STaskArgs& args);
	};

	struct SSubmissionTask
	{
		eTaskType type;

		union
		{
			SSignalFence        SignalFence;
			SWaitForFence       WaitForFence;
			SWaitForFences      WaitForFences;
			SExecuteCommandlist ExecuteCommandList;
			SResetCommandlist   ResetCommandList;
			SPresentBackbuffer  PresentBackbuffer;
		} Data;

		template<typename TaskType>
		void Process(const STaskArgs& args)
		{
			TaskType* pTask = reinterpret_cast<TaskType*>(&Data);
			pTask->Process(args);
		}
	};

	template<typename TaskType>
	void AddTask(SSubmissionTask& task)
	{
		if (CRenderer::CV_r_D3D12SubmissionThread)
		{
			m_TaskQueue.enqueue(task);
			m_TaskEvent.Release();
		}
		else
		{
			Flush();
			STaskArgs taskArgs = { m_pCmdListPool->GetD3D12CommandQueue(), &m_QueuedFramesCounter, &m_SignalledFenceValue };
			task.Process<TaskType>(taskArgs);
		}
	}

	void ThreadEntry() override;

	volatile UINT64                         m_SignalledFenceValue;
	volatile int                            m_QueuedFramesCounter;
	volatile bool                           m_bStopRequested;
	volatile bool                           m_bSleeping;

	CCommandListPool*                       m_pCmdListPool;
	ConcQueue<UnboundMPSC, SSubmissionTask> m_TaskQueue;
	CryFastSemaphore                        m_TaskEvent;
};
};

#endif
