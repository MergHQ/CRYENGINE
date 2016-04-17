// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CCryDX12SwapChain.hpp"

#include "DX12/Device/CCryDX12Device.hpp"
#include "DX12/Device/CCryDX12DeviceContext.hpp"
#include "DX12/Resource/CCryDX12Resource.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture1D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture3D.hpp"

#include "DX12/API/DX12Device.hpp"
#include "DX12/API/DX12SwapChain.hpp"
#include "DX12/API/DX12Resource.hpp"
#include "Dx12/API/DX12AsyncCommandQueue.hpp"

CCryDX12SwapChain* CCryDX12SwapChain::Create(CCryDX12Device* pDevice, IDXGIFactory4* factory, DXGI_SWAP_CHAIN_DESC* pDesc)
{
	CCryDX12DeviceContext* pDeviceContext = pDevice->GetDeviceContext();
	NCryDX12::CSwapChain* pDX12SwapChain = NCryDX12::CSwapChain::Create(pDeviceContext->GetCoreGraphicsCommandListPool(), factory, pDesc);

	return DX12_NEW_RAW(CCryDX12SwapChain(pDevice, pDX12SwapChain));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12SwapChain::CCryDX12SwapChain(CCryDX12Device* pDevice, NCryDX12::CSwapChain* pSwapChain)
	: Super()
	, m_pDevice(pDevice)
	, m_pDX12SwapChain(pSwapChain)
{
	DX12_FUNC_LOG
}

CCryDX12SwapChain::~CCryDX12SwapChain()
{
	DX12_FUNC_LOG
}

/* IDXGIDeviceSubObject implementation */

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetDevice(
  _In_ REFIID riid,
  _Out_ void** ppDevice)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetDevice(riid, ppDevice);
}

/* IDXGISwapChain implementation */

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::Present(
  UINT SyncInterval,
  UINT Flags)
{
	DX12_FUNC_LOG

	m_pDevice->GetDeviceContext()->Finish(m_pDX12SwapChain);

	DX12_LOG(g_nPrintDX12, "------------------------------------------------ PRESENT ------------------------------------------------");
	m_pDX12SwapChain->Present(SyncInterval, Flags);

	return m_pDX12SwapChain->GetLastPresentReturnValue();
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetBuffer(
  UINT Buffer,
  _In_ REFIID riid,
  _Out_ void** ppSurface)
{
	DX12_FUNC_LOG
	if (riid == __uuidof(ID3D11Texture2D))
	{
		const NCryDX12::CResource& dx12Resource = m_pDX12SwapChain->GetBackBuffer(Buffer);

		switch (dx12Resource.GetDesc().Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			*ppSurface = CCryDX12Buffer::Create(m_pDevice, this, dx12Resource.GetD3D12Resource());
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			*ppSurface = CCryDX12Texture1D::Create(m_pDevice, this, dx12Resource.GetD3D12Resource());
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			*ppSurface = CCryDX12Texture2D::Create(m_pDevice, this, dx12Resource.GetD3D12Resource());
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			*ppSurface = CCryDX12Texture3D::Create(m_pDevice, this, dx12Resource.GetD3D12Resource());
			break;
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:
		default:
			DX12_ASSERT(0, "Not implemented!");
			break;
		}
	}
	else
	{
		DX12_ASSERT(0, "Not implemented!");
		return -1;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::SetFullscreenState(
  BOOL Fullscreen,
  _In_opt_ IDXGIOutput* pTarget)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetFullscreenState(
  _Out_opt_ BOOL* pFullscreen,
  _Out_opt_ IDXGIOutput** ppTarget)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetDesc(
  _Out_ DXGI_SWAP_CHAIN_DESC* pDesc)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::ResizeBuffers(
  UINT BufferCount,
  UINT Width,
  UINT Height,
  DXGI_FORMAT NewFormat,
  UINT SwapChainFlags)
{
	DX12_FUNC_LOG

	UINT64 FrameFenceValues[CMDQUEUE_NUM];
	UINT64 FrameFenceNulls[CMDQUEUE_NUM] = { 0ULL, 0ULL, 0ULL };

	// Submit pending barriers in case the we try to resize in the middle of a frame
	m_pDX12SwapChain->UnblockBuffers(m_pDevice->GetDeviceContext()->GetCoreGraphicsCommandList());

	// Submit pending command-lists in case there are left-overs, make sure it's flushed to and executed on the hardware
	m_pDevice->FlushAndWaitForGPU();
	m_pDevice->GetDeviceContext()->GetCoreGraphicsCommandListPool().GetFences().GetLastCompletedFenceValues(FrameFenceValues);

	// Give up the buffers, they are passed to the release heap now
	m_pDX12SwapChain->ForfeitBuffers();

	// Clear the release heap of all pending releases, the buffers are gone now
	m_pDevice->GetDX12Device()->FlushReleaseHeap(FrameFenceValues, FrameFenceNulls);

	HRESULT res = m_pDX12SwapChain->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
	if (res == S_OK)
	{
		m_pDX12SwapChain->AcquireBuffers();
	}

	return res;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::ResizeTarget(
  _In_ const DXGI_MODE_DESC* pNewTargetParameters)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->ResizeTarget(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetContainingOutput(
  _Out_ IDXGIOutput** ppOutput)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetContainingOutput(ppOutput);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetFrameStatistics(
  _Out_ DXGI_FRAME_STATISTICS* pStats)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetFrameStatistics(pStats);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetLastPresentCount(
  _Out_ UINT* pLastPresentCount)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetDXGISwapChain()->GetLastPresentCount(pLastPresentCount);
}

/* IDXGISwapChain1 implementation */

/* IDXGISwapChain2 implementation */

/* IDXGISwapChain3 implementation */

UINT STDMETHODCALLTYPE CCryDX12SwapChain::GetCurrentBackBufferIndex(
  void)
{
	DX12_FUNC_LOG
	return m_pDX12SwapChain->GetCurrentBackbufferIndex();
}
