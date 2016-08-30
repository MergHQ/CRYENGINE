// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX12SwapChain.hpp"
//#define I_WONT_USE_THE_FRAME_DEBUGGER // MSVC frame debugger has different internal ref-counts

namespace NCryDX12
{

CSwapChain* CSwapChain::Create(CCommandListPool& commandQueue, IDXGIFactory4* pFactory, DXGI_SWAP_CHAIN_DESC* pDesc)
{
	IDXGISwapChain* pDXGISwapChain = NULL;
	IDXGISwapChain3* pDXGISwapChain3 = NULL;
	ID3D12CommandQueue* pQueue12 = commandQueue.GetD3D12CommandQueue();

	// If discard isn't implemented/supported/fails, try the newer swap-types
	// - flip_discard is win 10
	// - flip_sequentially is win 8
	if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
	{
		pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
	}
	else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
	{
		pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
	}

#ifdef CRY_USE_DX12_MULTIADAPTER
	// Put swap-chain on node "0"
	if (commandQueue.GetDevice()->IsMultiAdapter())
	{
		pQueue12 = commandQueue.GetDevice()->GetNativeObject(pQueue12, 0);
	}
#endif

	HRESULT hr = pFactory->CreateSwapChain(pQueue12, pDesc, &pDXGISwapChain);

	if (hr == S_OK && pDXGISwapChain)
	{
		hr = pDXGISwapChain->QueryInterface(IID_PPV_ARGS(&pDXGISwapChain3));
		pDXGISwapChain->Release();

		if (hr == S_OK && pDXGISwapChain3)
		{
			auto result = new CSwapChain(commandQueue, pDXGISwapChain3, pDesc);
			return result;
		}
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
CSwapChain::CSwapChain(CCommandListPool& commandQueue, IDXGISwapChain3* dxgiSwapChain, DXGI_SWAP_CHAIN_DESC* pDesc)
	: CRefCounted()
	, m_CurrentBackbufferIndex(dxgiSwapChain->GetCurrentBackBufferIndex())
	, m_bPresentScheduled(false)
	, m_pCommandQueue(commandQueue)
	, m_asyncQueue(commandQueue.GetAsyncCommandQueue())
{
	if (pDesc)
	{
		m_Desc = *pDesc;
	}
	else
	{
		dxgiSwapChain->GetDesc(&m_Desc);
	}

	m_PresentResult = S_OK;
	m_pDXGISwapChain = dxgiSwapChain;
	dxgiSwapChain->Release();

	AcquireBuffers();
}

//---------------------------------------------------------------------------------------------------------------------
CSwapChain::~CSwapChain()
{
	ForfeitBuffers();

	m_pDXGISwapChain->Release();
}

HRESULT CSwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
	return m_pDXGISwapChain->ResizeTarget(
	  pNewTargetParameters);
}

HRESULT CSwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	m_Desc.BufferCount = BufferCount ? BufferCount : m_Desc.BufferCount;
	m_Desc.BufferDesc.Width = Width ? Width : m_Desc.BufferDesc.Width;
	m_Desc.BufferDesc.Height = Height ? Height : m_Desc.BufferDesc.Height;
	m_Desc.BufferDesc.Format = NewFormat != DXGI_FORMAT_UNKNOWN ? NewFormat : m_Desc.BufferDesc.Format;
	m_Desc.Flags = SwapChainFlags;

	return m_pDXGISwapChain->ResizeBuffers(
	  m_Desc.BufferCount,
	  m_Desc.BufferDesc.Width,
	  m_Desc.BufferDesc.Height,
	  m_Desc.BufferDesc.Format,
	  m_Desc.Flags);
}

void CSwapChain::AcquireBuffers()
{
	m_BackBuffers.reserve(m_Desc.BufferCount);
	m_BackBufferRTVs.reserve(m_Desc.BufferCount);
	m_BackBufferSRVs.reserve(m_Desc.BufferCount);

#ifdef CRY_USE_DX12_MULTIADAPTER
	if (m_pCommandQueue.GetDevice()->IsMultiAdapter())
	{
		// Buffers created using ResizeBuffers1 with a non-null pCreationNodeMask array are visible to all nodes.
		std::vector<UINT> createNodeMasks(m_Desc.BufferCount, 0x1);
		std::vector<IUnknown*> presentCommandQueues(m_Desc.BufferCount, m_pCommandQueue.GetDevice()->GetNativeObject(m_pCommandQueue.GetD3D12CommandQueue(), 0));

		HRESULT ret = m_pDXGISwapChain->ResizeBuffers1(
		  m_Desc.BufferCount,
		  m_Desc.BufferDesc.Width,
		  m_Desc.BufferDesc.Height,
		  m_Desc.BufferDesc.Format,
		  m_Desc.Flags,
		  &createNodeMasks[0],
		  &presentCommandQueues[0]);

		DX12_ASSERT(ret == S_OK, "Failed to re-locate the swap-chain's backbuffer(s) to all device nodes!");
	}
#endif

	for (int i = 0; i < m_Desc.BufferCount; ++i)
	{
		ID3D12Resource* pResource12 = nullptr;
		if (S_OK == m_pDXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&pResource12)))
		{
#ifdef CRY_USE_DX12_MULTIADAPTER
			if (m_pCommandQueue.GetDevice()->IsMultiAdapter())
			{
				ID3D12Resource* pResource12s[2] = { pResource12, nullptr };

				HRESULT ret = m_pCommandQueue.GetDevice()->DuplicateNativeCommittedResource(
					1U << 1U,
					m_pCommandQueue.GetDevice()->GetNodeMask(),
					pResource12s[0],
					&pResource12s[1]);

	#if !defined(RELEASE) && defined(I_WONT_USE_THE_FRAME_DEBUGGER)
				ULONG refCount0 = pResource12s[0]->AddRef() - 1;
				pResource12s[0]->Release();
				ULONG refCount1 = pResource12s[1]->AddRef() - 1;
				pResource12s[1]->Release();
				DX12_ASSERT(refCount0 == 1 + i, "RefCount corruption!");
				DX12_ASSERT(refCount1 == 1 + 0, "RefCount corruption!");
	#endif  // !RELEASE

				// Get broadcast resource from node "0" valid for all nodes
				pResource12 = m_pCommandQueue.GetDevice()->CreateBroadcastObject(pResource12s);

				ULONG refCount = pResource12s[0]->Release();

	#if !defined(RELEASE) && defined(I_WONT_USE_THE_FRAME_DEBUGGER)
				DX12_ASSERT(refCount == 1 + i, "RefCount corruption!");
	#endif  // !RELEASE
			}
#endif

			m_BackBuffers.emplace_back(m_pCommandQueue.GetDevice());
			m_BackBuffers[i].Init(pResource12, D3D12_RESOURCE_STATE_PRESENT);
			m_BackBuffers[i].SetDX12SwapChain(this);

			m_BackBufferRTVs.emplace_back();
			m_BackBufferRTVs[i].Init(m_BackBuffers[i], EVT_RenderTargetView);

			m_BackBufferSRVs.emplace_back();
			m_BackBufferSRVs[i].Init(m_BackBuffers[i], EVT_ShaderResourceView);

			ULONG refCount = pResource12->Release();

#if !defined(RELEASE) && defined(I_WONT_USE_THE_FRAME_DEBUGGER)
			if (m_pCommandQueue.GetDevice()->IsMultiAdapter())
			{
				DX12_ASSERT(refCount == 1 + 0, "RefCount corruption!");
			}
			else
			{
				DX12_ASSERT(refCount == 1 + i, "RefCount corruption!");
			}
#endif  // !RELEASE
		}
	}

	m_CurrentBackbufferIndex = m_pDXGISwapChain->GetCurrentBackBufferIndex();
}

