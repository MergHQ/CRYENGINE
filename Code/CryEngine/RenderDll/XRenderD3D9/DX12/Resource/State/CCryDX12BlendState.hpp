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
#ifndef __CCRYDX12BLENDSTATE__
	#define __CCRYDX12BLENDSTATE__

	#include "DX12/Device/CCryDX12DeviceChild.hpp"

class CCryDX12BlendState : public CCryDX12DeviceChild<ID3D11BlendStateToImplement>
{
public:
	DX12_OBJECT(CCryDX12BlendState, CCryDX12DeviceChild<ID3D11BlendStateToImplement> );

	static CCryDX12BlendState* Create(const D3D11_BLEND_DESC* pBlendStateDesc);

	virtual ~CCryDX12BlendState();

	const D3D12_BLEND_DESC& GetD3D12BlendDesc() const
	{
		return m_Desc12;
	}

	#pragma region /* ID3D11BlendState implementation */

	virtual void STDMETHODCALLTYPE GetDesc(
	  _Out_ D3D11_BLEND_DESC* pDesc);

	#pragma endregion

protected:
	CCryDX12BlendState(const D3D11_BLEND_DESC& desc11, const D3D12_BLEND_DESC& desc12);

private:
	D3D11_BLEND_DESC m_Desc11;

	D3D12_BLEND_DESC m_Desc12;
};

#endif // __CCRYDX12BLENDSTATE__
