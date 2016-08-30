// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     08/05/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __DX12SWAPCHAIN__
	#define __DX12SWAPCHAIN__

	#include "DX12Base.hpp"
	#include "DX12CommandList.hpp"
	#include "DX12AsyncCommandQueue.hpp"

namespace NCryDX12
{

class CSwapChain : public CRefCounted
{
public:
	static CSwapChain* Create(CCommandListPool& commandQueue, IDXGIFactory4* factory, DXGI_SWAP_CHAIN_DESC* pDesc);

protected:
	CSwapChain(CCommandListPool& commandQueue, IDXGISwapChain3* dxgiSwapChain, DXGI_SWAP_CHAIN_DESC* pDesc);

	virtual ~CSwapChain();

public:
	ILINE IDXGISwapChain3* GetDXGISwapChain() const
	{
		return m_pDXGISwapChain;
	}

	ILINE UINT GetBackBufferCount()
	{
		return m_Desc.BufferCount;
	}

	ILINE CResource& GetBackBuffer(UINT index)
	{
		return m_BackBuffers[index];
	}
	ILINE CResource& GetCurrentBackBuffer()
	{
		return m_BackBuffers[m_pDXGISwapChain->GetCurrentBackBufferIndex()];
	}

	ILINE CView& GetBackBufferView(UINT index, bool isRTV)
	{
		if (isRTV)
			return m_BackBufferRTVs[index];
		/* else */
		return m_BackBufferSRVs[index];
	}
	ILINE CView& GetCurrentBackBufferView(bool isRTV)
	{
		if (isRTV)
			return m_BackBufferRTVs[m_pDXGISwapChain->GetCurrentBackBufferIndex()];
		/* else */
		return m_BackBufferSRVs[m_pDXGISwapChain->GetCurrentBackBufferIndex()];
	}

	ILINE UINT GetCurrentBackbufferIndex() const
	{
		return m_CurrentBackbufferIndex;
	}

	ILINE bool IsPresentScheduled() const
	{
		return m_asyncQueue.GetQueuedFramesCount() > 0;
	}

	void Present(UINT SyncInterval, UINT Flags)
	{
		m_asyncQueue.FlushNextPresent();
		DX12_ASSERT(!IsPresentScheduled(), "Flush didn't dry out all outstanding Present() calls!");

		m_asyncQueue.Present(m_pDXGISwapChain, &m_PresentResult, SyncInterval, Flags, m_Desc.Flags, m_CurrentBackbufferIndex);

		m_CurrentBackbufferIndex = (m_CurrentBackbufferIndex + 1) % m_Desc.BufferCount;
	}

	HRESULT GetLastPresentReturnValue() { return m_PresentResult; }

	HRESULT ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters);
	HRESULT ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	void    AcquireBuffers();
	void    UnblockBuffers(NCryDX12::CCommandList* pCommandList);
	void    VerifyBufferCounters();
	void    ForfeitBuffers();

private:
	CAsyncCommandQueue&  m_asyncQueue;
	CCommandListPool&    m_pCommandQueue;

	DXGI_SWAP_CHAIN_DESC m_Desc;
	UINT                 m_CurrentBackbufferIndex;

	HRESULT              m_PresentResult;
	DX12_PTR(IDXGISwapChain3) m_pDXGISwapChain;

	std::vector<CResource> m_BackBuffers;
	std::vector<CView>     m_BackBufferRTVs;
	std::vector<CView>     m_BackBufferSRVs;
	bool                   m_bPresentScheduled;
};

}

#endif // __DX12SWAPCHAIN__