void CSwapChain::UnblockBuffers(NCryDX12::CCommandList* pCommandList)
{
	for (int i = 0; i < m_Desc.BufferCount; ++i)
	{
		D3D12_RESOURCE_STATES resourceState = m_BackBuffers[i].GetAnnouncedState();
		if (resourceState != (D3D12_RESOURCE_STATES)-1)
		{
			m_BackBuffers[i].EndTransitionBarrier(pCommandList, resourceState);
		}
	}
}

void CSwapChain::VerifyBufferCounters()
{
#if !defined(RELEASE) && defined(I_WONT_USE_THE_FRAME_DEBUGGER)
	for (int i = 0; i < m_Desc.BufferCount; ++i)
	{
		ID3D12Resource* pResource12 = m_BackBuffers[i].GetD3D12Resource();

	#ifdef CRY_USE_DX12_MULTIADAPTER
		if (m_pCommandQueue.GetDevice()->IsMultiAdapter())
		{
			;
		}
	#endif

	#ifndef RELEASE
		ULONG refCount = pResource12->AddRef() - 1;
		pResource12->Release();
		if (m_pCommandQueue.GetDevice()->IsMultiAdapter())
		{
			// 1x for m_BackBuffers, 1x for ReleaseHeap
			DX12_ASSERT(refCount == 2, "RefCount corruption!");
		}
		else
		{
			// 1x for m_BackBuffers, 1x for ReleaseHeap + counter is shared
			DX12_ASSERT(refCount == 2 * m_Desc.BufferCount, "RefCount corruption!");
		}
	#endif
	}
#endif // !RELEASE
}

void CSwapChain::ForfeitBuffers()
{
	VerifyBufferCounters();

	m_BackBufferRTVs.clear();
	m_BackBufferSRVs.clear();
	m_BackBuffers.clear();
}

}
