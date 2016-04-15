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
#ifndef __CCRYDX12GIOUTPUT__
	#define __CCRYDX12GIOUTPUT__

	#include "DX12/CCryDX12Object.hpp"
	#include "CCryDX12GIAdapter.hpp"

class CCryDX12GIOutput : public CCryDX12GIObject<IDXGIOutput4>
{
public:
	DX12_OBJECT(CCryDX12GIOutput, CCryDX12GIObject<IDXGIOutput4> );

	static CCryDX12GIOutput* Create(CCryDX12GIAdapter* Adapter, UINT Output);

	virtual ~CCryDX12GIOutput();

	ILINE CCryDX12GIAdapter* GetAdapter() const    { return m_pAdapter; }
	ILINE IDXGIOutput4*      GetDXGIOutput() const { return m_pDXGIOutput4; }

	#pragma region /* IDXGIOutput implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
	  /* [annotation][out] */
	  _Out_ DXGI_OUTPUT_DESC* pDesc) final
	{
		return m_pDXGIOutput4->GetDesc(pDesc);
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList(
	  /* [in] */ DXGI_FORMAT EnumFormat,
	  /* [in] */ UINT Flags,
	  /* [annotation][out][in] */
	  _Inout_ UINT* pNumModes,
	  /* [annotation][out] */
	  _Out_writes_to_opt_(*pNumModes, *pNumModes)  DXGI_MODE_DESC* pDesc) final
	{
		return m_pDXGIOutput4->GetDisplayModeList(EnumFormat, Flags, pNumModes, pDesc);
	}

	virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode(
	  /* [annotation][in] */
	  _In_ const DXGI_MODE_DESC* pModeToMatch,
	  /* [annotation][out] */
	  _Out_ DXGI_MODE_DESC* pClosestMatch,
	  /* [annotation][in] */
	  _In_opt_ IUnknown* pConcernedDevice) final;

	virtual HRESULT STDMETHODCALLTYPE WaitForVBlank(void) final
	{
		return m_pDXGIOutput4->WaitForVBlank();
	}

	virtual HRESULT STDMETHODCALLTYPE TakeOwnership(
	  /* [annotation][in] */
	  _In_ IUnknown* pDevice,
	  BOOL Exclusive) final;

	virtual void STDMETHODCALLTYPE ReleaseOwnership(void) final
	{
		return m_pDXGIOutput4->ReleaseOwnership();
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities(
	  /* [annotation][out] */
	  _Out_ DXGI_GAMMA_CONTROL_CAPABILITIES* pGammaCaps) final
	{
		return m_pDXGIOutput4->GetGammaControlCapabilities(pGammaCaps);
	}

	virtual HRESULT STDMETHODCALLTYPE SetGammaControl(
	  /* [annotation][in] */
	  _In_ const DXGI_GAMMA_CONTROL* pArray) final
	{
		return m_pDXGIOutput4->SetGammaControl(pArray);
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControl(
	  /* [annotation][out] */
	  _Out_ DXGI_GAMMA_CONTROL* pArray) final
	{
		return m_pDXGIOutput4->GetGammaControl(pArray);
	}

	virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface(
	  /* [annotation][in] */
	  _In_ IDXGISurface* pScanoutSurface) final
	{
		return m_pDXGIOutput4->SetDisplaySurface(pScanoutSurface);
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData(
	  /* [annotation][in] */
	  _In_ IDXGISurface* pDestination) final
	{
		return m_pDXGIOutput4->GetDisplaySurfaceData(pDestination);
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
	  /* [annotation][out] */
	  _Out_ DXGI_FRAME_STATISTICS* pStats) final
	{
		return m_pDXGIOutput4->GetFrameStatistics(pStats);
	}

	#pragma endregion

	#pragma region /* IDXGIOutput1 implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList1(
	  /* [in] */ DXGI_FORMAT EnumFormat,
	  /* [in] */ UINT Flags,
	  /* [annotation][out][in] */
	  _Inout_ UINT* pNumModes,
	  /* [annotation][out] */
	  _Out_writes_to_opt_(*pNumModes, *pNumModes)  DXGI_MODE_DESC1* pDesc) final
	{
		return m_pDXGIOutput4->GetDisplayModeList1(EnumFormat, Flags, pNumModes, pDesc);
	}

	virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode1(
	  /* [annotation][in] */
	  _In_ const DXGI_MODE_DESC1* pModeToMatch,
	  /* [annotation][out] */
	  _Out_ DXGI_MODE_DESC1* pClosestMatch,
	  /* [annotation][in] */
	  _In_opt_ IUnknown* pConcernedDevice) final;

	virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData1(
	  /* [annotation][in] */
	  _In_ IDXGIResource* pDestination) final
	{
		return m_pDXGIOutput4->GetDisplaySurfaceData1(pDestination);
	}

	virtual HRESULT STDMETHODCALLTYPE DuplicateOutput(
	  /* [annotation][in] */
	  _In_ IUnknown* pDevice,
	  /* [annotation][out] */
	  _COM_Outptr_ IDXGIOutputDuplication** ppOutputDuplication) final
	{
		return m_pDXGIOutput4->DuplicateOutput(pDevice, ppOutputDuplication);
	}

	#pragma endregion

	#pragma region /* IDXGIOutput2 implementation */

	virtual BOOL STDMETHODCALLTYPE SupportsOverlays(void) final
	{
		return m_pDXGIOutput4->SupportsOverlays();
	}

	#pragma endregion

	#pragma region /* IDXGIOutput3 implementation */

	virtual HRESULT STDMETHODCALLTYPE CheckOverlaySupport(
	  /* [annotation][in] */
	  _In_ DXGI_FORMAT EnumFormat,
	  /* [annotation][out] */
	  _In_ IUnknown* pConcernedDevice,
	  /* [annotation][out] */
	  _Out_ UINT* pFlags) final;

	#pragma endregion

	#pragma region /* IDXGIOutput4 implementation */

	virtual HRESULT STDMETHODCALLTYPE CheckOverlayColorSpaceSupport(
	  /* [annotation][in] */
	  _In_ DXGI_FORMAT Format,
	  /* [annotation][in] */
	  _In_ DXGI_COLOR_SPACE_TYPE ColorSpace,
	  /* [annotation][in] */
	  _In_ IUnknown* pConcernedDevice,
	  /* [annotation][out] */
	  _Out_ UINT* pFlags) final;

	#pragma endregion

protected:
	CCryDX12GIOutput(CCryDX12GIAdapter* pAdapter, IDXGIOutput4* pOutput);

private:
	CCryDX12GIAdapter* m_pAdapter;

	IDXGIOutput4*      m_pDXGIOutput4;
};

#endif // __CCRYDX12GIOUTPUT__
