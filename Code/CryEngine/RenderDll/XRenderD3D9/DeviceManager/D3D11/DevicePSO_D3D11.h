// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../../DeviceManager/DevicePSO.h"

class CDeviceGraphicsPSO_DX11 : public CDeviceGraphicsPSO
{
public:
	CDeviceGraphicsPSO_DX11();

	virtual EInitResult Init(const CDeviceGraphicsPSODesc& psoDesc) final;

	_smart_ptr<ID3D11RasterizerState>                 m_pRasterizerState;
	_smart_ptr<ID3D11BlendState>                      m_pBlendState;
	_smart_ptr<ID3D11DepthStencilState>               m_pDepthStencilState;
	_smart_ptr<ID3D11InputLayout>                     m_pInputLayout;

	std::array<void*, eHWSC_Num>                      m_pDeviceShaders;

	// Resources required by the PSO
	std::array< std::bitset<ResourceSetMaxSrvCount>, eHWSC_Num> m_requiredSRVs;
	std::array< std::bitset<ResourceSetMaxUavCount>, eHWSC_Num> m_requiredUAVs;
	std::array< std::bitset<ResourceSetMaxSamplerCount>, eHWSC_Num> m_requiredSamplers;

	// Do we still need these?
	uint64           m_ShaderFlags_RT;
	uint32           m_ShaderFlags_MD;
	EVertexModifier  m_ShaderFlags_MDV;

	D3DPrimitiveType m_PrimitiveTopology;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX11 : public CDeviceComputePSO
{
public:
	CDeviceComputePSO_DX11();

	virtual bool Init(const CDeviceComputePSODesc& psoDesc) final;

	void* m_pDeviceShader;
};