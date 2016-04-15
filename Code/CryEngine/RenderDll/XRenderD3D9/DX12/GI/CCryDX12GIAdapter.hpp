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
#ifndef __CCRYDX12GIADAPTER__
	#define __CCRYDX12GIADAPTER__

	#include "DX12/CCryDX12Object.hpp"
	#include "CCryDX12GIFactory.hpp"

class CCryDX12GIAdapter : public CCryDX12GIObject<IDXGIAdapter3>
{
public:
	DX12_OBJECT(CCryDX12GIAdapter, CCryDX12GIObject<IDXGIAdapter3> );

	static CCryDX12GIAdapter* Create(CCryDX12GIFactory* factory, UINT Adapter);

	virtual ~CCryDX12GIAdapter();

	ILINE CCryDX12GIFactory* GetFactory() const     { return m_pFactory; }
	ILINE IDXGIAdapter3*     GetDXGIAdapter() const { return m_pDXGIAdapter3; }

	#pragma region /* IDXGIAdapter implementation */

	virtual HRESULT STDMETHODCALLTYPE EnumOutputs(
	  /* [in] */ UINT Output,
	  /* [annotation][out][in] */
	  _COM_Outptr_ IDXGIOutput** ppOutput) final;

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
	  /* [annotation][out] */
	  _Out_ DXGI_ADAPTER_DESC* pDesc) final
	{
		return m_pDXGIAdapter3->GetDesc(pDesc);
	}

	virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(
	  /* [annotation][in] */
	  _In_ REFGUID InterfaceName,
	  /* [annotation][out] */
	  _Out_ LARGE_INTEGER* pUMDVersion) final
	{
		return m_pDXGIAdapter3->CheckInterfaceSupport(InterfaceName, pUMDVersion);
	}

	#pragma endregion

	#pragma region /* IDXGIAdapter1 implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDesc1(
	  _Out_ DXGI_ADAPTER_DESC1* pDesc) final
	{
		return m_pDXGIAdapter3->GetDesc1(pDesc);
	}

	#pragma endregion

	#pragma region /* IDXGIAdapter2 implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDesc2(
	  /* [annotation][out] */
	  _Out_ DXGI_ADAPTER_DESC2* pDesc) final
	{
		return m_pDXGIAdapter3->GetDesc2(pDesc);
	}

	#pragma endregion

	#pragma region /* IDXGIAdapter3 implementation */

	virtual HRESULT STDMETHODCALLTYPE RegisterHardwareContentProtectionTeardownStatusEvent(
	  /* [annotation][in] */
	  _In_ HANDLE hEvent,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final
	{
		return m_pDXGIAdapter3->RegisterHardwareContentProtectionTeardownStatusEvent(hEvent, pdwCookie);
	}

	virtual void STDMETHODCALLTYPE UnregisterHardwareContentProtectionTeardownStatus(
	  /* [annotation][in] */
	  _In_ DWORD dwCookie) final
	{
		return m_pDXGIAdapter3->UnregisterHardwareContentProtectionTeardownStatus(dwCookie);
	}

	virtual HRESULT STDMETHODCALLTYPE QueryVideoMemoryInfo(
	  /* [annotation][in] */
	  _In_ UINT NodeIndex,
	  /* [annotation][in] */
	  _In_ DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
	  /* [annotation][out] */
	  _Out_ DXGI_QUERY_VIDEO_MEMORY_INFO* pVideoMemoryInfo) final
	{
		return m_pDXGIAdapter3->QueryVideoMemoryInfo(NodeIndex, MemorySegmentGroup, pVideoMemoryInfo);
	}

	virtual HRESULT STDMETHODCALLTYPE SetVideoMemoryReservation(
	  /* [annotation][in] */
	  _In_ UINT NodeIndex,
	  /* [annotation][in] */
	  _In_ DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
	  /* [annotation][in] */
	  _In_ UINT64 Reservation) final
	{
		return m_pDXGIAdapter3->SetVideoMemoryReservation(NodeIndex, MemorySegmentGroup, Reservation);
	}

	virtual HRESULT STDMETHODCALLTYPE RegisterVideoMemoryBudgetChangeNotificationEvent(
	  /* [annotation][in] */
	  _In_ HANDLE hEvent,
	  /* [annotation][out] */
	  _Out_ DWORD* pdwCookie) final
	{
		return m_pDXGIAdapter3->RegisterVideoMemoryBudgetChangeNotificationEvent(hEvent, pdwCookie);
	}

	virtual void STDMETHODCALLTYPE UnregisterVideoMemoryBudgetChangeNotification(
	  /* [annotation][in] */
	  _In_ DWORD dwCookie) final
	{
		return m_pDXGIAdapter3->UnregisterVideoMemoryBudgetChangeNotification(dwCookie);
	}

	#pragma endregion

protected:
	CCryDX12GIAdapter(CCryDX12GIFactory* pFactory, IDXGIAdapter3* pAdapter);

private:
	CCryDX12GIFactory* m_pFactory;

	IDXGIAdapter3*     m_pDXGIAdapter3;
};

#endif // __CCRYDX12GIADAPTER__
