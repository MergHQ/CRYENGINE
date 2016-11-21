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
#ifndef __CCRYDX12RASTERIZERSTATE__
	#define __CCRYDX12RASTERIZERSTATE__

	#include "DX12/Device/CCryDX12DeviceChild.hpp"

class CCryDX12RasterizerState : public CCryDX12DeviceChild<ID3D11RasterizerStateToImplement>
{
public:
	DX12_OBJECT(CCryDX12RasterizerState, CCryDX12DeviceChild<ID3D11RasterizerStateToImplement> );

	static CCryDX12RasterizerState* Create(const D3D11_RASTERIZER_DESC* pRasterizerDesc);

	virtual ~CCryDX12RasterizerState();

	const D3D12_RASTERIZER_DESC& GetD3D12RasterizerDesc() const
	{
		return m_Desc12;
	}
	const D3D11_RASTERIZER_DESC& GetD3D11RasterizerDesc() const
	{
		return m_Desc11;
	}

	#pragma region /* ID3D11RasterizerState implementation */

	virtual void STDMETHODCALLTYPE GetDesc(
	  _Out_ D3D11_RASTERIZER_DESC* pDesc);

	#pragma endregion

protected:
	CCryDX12RasterizerState(const D3D11_RASTERIZER_DESC& desc11, const D3D12_RASTERIZER_DESC& desc12);

private:
	D3D11_RASTERIZER_DESC m_Desc11;
	D3D12_RASTERIZER_DESC m_Desc12;
};

#endif // __CCRYDX12RASTERIZERSTATE__
