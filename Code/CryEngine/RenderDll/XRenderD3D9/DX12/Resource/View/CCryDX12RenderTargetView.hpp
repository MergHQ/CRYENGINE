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
#ifndef __CCRYDX12RENDERTARGETVIEW__
	#define __CCRYDX12RENDERTARGETVIEW__

	#include "DX12/Resource/CCryDX12View.hpp"

class CCryDX12RenderTargetView : public CCryDX12View<ID3D11RenderTargetView>
{
public:
	DX12_OBJECT(CCryDX12RenderTargetView, CCryDX12View<ID3D11RenderTargetView> );

	static CCryDX12RenderTargetView* Create(CCryDX12Device* pDevice, ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc);

	virtual ~CCryDX12RenderTargetView();

	#pragma region /* ID3D11RenderTargetView implementation */

	virtual void STDMETHODCALLTYPE GetDesc(
	  _Out_ D3D11_RENDER_TARGET_VIEW_DESC* pDesc) final
	{
		if (pDesc)
		{
			*pDesc = m_Desc11;
		}
	}

	#pragma endregion

	template<class T>
	void BeginResourceStateTransition(T* pCmdList)
	{
		pCmdList->BeginResourceStateTransition(pCmdList->PatchRenderTarget(GetDX12View().GetDX12Resource()), GetDX12View(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	template<class T>
	void EndResourceStateTransition(T* pCmdList)
	{
		pCmdList->EndResourceStateTransition(pCmdList->PatchRenderTarget(GetDX12View().GetDX12Resource()), GetDX12View(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

protected:
	CCryDX12RenderTargetView(ID3D11Resource* pResource11, const D3D11_RENDER_TARGET_VIEW_DESC& rDesc11, ID3D12Resource* pResource12, const D3D12_RENDER_TARGET_VIEW_DESC& rDesc12);

	CCryDX12RenderTargetView(ID3D11Resource* pResource11, const D3D11_RENDER_TARGET_VIEW_DESC& rDesc11, ID3D12Resource* pResource12);

private:
	D3D11_RENDER_TARGET_VIEW_DESC m_Desc11;
};

#endif // __CCRYDX12RENDERTARGETVIEW__
