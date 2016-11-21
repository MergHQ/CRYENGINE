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
#ifndef __CCRYDX12DEPTHSTENCILVIEW__
	#define __CCRYDX12DEPTHSTENCILVIEW__

	#include "DX12/Resource/CCryDX12View.hpp"

class CCryDX12DepthStencilView : public CCryDX12View<ID3D11DepthStencilViewToImplement>
{
public:
	DX12_OBJECT(CCryDX12DepthStencilView, CCryDX12View<ID3D11DepthStencilViewToImplement> );

	static CCryDX12DepthStencilView* Create(CCryDX12Device* pDevice, ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc);

	virtual ~CCryDX12DepthStencilView();

	#pragma region /* ID3D11DepthStencilView implementation */

	virtual void STDMETHODCALLTYPE GetDesc(
	  _Out_ D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc) final;

	#pragma endregion

	template<class T>
	void BeginResourceStateTransition(T* pCmdList)
	{
		pCmdList->BeginResourceStateTransition(GetDX12View().GetDX12Resource(), GetDX12View(), m_Desc11.Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	template<class T>
	void EndResourceStateTransition(T* pCmdList)
	{
		pCmdList->EndResourceStateTransition(GetDX12View().GetDX12Resource(), GetDX12View(), m_Desc11.Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

protected:
	CCryDX12DepthStencilView(ID3D11Resource* pResource11, const D3D11_DEPTH_STENCIL_VIEW_DESC& rDesc11, ID3D12Resource* pResource12, const D3D12_DEPTH_STENCIL_VIEW_DESC& rDesc12);

	CCryDX12DepthStencilView(ID3D11Resource* pResource11, const D3D11_DEPTH_STENCIL_VIEW_DESC& rDesc11, ID3D12Resource* pResource12);

private:
	D3D11_DEPTH_STENCIL_VIEW_DESC m_Desc11;
};

#endif // __CCRYDX12DEPTHSTENCILVIEW__
