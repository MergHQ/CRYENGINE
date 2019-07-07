// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX11CommandScheduler.hpp"

namespace NCryDX11
{

//---------------------------------------------------------------------------------------------------------------------
CCommandScheduler::CCommandScheduler(CDevice* pDevice)
	: CDeviceObject(pDevice)
	, m_CmdListFenceSet(pDevice)
	, m_CmdListPool(pDevice)
{
	m_CmdListPool.Init();
	m_CmdListFenceSet.Init();

	ZeroMemory(&m_FrameFenceValuesSubmitted, sizeof(m_FrameFenceValuesSubmitted));
	ZeroMemory(&m_FrameFenceValuesCompleted, sizeof(m_FrameFenceValuesCompleted));

	m_FrameFenceCursor = 0;
}

//---------------------------------------------------------------------------------------------------------------------
CCommandScheduler::~CCommandScheduler()
{
}

//---------------------------------------------------------------------------------------------------------------------
bool CCommandScheduler::RecreateCommandListPool()
{
	m_CmdListPool.Clear();
	m_CmdListPool.Init();

	return true;
}

void CCommandScheduler::Flush(bool bWait)
{
	auto naryFences = m_CmdListFenceSet.GetD3D11Fence();

	const UINT64 currFenceValue = m_CmdListFenceSet.GetCurrentValue();
	const UINT64 nextFenceValue = currFenceValue + 1;

	{
		// Same as "Schedule" (submit) + "Execute" (signal)
		gcpRendD3D->GetDeviceContext()->End((*naryFences)[currFenceValue % naryFences->size()]);
		m_CmdListFenceSet.SetSubmittedValue(currFenceValue);

		gcpRendD3D->GetDeviceContext()->Flush();
		m_CmdListFenceSet.SetSignalledValue(currFenceValue);

		// Wait if necessary
		if (bWait)
		{
			m_CmdListFenceSet.WaitForFence(currFenceValue);
		}
	}

	{
		// Raise fence for continuous time-line
	//	gcpRendD3D->GetDeviceContext()->Begin((*naryFences)[nextFenceValue % naryFences->size()]);
		m_CmdListFenceSet.SetCurrentValue(nextFenceValue);
	}
}

void CCommandScheduler::GarbageCollect()
{
	CRY_PROFILE_SECTION(PROFILE_RENDERER, "FLUSH GPU HEAPS");

	// Ring buffer for the _completed_ fences of past number of frames
	m_CmdListFenceSet.AdvanceCompletion();
	m_FrameFenceValuesCompleted[(m_FrameFenceCursor)] = m_CmdListFenceSet.GetLastCompletedFenceValue();
	GetDevice()->FlushReleaseHeap(
		m_FrameFenceValuesCompleted[(m_FrameFenceCursor)],
		m_FrameFenceValuesCompleted[(m_FrameFenceCursor + (FRAME_FENCES - std::max(1, FRAME_FENCE_LATENCY - 1))) % FRAME_FENCES]);
}

void CCommandScheduler::SyncFrame()
{
	// Stall render thread until GPU has finished processing previous frame (in case max frame latency is 1)
	CRY_PROFILE_SECTION(PROFILE_RENDERER, "SYNC TO FRAME FENCE");

	// Block when more than N frames have not been rendered yet
	m_FrameFenceValuesSubmitted[(m_FrameFenceCursor)] = m_CmdListFenceSet.GetSubmittedValue();
	m_CmdListFenceSet.WaitForFence(
		m_FrameFenceValuesSubmitted[(m_FrameFenceCursor + (FRAME_FENCES - std::max(1, FRAME_FENCE_INFLIGHT - 1))) % FRAME_FENCES]);
}

void CCommandScheduler::EndOfFrame(bool bWait)
{
	// TODO: add final bWait if back-buffer is copied directly before the present
	Flush(bWait);

	if (CRendererCVars::CV_r_SyncToFrameFence)
		SyncFrame();

	GarbageCollect();

	m_FrameFenceCursor = ((++m_FrameFenceCursor) % FRAME_FENCES);
}

}
