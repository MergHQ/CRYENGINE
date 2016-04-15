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
#ifndef __CCRYDX12GIFACTORY__
	#define __CCRYDX12GIFACTORY__

	#include "DX12/CCryDX12Object.hpp"

class CCryDX12GIFactory : public CCryDX12GIObject<IDXGIFactory4>
{
public:
	DX12_OBJECT(CCryDX12GIFactory, CCryDX12GIObject<IDXGIFactory4> );

	static CCryDX12GIFactory* Create();

	virtual ~CCryDX12GIFactory();

	IDXGIFactory4* GetDXGIFactory() const { return m_pDXGIFactory4; }

	#pragma region /* IDXGIFactory implementation */

	virtual HRESULT STDMETHODCALLTYPE EnumAdapters(
	  UINT Adapter,
	  _Out_ IDXGIAdapter** ppAdapter) final;

	virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(
	  HWND WindowHandle,
	  UINT Flags) final;

	virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(
	  _Out_ HWND* pWindowHandle) final;

	virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(
	  _In_ IUnknown* pDevice,
	  _In_ DXGI_SWAP_CHAIN_DESC* pDesc,
	  _Out_ IDXGISwapChain** ppSwapChain) final;

	virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(
	  HMODULE Module,
	  _Out_ IDXGIAdapter** ppAdapter) final;

	#pragma endregion

	#pragma region /* IDXGIFactory1 implementation */

	virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(
	  UINT Adapter,
	  _Out_ IDXGIAdapter1** ppAdapter) final;

	virtual BOOL STDMETHODCALLTYPE IsCurrent() final;

	#pragma endregion

	#pragma region /* IDXGIFactory2 implementation */

	virtual BOOL STDMETHODCALLTYPE    IsWindowedStereoEnabled() final { abort(); return FALSE; }

	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
	  /* [annotation][in] */
	  _In_ IUnknown* pDevice,
	  /* [annotation][in] */
	  _In_ HWND hWnd,
	  /* [annotation][in] */
	  _In_ const DXGI_SWAP_CHAIN_DESC1* pDesc,
	  /* [annotation][in] */
	  _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
	  /* [annotation][in] */
	  _In_opt_ IDXGIOutput* pRestrictToOutput,
	  /* [annotation][out] */
	  _COM_Outptr_ IDXGISwapChain1** ppSwapChain) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(
	  /* [annotation][in] */
	  _In_ IUnknown* pDevice,
	  /* [annotation][in] */
	  _In_ IUnknown* pWindow,
	  /* [annotation][in] */
	  _In_ const DXGI_SWAP_CHAIN_DESC1* pDesc,
	  /* [annotation][in] */
	  _In_opt_ IDXGIOutput* pRestrictToOutput,
	  /* [annotation][out] */
	  _COM_Outptr_ IDXGISwapChain1** ppSwapChain) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(
	  /* [annotation] */
	  _In_ HANDLE hResource,
	  /* [annotation] */
	  _Out_ LUID* pLuid) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(
	  /* [annotation][in] */
	  _In_ HWND WindowHandle,
	  /* [annotation][in] */
	  _In_ UINT wMsg,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(
	  /* [annotation][in] */
	  _In_ HANDLE hEvent,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final { abort(); return S_OK; }

	virtual void STDMETHODCALLTYPE UnregisterStereoStatus(
	  /* [annotation][in] */
	  _In_ DWORD dwCookie) final { abort(); }

	virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(
	  /* [annotation][in] */
	  _In_ HWND WindowHandle,
	  /* [annotation][in] */
	  _In_ UINT wMsg,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(
	  /* [annotation][in] */
	  _In_ HANDLE hEvent,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final { abort(); return S_OK; }

	virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus(
	  /* [annotation][in] */
	  _In_ DWORD dwCookie) final { abort(); }

	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(
	  /* [annotation][in] */
	  _In_ IUnknown* pDevice,
	  /* [annotation][in] */
	  _In_ const DXGI_SWAP_CHAIN_DESC1* pDesc,
	  /* [annotation][in] */
	  _In_opt_ IDXGIOutput* pRestrictToOutput,
	  /* [annotation][out] */
	  _COM_Outptr_ IDXGISwapChain1** ppSwapChain) final { abort(); return S_OK; }

	#pragma endregion

	#pragma region /* IDXGIFactory3 implementation */

	virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) final { abort(); return 0; }

	#pragma endregion

	#pragma region /* IDXGIFactory4 implementation */

	virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(
	  /* [annotation] */
	  _In_ LUID AdapterLuid,
	  /* [annotation] */
	  _In_ REFIID riid,
	  /* [annotation] */
	  _COM_Outptr_ void** ppvAdapter) final { abort(); return S_OK; }

	virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter(
	  /* [annotation] */
	  _In_ REFIID riid,
	  /* [annotation] */
	  _COM_Outptr_ void** ppvAdapter) final { abort(); return S_OK; }

	#pragma endregion

protected:
	CCryDX12GIFactory(IDXGIFactory4* pDXGIFactory4);

private:
	DX12_PTR(IDXGIFactory4) m_pDXGIFactory4;
};

#endif // __CRYDX12GIFACTORY__
