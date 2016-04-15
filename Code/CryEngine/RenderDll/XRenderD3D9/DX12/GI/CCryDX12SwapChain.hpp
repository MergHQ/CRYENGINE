// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     03/02/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __CCRYDX12GISWAPCHAIN__
	#define __CCRYDX12GISWAPCHAIN__

	#include "DX12/CCryDX12Object.hpp"
	#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"

	#include "DX12/API/DX12SwapChain.hpp"

class CCryDX12SwapChain : public CCryDX12GIObject<IDXGISwapChain3>
{
public:
	DX12_OBJECT(CCryDX12SwapChain, CCryDX12GIObject<IDXGISwapChain3> );

	static CCryDX12SwapChain* Create(CCryDX12Device* pDevice, IDXGIFactory4* factory, DXGI_SWAP_CHAIN_DESC* pDesc);

	virtual ~CCryDX12SwapChain();

	ILINE CCryDX12Device* GetDevice() const
	{
		return m_pDevice;
	}

	ILINE IDXGISwapChain3* GetDXGISwapChain() const
	{
		return m_pDX12SwapChain->GetDXGISwapChain();
	}

	ILINE NCryDX12::CSwapChain* GetDX12SwapChain()
	{
		return m_pDX12SwapChain;
	}

	#pragma region /* IDXGIDeviceSubObject implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDevice(
	  _In_ REFIID riid,
	  _Out_ void** ppDevice) final;

	#pragma endregion

	#pragma region /* IDXGISwapChain implementation */

	virtual HRESULT STDMETHODCALLTYPE Present(
	  UINT SyncInterval,
	  UINT Flags) final;

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(
	  UINT Buffer,
	  _In_ REFIID riid,
	  _Out_ void** ppSurface) final;

	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
	  BOOL Fullscreen,
	  _In_opt_ IDXGIOutput* pTarget) final;

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
	  _Out_opt_ BOOL* pFullscreen,
	  _Out_opt_ IDXGIOutput** ppTarget) final;

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
	  _Out_ DXGI_SWAP_CHAIN_DESC* pDesc) final;

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
	  UINT BufferCount,
	  UINT Width,
	  UINT Height,
	  DXGI_FORMAT NewFormat,
	  UINT SwapChainFlags) final;

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
	  _In_ const DXGI_MODE_DESC* pNewTargetParameters) final;

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
	  _Out_ IDXGIOutput** ppOutput) final;

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
	  _Out_ DXGI_FRAME_STATISTICS* pStats) final;

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
	  _Out_ UINT* pLastPresentCount) final;

	#pragma endregion

	#pragma region /* IDXGISwapChain1 implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDesc1(
	  /* [annotation][out] */
	  _Out_ DXGI_SWAP_CHAIN_DESC1* pDesc) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(
	  /* [annotation][out] */
	  _Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetHwnd(
	  /* [annotation][out] */
	  _Out_ HWND* pHwnd) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(
	  /* [annotation][in] */
	  _In_ REFIID refiid,
	  /* [annotation][out] */
	  _COM_Outptr_ void** ppUnk) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Present1(
	  /* [in] */ UINT SyncInterval,
	  /* [in] */ UINT PresentFlags,
	  /* [annotation][in] */
	  _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() final
	{
		DX12_NOT_IMPLEMENTED
		return FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(
	  /* [annotation][out] */
	  _Out_ IDXGIOutput** ppRestrictToOutput) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(
	  /* [annotation][in] */
	  _In_ const DXGI_RGBA* pColor) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(
	  /* [annotation][out] */
	  _Out_ DXGI_RGBA* pColor) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetRotation(
	  /* [annotation][in] */
	  _In_ DXGI_MODE_ROTATION Rotation) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRotation(
	  /* [annotation][out] */
	  _Out_ DXGI_MODE_ROTATION* pRotation) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	#pragma endregion

	#pragma region /* IDXGISwapChain2 implementation */

	virtual HRESULT STDMETHODCALLTYPE SetSourceSize(
	  UINT Width,
	  UINT Height) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetSourceSize(
	  /* [annotation][out] */
	  _Out_ UINT* pWidth,
	  /* [annotation][out] */
	  _Out_ UINT* pHeight) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(
	  UINT MaxLatency) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(
	  /* [annotation][out] */
	  _Out_ UINT* pMaxLatency) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() final
	{
		DX12_NOT_IMPLEMENTED
		return 0;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(
	  const DXGI_MATRIX_3X2_F* pMatrix) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(
	  /* [annotation][out] */
	  _Out_ DXGI_MATRIX_3X2_F* pMatrix) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	#pragma endregion

	#pragma region /* IDXGISwapChain3 implementation */

	virtual UINT STDMETHODCALLTYPE    GetCurrentBackBufferIndex() final;

	virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(
	  /* [annotation][in] */
	  _In_ DXGI_COLOR_SPACE_TYPE ColorSpace,
	  /* [annotation][out] */
	  _Out_ UINT* pColorSpaceSupport) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(
	  /* [annotation][in] */
	  _In_ DXGI_COLOR_SPACE_TYPE ColorSpace) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(
	  /* [annotation][in] */
	  _In_ UINT BufferCount,
	  /* [annotation][in] */
	  _In_ UINT Width,
	  /* [annotation][in] */
	  _In_ UINT Height,
	  /* [annotation][in] */
	  _In_ DXGI_FORMAT Format,
	  /* [annotation][in] */
	  _In_ UINT SwapChainFlags,
	  /* [annotation][in] */
	  _In_reads_(BufferCount)  const UINT* pCreationNodeMask,
	  /* [annotation][in] */
	  _In_reads_(BufferCount)  IUnknown* const* ppPresentQueue) final
	{
		DX12_NOT_IMPLEMENTED
		return S_OK;
	}

	#pragma endregion

protected:
	CCryDX12SwapChain(CCryDX12Device* pDevice, NCryDX12::CSwapChain* pSwapChain);

private:
	CCryDX12Device*       m_pDevice;

	NCryDX12::CSwapChain* m_pDX12SwapChain;
};

#endif // __CCRYDX12GISWAPCHAIN__
