// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"

class CSwapChain
{
private:
	_smart_ptr<DXGISwapChain> m_pSwapChain;
	DXGIOutput*               m_pOutput;
	DXGI_SURFACE_DESC         m_surfaceDesc;

	uint32                    m_refreshRateNumerator = 0;
	uint32                    m_refreshRateDenominator = 0;

	CSwapChain(_smart_ptr<DXGISwapChain>&& pSwapChain, DXGIOutput* pOutput) : m_pSwapChain(std::move(pSwapChain)), m_pOutput(pOutput) { ReadSwapChainSurfaceDesc(); }

	void ReadSwapChainSurfaceDesc();

public:
	CSwapChain() = default;

	CSwapChain(CSwapChain &&o) noexcept
		: m_pSwapChain(std::move(o.m_pSwapChain))
		, m_pOutput(o.m_pOutput)
		, m_surfaceDesc(o.m_surfaceDesc)
		, m_refreshRateNumerator(o.m_refreshRateNumerator)
		, m_refreshRateDenominator(o.m_refreshRateDenominator)
	{}
	CSwapChain &operator=(CSwapChain &&o) noexcept
	{
		m_pSwapChain = std::move(o.m_pSwapChain);
		m_pOutput = o.m_pOutput;
		m_surfaceDesc = o.m_surfaceDesc;
		m_refreshRateNumerator = o.m_refreshRateNumerator;
		m_refreshRateDenominator = o.m_refreshRateDenominator;

		return *this;
	}

	void ResizeSwapChain(uint32_t buffers, uint32_t width, uint32_t height, bool isFullscreen, bool isMainContext, bool bResizeTarget = false);

#ifdef CRY_RENDERER_GNM
	HRESULT                   Present(CGnmGraphicsCommandList* pCommandList, CGnmSwapChain::EFlipMode flipMode);
#else
	HRESULT                   Present(unsigned int syncInterval, unsigned int flags);
#endif

	static DXGI_FORMAT        GetSwapChainFormat();
	_smart_ptr<DXGISwapChain> GetSwapChain() const { return m_pSwapChain; }
	const DXGI_SURFACE_DESC&  GetSurfaceDesc() const { return m_surfaceDesc; }

#if (CRY_RENDERER_DIRECT3D >= 110) || (CRY_RENDERER_VULKAN >= 10)
	DXGI_SWAP_CHAIN_DESC      GetDesc() const { DXGI_SWAP_CHAIN_DESC scDesc; ZeroStruct(scDesc); m_pSwapChain->GetDesc(&scDesc); return scDesc; }
#endif

	uint32                    GetRefreshRateNumerator() const { return m_refreshRateNumerator; }
	uint32                    GetRefreshRateDenominator() const { return m_refreshRateDenominator; }

public:
#if CRY_PLATFORM_DURANGO
#if (CRY_RENDERER_DIRECT3D >= 120)
	static CSwapChain        CreateSwapChain(IDXGIFactory2ToCall* pDXGIFactory, ID3D12Device* pD3D12Device, CCryDX12Device* pDX12Device, uint32_t width, uint32_t height);
#else
	static CSwapChain        CreateSwapChain(IDXGIFactory2ToCall* pDXGIFactory, ID3D11Device* pD3D11Device, uint32_t width, uint32_t height);
#endif
#elif CRY_RENDERER_GNM
	static CSwapChain        CreateSwapChain(uint32_t width, uint32_t height);
#else
	static CSwapChain        CreateSwapChain(CRY_HWND hWnd, DXGIOutput* pOutput, uint32_t width, uint32_t height, bool isFullscreen, bool isMainContext, bool vsync);
#endif

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
	static void UserOverrideDisplayProperties(DXGI_MODE_DESC& desc);
#endif
};
