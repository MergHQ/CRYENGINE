// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DX11Base.hpp"
#include "DX11Device.hpp"
#include "DX11CommandList.hpp"
#include "DX11CommandListPool.hpp"

namespace NCryDX11 {

class CDevice;

class CCommandScheduler : public CDeviceObject
{
public:
	CCommandScheduler(CDevice* pDevice);
	virtual ~CCommandScheduler();
	
	inline CCommandListPool& GetCommandListPool() { return m_CmdListPool; }

	bool RecreateCommandListPool();

	void Flush(bool bWait);
	void GarbageCollect();
	void SyncFrame();
	void EndOfFrame(bool bWait);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fence management API
	ILINE const CCommandListFenceSet& GetFenceManager() const { return m_CmdListFenceSet; }
	
private:
#define FRAME_FENCES         32
#define FRAME_FENCE_LATENCY  32
#define FRAME_FENCE_INFLIGHT std::min(MAX_FRAMES_IN_FLIGHT, CRendererCVars::CV_r_MaxFrameLatency + 1)
	UINT64                        m_FrameFenceValuesSubmitted[FRAME_FENCES];
	UINT64                        m_FrameFenceValuesCompleted[FRAME_FENCES];
	ULONG                         m_FrameFenceCursor;

	CCommandListFenceSet          m_CmdListFenceSet;
#if defined(_ALLOW_INITIALIZER_LISTS)
	CCommandListPool              m_CmdListPool;
#else
	std::vector<CCommandListPool> m_CmdListPool;
#endif

#ifdef DX11_STATS
	size_t m_NumCommandListOverflows;
	size_t m_NumCommandListSplits;
#endif // DX11_STATS
};

}
