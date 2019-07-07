// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "../DeviceObjects.h"
#include "../../../Common/ReverseDepth.h"

#include "DeviceResourceSet_D3D11.h"
#include "DevicePSO_D3D11.h"
#include "DeviceObjects_D3D11.h"

CDeviceGraphicsPSO_DX11::CDeviceGraphicsPSO_DX11()
{
	m_PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_ShaderFlags_RT = 0;
	m_ShaderFlags_MD = 0;
	m_ShaderFlags_MDV = MDV_NONE;
}

CDeviceGraphicsPSO::EInitResult CDeviceGraphicsPSO_DX11::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
	m_isValid = false;
	m_updateCount++;

	m_pRasterizerState = nullptr;
	m_pBlendState = nullptr;
	m_pDepthStencilState = nullptr;
	m_pInputLayout = nullptr;

	m_requiredSRVs.fill(0);
	m_requiredUAVs.fill(0);
	m_requiredSamplers.fill(0);

	SDeviceObjectHelpers::THwShaderInfo hwShaders;
	EShaderStage validShaderStages = SDeviceObjectHelpers::GetShaderInstanceInfo(hwShaders, psoDesc.m_pShader, psoDesc.m_technique,
		psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);

	if (validShaderStages == EShaderStage_None)
		return EInitResult::Failure;

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_NumGfx; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		m_pDeviceShaders[shaderClass]     = hwShaders[shaderClass].pDeviceShader;
		m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	D3D11_BLEND_DESC blendDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

	CDeviceStatesManagerDX11 *const pDeviceStatesManager = CDeviceStatesManagerDX11::GetInstance();

	uint32 rasterStateIndex = pDeviceStatesManager->GetOrCreateRasterState(rasterizerDesc);
	uint32 blendStateIndex = pDeviceStatesManager->GetOrCreateBlendState(blendDesc);
	uint32 depthStateIndex = pDeviceStatesManager->GetOrCreateDepthState(depthStencilDesc);

	if (rasterStateIndex == uint32(-1) || blendStateIndex == uint32(-1) || depthStateIndex == uint32(-1))
		return EInitResult::Failure;

	m_pDepthStencilState = pDeviceStatesManager->GetDepthState(depthStateIndex).pState;
	m_pRasterizerState = pDeviceStatesManager->GetRasterState(rasterStateIndex).pState;
	m_pBlendState = pDeviceStatesManager->GetBlendState(blendStateIndex).pState;

	// input layout
	m_pInputLayout = nullptr;
	if (psoDesc.m_VertexFormat != EDefaultInputLayouts::Empty)
	{
		if (psoDesc.m_VertexFormat != EDefaultInputLayouts::Unspecified)
		{
			if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
			{
				EStreamMasks streamMask = psoDesc.CombineVertexStreamMasks(pVsInstance->m_VStreamMask_Decl, psoDesc.m_ObjectStreamMask);

				const auto inputLayoutPair = CDeviceObjectFactory::GetOrCreateInputLayout(&pVsInstance->m_Shader, streamMask, psoDesc.m_VertexFormat);
				m_pInputLayout = inputLayoutPair->second;
			}
		}

		if (!m_pInputLayout)
			return EInitResult::Failure;
	}

	// init required data bit fields
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_NumGfx; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (auto pShaderInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[shaderClass].pHwShaderInstance))
		{
			// Sampler ranges
			for (const auto& smp : pShaderInstance->m_Samplers)
				m_requiredSamplers[shaderClass][smp.m_dwCBufSlot] = true;

			// SRV ranges
			for (const auto& tex : pShaderInstance->m_Textures)
			{
				CRY_ASSERT(tex.m_dwCBufSlot < m_requiredSRVs[shaderClass].size());
				m_requiredSRVs[shaderClass][tex.m_dwCBufSlot] = true;
			}
		}
	}

	if (!ValidateShadersAndTopologyCombination(psoDesc, m_pHwShaderInstances))
		return EInitResult::ErrorShadersAndTopologyCombination;

	m_PrimitiveTopology = static_cast<D3DPrimitiveType>(psoDesc.m_PrimitiveType);

	m_ShaderFlags_RT = psoDesc.m_ShaderFlags_RT;
	m_ShaderFlags_MD = psoDesc.m_ShaderFlags_MD;
	m_ShaderFlags_MDV = psoDesc.m_ShaderFlags_MDV;

#if defined(ENABLE_PROFILING_CODE)
	m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
#endif

	m_isValid = true;
	return EInitResult::Success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceComputePSO_DX11::CDeviceComputePSO_DX11()
{
}

bool CDeviceComputePSO_DX11::Init(const CDeviceComputePSODesc& psoDesc)
{
	m_isValid = false;
	m_updateCount++;

	SDeviceObjectHelpers::THwShaderInfo hwShaders;
	EShaderStage validShaderStages = SDeviceObjectHelpers::GetShaderInstanceInfo(hwShaders, psoDesc.m_pShader, psoDesc.m_technique, 
		psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, MDV_NONE, nullptr, false);

	if (validShaderStages != EShaderStage_Compute)
		return false;

	m_pDeviceShader     = hwShaders[eHWSC_Compute].pDeviceShader;
	m_pHwShaderInstance = hwShaders[eHWSC_Compute].pHwShaderInstance;

	m_isValid = true;
	return true;
}
