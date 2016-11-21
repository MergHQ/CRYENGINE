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
#ifndef __CRYDX12SHADERRESOURCEVIEW__
	#define __CRYDX12SHADERRESOURCEVIEW__

	#include "DX12/Resource/CCryDX12View.hpp"

class CCryDX12ShaderResourceView : public CCryDX12View<ID3D11ShaderResourceViewToImplement>
{
public:
	DX12_OBJECT(CCryDX12ShaderResourceView, CCryDX12View<ID3D11ShaderResourceViewToImplement> );

	static CCryDX12ShaderResourceView* Create(CCryDX12Device* pDevice, ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc);

	virtual ~CCryDX12ShaderResourceView();

	#pragma region /* ID3D11ShaderResourceView implementation */

	virtual void STDMETHODCALLTYPE GetDesc(
	  _Out_ D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc) final;

	#pragma endregion

	template<class T>
	void BeginResourceStateTransition(T* pCmdList)
	{
		pCmdList->BeginResourceStateTransition(pCmdList->PatchRenderTarget(GetDX12View().GetDX12Resource()), GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	template<class T>
	void EndResourceStateTransition(T* pCmdList)
	{
		pCmdList->EndResourceStateTransition(pCmdList->PatchRenderTarget(GetDX12View().GetDX12Resource()), GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

protected:
	CCryDX12ShaderResourceView(ID3D11Resource* pResource11, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc11, ID3D12Resource* pResource12, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc12);

private:
	D3D11_SHADER_RESOURCE_VIEW_DESC m_Desc11;
};

#endif
