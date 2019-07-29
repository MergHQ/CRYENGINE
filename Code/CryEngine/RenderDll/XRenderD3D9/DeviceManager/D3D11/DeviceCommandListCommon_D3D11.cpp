// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "../DeviceObjects.h"
#include "../../../Common/ReverseDepth.h"

#include "DeviceCommandListCommon_D3D11.h"
#include "DeviceObjects_D3D11.h"
#include "DeviceResourceSet_D3D11.h"
#include "DevicePSO_D3D11.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceTimestampGroup::CDeviceTimestampGroup()
	: m_numTimestamps(0)
	, m_pDisjointQuery(nullptr)
	, m_frequency(0L)
	, m_measurable(false)
	, m_measured(false)
{
	m_timestampQueries.fill(nullptr);
	m_timeValues.fill(0);
}

CDeviceTimestampGroup::~CDeviceTimestampGroup()
{
	SAFE_RELEASE(m_pDisjointQuery);
	for (uint32 i = 0; i < m_timestampQueries.size(); i++)
	{
		SAFE_RELEASE(m_timestampQueries[i]);
	}
}

void CDeviceTimestampGroup::Init()
{
	D3D11_QUERY_DESC queryTimestampDesc = { D3D11_QUERY_TIMESTAMP, 0 };
	D3D11_QUERY_DESC queryDisjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };

	for (uint32 i = 0; i < m_timestampQueries.size(); i++)
	{
		gcpRendD3D->GetDevice()->CreateQuery(&queryTimestampDesc, &m_timestampQueries[i]);
	}
	gcpRendD3D->GetDevice()->CreateQuery(&queryDisjointDesc, &m_pDisjointQuery);
}

void CDeviceTimestampGroup::BeginMeasurement()
{
	m_numTimestamps = 0;
	m_frequency = 0;
	m_measurable = false;
	m_measured = false;

	gcpRendD3D->GetDeviceContext()->Begin(m_pDisjointQuery);
}

void CDeviceTimestampGroup::EndMeasurement()
{
	gcpRendD3D->GetDeviceContext()->End(m_pDisjointQuery);

	m_measurable = true;
}

uint32 CDeviceTimestampGroup::IssueTimestamp(CDeviceCommandList* pCommandList)
{
	// Passing a nullptr means we want to use the current core command-list
	auto* pContext = pCommandList 
		? pCommandList->GetDX11CommandList()->GetD3D11DeviceContext()
		: gcpRendD3D->GetDeviceContext();

	assert(m_numTimestamps < m_timestampQueries.size());
	pContext->End(m_timestampQueries[m_numTimestamps]);
	m_timeValues[m_numTimestamps] = 0;

	return (m_numTimestamps + 1) < m_timestampQueries.size() ? m_numTimestamps++ : m_numTimestamps;
}

bool CDeviceTimestampGroup::ResolveTimestamps()
{
	if (!m_measurable)
		return false;
	if (m_measured)
		return true;

	// Don't ask twice (API violation)
	if (!m_frequency)
	{
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
		if (gcpRendD3D->GetDeviceContext()->GetData(m_pDisjointQuery, &disjointData, m_pDisjointQuery->GetDataSize(), 0) == S_OK)
		{
			m_frequency = disjointData.Frequency;
		}
		else
		{
			m_frequency = 0;
			return false;
		}
	}

	int i = m_numTimestamps;
	while (--i >= 0)
	{
		// Don't ask twice (API violation)
		if (!m_timeValues[i])
		{
			if (gcpRendD3D->GetDeviceContext()->GetData(m_timestampQueries[i], &m_timeValues[i], m_timestampQueries[i]->GetDataSize(), 0) != S_OK)
			{
				m_timeValues[i] = 0;
				return false;
			}
		}
	}

	return m_measured = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceNvidiaCommandInterfaceImpl* CDeviceCommandListImpl::GetNvidiaCommandInterfaceImpl()
{
	if (gcpRendD3D->m_bVendorLibInitialized && (gRenDev->GetFeatures() & RFT_HW_NVIDIA))
	{
		return reinterpret_cast<CDeviceNvidiaCommandInterfaceImpl*>(this);
	}

	return nullptr;
}

void CDeviceCommandListImpl::SetProfilerMarker(const char* label)
{
#if CRY_PLATFORM_DURANGO
	PROFILE_LABEL_GPU_MT(label, GetDX11CommandList()->GetD3D11DeviceContext());
#else
	PROFILE_LABEL_GPU(label);
#endif
}

void CDeviceCommandListImpl::BeginProfilerEvent(const char* label)
{
#if CRY_PLATFORM_DURANGO
	PROFILE_LABEL_PUSH_GPU_MT(label, GetDX11CommandList()->GetD3D11DeviceContext());
#else
	PROFILE_LABEL_PUSH_GPU(label);
#endif
}

void CDeviceCommandListImpl::EndProfilerEvent(const char* label)
{
#if CRY_PLATFORM_DURANGO
	PROFILE_LABEL_POP_GPU_MT(label, GetDX11CommandList()->GetD3D11DeviceContext());
#else
	PROFILE_LABEL_POP_GPU(label);
#endif
}

void CDeviceCommandListImpl::ClearStateImpl(bool bOutputMergerOnly) const
{
	if (GetDX11CommandList() && GetDX11CommandList()->GetD3D11DeviceContext())
	{
		D3DDepthSurface* pDSV = 0;
		D3DSurface*      pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { 0 };

		if (bOutputMergerOnly)
			GetDX11CommandList()->GetD3D11DeviceContext()->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, pDSV);
		else
			GetDX11CommandList()->GetD3D11DeviceContext()->ClearState();
	}
}

void CDeviceCommandListImpl::LockToThreadImpl()
{
	GetDX11CommandList()->Begin();
}

void CDeviceCommandListImpl::CloseImpl()
{
	GetDX11CommandList()->End();
}

void CDeviceCommandListImpl::ResetImpl()
{
	const uint8 InvalidShaderPointer = (uint8)(uint64)CDeviceResourceSet_DX11::InvalidPointer;
	CRY_ASSERT((InvalidShaderPointer & 0x1) != 0);

	memset(m_sharedState.shader, InvalidShaderPointer, sizeof(m_sharedState.shader));
	memset(m_sharedState.shaderResourceView, InvalidShaderPointer, sizeof(m_sharedState.shaderResourceView));
	memset(m_sharedState.samplerState, InvalidShaderPointer, sizeof(m_sharedState.samplerState));
	memset(m_sharedState.constantBuffer, InvalidShaderPointer, sizeof(m_sharedState.constantBuffer));
	m_sharedState.validShaderStages = EShaderStage_All;

	m_sharedState.requiredSRVs.fill(0);
	m_sharedState.requiredUAVs.fill(0);
	m_sharedState.requiredSamplers.fill(0);

	UINT_PTR invalidPtr = UINT_PTR(~0u);
	m_graphicsState.custom.depthStencilState = nullptr;
	m_graphicsState.custom.rasterizerState = nullptr;
	m_graphicsState.custom.blendState = nullptr;
	m_graphicsState.custom.inputLayout = reinterpret_cast<ID3D11InputLayout*>(invalidPtr);
	m_graphicsState.custom.topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_graphicsState.custom.bDepthStencilStateDirty = true;
	m_graphicsState.custom.depthConstBias = 0.0f;
	m_graphicsState.custom.depthSlopeBias = 0.0f;
	m_graphicsState.custom.depthBiasClamp = 0.0f;
	m_graphicsState.custom.bRasterizerStateDirty = true;

	m_computeState.custom.boundUAVs = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// *INDENT-OFF*
static_assert(
	(1 << eHWSC_Vertex  ) == EShaderStage_Vertex   &&
	(1 << eHWSC_Pixel   ) == EShaderStage_Pixel    &&
	(1 << eHWSC_Geometry) == EShaderStage_Geometry &&
	(1 << eHWSC_Domain  ) == EShaderStage_Domain   &&
	(1 << eHWSC_Hull    ) == EShaderStage_Hull,
	"EShaderStage should have bits corresponding with EHWShaderClass");

static_assert(
	eHWSC_Vertex   == 0 &&
	eHWSC_Pixel    == 1 &&
	eHWSC_Geometry == 2 &&
	eHWSC_Domain   == 3 &&
	eHWSC_Hull     == 4 &&
	eHWSC_Compute  == 5,
	"EHWShaderClass members are not the same as the type delegate assumptions");

static inline void BindShaderResources(D3DDeviceContext* pContext, EHWShaderClass shaderClass, uint32 startSlot, uint32 viewCount, ID3D11ShaderResourceView* const * ppSRVs)
{
	typedef void (STDMETHODCALLTYPE ID3D11DeviceContext::*typeSetShaderResources)(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews);

	constexpr typeSetShaderResources mapSetShaderResources[] =
	{
		&ID3D11DeviceContext::VSSetShaderResources,
		&ID3D11DeviceContext::PSSetShaderResources,
		&ID3D11DeviceContext::GSSetShaderResources,
		&ID3D11DeviceContext::DSSetShaderResources,
		&ID3D11DeviceContext::HSSetShaderResources,
		&ID3D11DeviceContext::CSSetShaderResources,
	};

	(pContext->*mapSetShaderResources[shaderClass])(startSlot, viewCount, ppSRVs);
}

static inline void BindSamplers(D3DDeviceContext* pContext, EHWShaderClass shaderClass, uint32 startSlot, uint32 samplerCount, ID3D11SamplerState* const * ppSamplerStates)
{
	typedef void (STDMETHODCALLTYPE ID3D11DeviceContext::*typeSetSamplers)(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers);

	constexpr typeSetSamplers mapSetSamplers[] =
	{
		&ID3D11DeviceContext::VSSetSamplers,
		&ID3D11DeviceContext::PSSetSamplers,
		&ID3D11DeviceContext::GSSetSamplers,
		&ID3D11DeviceContext::DSSetSamplers,
		&ID3D11DeviceContext::HSSetSamplers,
		&ID3D11DeviceContext::CSSetSamplers,
	};

	(pContext->*mapSetSamplers[shaderClass])(startSlot, samplerCount, ppSamplerStates);
}

static inline void BindConstantBuffer(D3DDeviceContext* pContext, EHWShaderClass shaderClass, D3DBuffer* pBuffer, uint32 slot, uint32 offsetInBytes, uint32 sizeInBytes)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	typedef void (STDMETHODCALLTYPE ID3D11DeviceContext1::*typeSetConstantBuffers1)(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants);

	constexpr typeSetConstantBuffers1 mapSetConstantBuffers1[] =
	{
		&ID3D11DeviceContext1::VSSetConstantBuffers1,
		&ID3D11DeviceContext1::PSSetConstantBuffers1,
		&ID3D11DeviceContext1::GSSetConstantBuffers1,
		&ID3D11DeviceContext1::DSSetConstantBuffers1,
		&ID3D11DeviceContext1::HSSetConstantBuffers1,
		&ID3D11DeviceContext1::CSSetConstantBuffers1,
	};

	uint32 offsetInConstants = offsetInBytes >> 4;
	uint32 sizeInConstants = sizeInBytes >> 4;

	(pContext->*mapSetConstantBuffers1[shaderClass])(slot, 1, &pBuffer, &offsetInConstants, &sizeInConstants);
#else
	typedef void (STDMETHODCALLTYPE ID3D11DeviceContext::*typeSetConstantBuffers)(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
	constexpr typeSetConstantBuffers mapSetConstantBuffers[] =
	{
		&ID3D11DeviceContext::VSSetConstantBuffers,
		&ID3D11DeviceContext::PSSetConstantBuffers,
		&ID3D11DeviceContext::GSSetConstantBuffers,
		&ID3D11DeviceContext::DSSetConstantBuffers,
		&ID3D11DeviceContext::HSSetConstantBuffers,
		&ID3D11DeviceContext::CSSetConstantBuffers,
	};

	(pContext->*mapSetConstantBuffers[shaderClass])(slot, 1, &pBuffer);
#endif
}

static inline void BindShader(D3DDeviceContext* pContext, EHWShaderClass shaderClass, ID3D11DeviceChild* pShader)
{
	typedef void (STDMETHODCALLTYPE ID3D11DeviceContext::*typeSetShader)(ID3D11DeviceChild* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances);

	constexpr typeSetShader mapSetShader[6] =
	{
		(typeSetShader)&ID3D11DeviceContext::VSSetShader,
		(typeSetShader)&ID3D11DeviceContext::PSSetShader,
		(typeSetShader)&ID3D11DeviceContext::GSSetShader,
		(typeSetShader)&ID3D11DeviceContext::DSSetShader,
		(typeSetShader)&ID3D11DeviceContext::HSSetShader,
		(typeSetShader)&ID3D11DeviceContext::CSSetShader,
	};

	(pContext->*mapSetShader[shaderClass])(pShader, nullptr, 0);
}


static inline void BindUnorderedAccessViews(D3DDeviceContext* pContext, EHWShaderClass shaderClass, uint32 startSlot, uint32 uavCount, ID3D11UnorderedAccessView* const * ppUavs)
{
	static const uint32 counts[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	
	switch (shaderClass)
	{
	case eHWSC_Compute:
		pContext->CSSetUnorderedAccessViews(startSlot, uavCount, ppUavs, counts);
		break;
	case eHWSC_Pixel:
		pContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, startSlot, uavCount, ppUavs, counts);
		break;
	default:
		CRY_ASSERT_MESSAGE(0, "UAVs can only be bound to compute- or pixel shaders");
	}
}

// *INDENT-ON*

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandInterfaceImpl::BeginRenderPassImpl(const CDeviceRenderPass& renderPass, const D3DRectangle& renderArea)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->OMSetRenderTargets(renderPass.m_RenderTargetCount, renderPass.m_RenderTargetViews.data(), renderPass.m_pDepthStencilView);
}

void CDeviceGraphicsCommandInterfaceImpl::SetViewportsImpl(uint32 vpCount, const D3DViewPort* pViewports)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->RSSetViewports(vpCount, pViewports);
}

void CDeviceGraphicsCommandInterfaceImpl::SetScissorRectsImpl(uint32 rcCount, const D3DRectangle* pRects)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->RSSetScissorRects(rcCount, pRects);
}

void CDeviceGraphicsCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceGraphicsPSO* devicePSO)
{
	const CDeviceGraphicsPSO_DX11* pDevicePSO = reinterpret_cast<const CDeviceGraphicsPSO_DX11*>(devicePSO);

	// RasterState, BlendState
	if (m_graphicsState.custom.blendState.Set(pDevicePSO->m_pBlendState.get()))
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->OMSetBlendState(pDevicePSO->m_pBlendState, NULL, 0xffffffff);
	}

	// Rasterizer state: NOTE - we don't know the final depth bias values yet so only mark dirty here and set in actual draw call
	if (m_graphicsState.custom.rasterizerState.Set(pDevicePSO->m_pRasterizerState.get()))
	{
		m_graphicsState.custom.bRasterizerStateDirty = true;
	}

	// Depth stencil state: NOTE - we don't know the final stencil ref value yet so only mark dirty here and set in actual draw call
	if (m_graphicsState.custom.depthStencilState.Set(pDevicePSO->m_pDepthStencilState.get()))
	{
		m_graphicsState.custom.bDepthStencilStateDirty = true;
	}

	// input layout and topology
	if (m_graphicsState.custom.inputLayout.Set(pDevicePSO->m_pInputLayout.get()))   GetDX11CommandList()->GetD3D11DeviceContext()->IASetInputLayout(pDevicePSO->m_pInputLayout);
	if (m_graphicsState.custom.topology.Set(pDevicePSO->m_PrimitiveTopology))       GetDX11CommandList()->GetD3D11DeviceContext()->IASetPrimitiveTopology(pDevicePSO->m_PrimitiveTopology);

	// Shaders and update valid shader mask
	m_sharedState.validShaderStages = EShaderStage_None;

	const std::array<void*, eHWSC_Num>& shaders = pDevicePSO->m_pDeviceShaders;
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_NumGfx; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		m_sharedState.validShaderStages |= SHADERSTAGE_FROM_SHADERCLASS_CONDITIONAL(shaderClass, shaders[shaderClass] ? 1 : 0);

		if (m_sharedState.shader[shaderClass].Set(shaders[shaderClass]))
		{
			BindShader(GetDX11CommandList()->GetD3D11DeviceContext(), shaderClass, (ID3D11DeviceChild*) shaders[shaderClass]);
		}
	}

	m_sharedState.requiredSRVs = pDevicePSO->m_requiredSRVs;
	m_sharedState.requiredUAVs = pDevicePSO->m_requiredUAVs;
	m_sharedState.requiredSamplers = pDevicePSO->m_requiredSamplers;
}

void CDeviceGraphicsCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources)
{
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);

	if (pResources->GetFlags() & CDeviceResourceSet::EFlags_ForceSetAllState)
	{
		SetResources_All(pResourcesDX11);
	}
	else
	{
		SetResources_RequestedByShaderOnly(pResourcesDX11);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetResources_RequestedByShaderOnly(const CDeviceResourceSet* pResources)
{
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);
	auto pContext = GetDX11CommandList()->GetD3D11DeviceContext();

	// Shader stages are ordered by usage-frequency and loop exists according to usage-frequency (VS+PS fast, etc.)
	int validShaderStages = m_sharedState.validShaderStages;
	for (EHWShaderClass shaderClass = eHWSC_Vertex; validShaderStages; shaderClass = EHWShaderClass(shaderClass + 1), validShaderStages >>= 1)
	{
		if (validShaderStages & 1)
		{
			pResourcesDX11->compiledSRVs.ForEach(
				shaderClass, 
				m_sharedState.requiredSRVs[shaderClass],
				m_sharedState.shaderResourceView[shaderClass], 
				[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 viewCount, ID3D11ShaderResourceView* const * ppSRVs)
				{
					BindShaderResources(pContext, shaderClass, startSlot, viewCount, ppSRVs);
				}
			);

			pResourcesDX11->compiledSamplers.ForEach(
				shaderClass, 
				m_sharedState.requiredSamplers[shaderClass],
				m_sharedState.samplerState[shaderClass], 
				[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 samplerCount, ID3D11SamplerState* const * ppSamplerStates)
				{
					BindSamplers(pContext, shaderClass, startSlot, samplerCount, ppSamplerStates);
				}
			);

			// Bind constant buffers
			for (int i = 0; i < pResourcesDX11->numCompiledCBs[shaderClass]; ++i)
			{
				const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[shaderClass][i];
				if (m_sharedState.constantBuffer[shaderClass][cb.slot].Set(cb.code))
				{
					BindConstantBuffer(pContext, shaderClass, cb.pBuffer, cb.slot, cb.offset, cb.size);
				}
			}
		}
	}

	pResourcesDX11->compiledUAVs.ForEach(eHWSC_Pixel, [pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 uavCount, ID3D11UnorderedAccessView* const * ppUavs)
	{
		BindUnorderedAccessViews(pContext, shaderClass, startSlot, uavCount, ppUavs);
	});
}

void CDeviceGraphicsCommandInterfaceImpl::SetResources_All(const CDeviceResourceSet* pResources)
{
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);
	auto pContext = GetDX11CommandList()->GetD3D11DeviceContext();

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_NumGfx; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		pResourcesDX11->compiledSRVs.ForEach(
			shaderClass,
			m_sharedState.shaderResourceView[shaderClass], 
			[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 viewCount, ID3D11ShaderResourceView* const * ppSRVs)
			{
				BindShaderResources(pContext, shaderClass, startSlot, viewCount, ppSRVs);
			}
		);

		pResourcesDX11->compiledSamplers.ForEach(
			shaderClass, 
			m_sharedState.samplerState[shaderClass],
			[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 samplerCount, ID3D11SamplerState* const * ppSamplerStates)
			{
				BindSamplers(pContext, shaderClass, startSlot, samplerCount, ppSamplerStates);
			}
		);

		// Bind constant buffers
		for (int i = 0; i < pResourcesDX11->numCompiledCBs[shaderClass]; ++i)
		{
			const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[shaderClass][i];
			if (m_sharedState.constantBuffer[shaderClass][cb.slot].Set(cb.code))
			{
				BindConstantBuffer(pContext, shaderClass, cb.pBuffer, cb.slot, cb.offset, cb.size);
			}
		}
	}

	// Bind UAVs
	pResourcesDX11->compiledUAVs.ForEach(eHWSC_Pixel, [pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 uavCount, ID3D11UnorderedAccessView* const * ppUavs)
	{
		BindUnorderedAccessViews(pContext, shaderClass, startSlot, uavCount, ppUavs);
	});
}
// *INDENT-ON*

void CDeviceGraphicsCommandInterfaceImpl::ApplyDepthStencilState()
{
	if (m_graphicsState.custom.bDepthStencilStateDirty)
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->OMSetDepthStencilState(m_graphicsState.custom.depthStencilState.cachedValue, m_graphicsState.stencilRef.cachedValue);
		m_graphicsState.custom.bDepthStencilStateDirty = false;
	}
}

void CDeviceGraphicsCommandInterfaceImpl::ApplyRasterizerState()
{
	if (m_graphicsState.custom.bRasterizerStateDirty)
	{
		auto pRasterizerState = m_graphicsState.custom.rasterizerState.cachedValue;

		if (m_graphicsState.custom.depthConstBias != 0.0f || m_graphicsState.custom.depthSlopeBias != 0.0f)
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			m_graphicsState.custom.rasterizerState.cachedValue->GetDesc(&rasterDesc);
			rasterDesc.DepthBias = int(m_graphicsState.custom.depthConstBias);
			rasterDesc.SlopeScaledDepthBias = m_graphicsState.custom.depthSlopeBias;
			rasterDesc.DepthBiasClamp = m_graphicsState.custom.depthBiasClamp;

			CDeviceStatesManagerDX11 *const pDeviceStatesManager = CDeviceStatesManagerDX11::GetInstance();
			uint32 newRasterizerStateIndex = pDeviceStatesManager->GetOrCreateRasterState(rasterDesc);
			if (newRasterizerStateIndex != uint32(-1))
			{
				pRasterizerState = pDeviceStatesManager->GetRasterState(newRasterizerStateIndex).pState;
			}
		}

		GetDX11CommandList()->GetD3D11DeviceContext()->RSSetState(pRasterizerState);
		m_graphicsState.custom.bRasterizerStateDirty = false;
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages)
{
	// Shader stages are ordered by usage-frequency and loop exists according to usage-frequency (VS+PS fast, etc.)
	int validShaderStages = shaderStages;
	for (EHWShaderClass shaderClass = eHWSC_Vertex; validShaderStages; shaderClass = EHWShaderClass(shaderClass + 1), validShaderStages >>= 1)
	{
		if (validShaderStages & 1)
			SetInlineConstantBufferImpl(bindSlot, pBuffer, shaderSlot, shaderClass);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
	if (m_sharedState.constantBuffer[shaderClass][shaderSlot].Set(pBuffer->GetCode()))
	{
		buffer_size_t offset, size;
		D3DBuffer* pBufferDX11 = pBuffer->GetD3D(&offset, &size);

		BindConstantBuffer(GetDX11CommandList()->GetD3D11DeviceContext(), shaderClass, pBufferDX11, shaderSlot, offset, size);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, EShaderStage shaderStages, ResourceViewHandle resourceViewID)
{
	// Shader stages are ordered by usage-frequency and loop exists according to usage-frequency (VS+PS fast, etc.)
	int validShaderStages = shaderStages;
	for (EHWShaderClass shaderClass = eHWSC_Vertex; validShaderStages; shaderClass = EHWShaderClass(shaderClass + 1), validShaderStages >>= 1)
	{
		if (validShaderStages & 1)
			SetInlineShaderResourceImpl(bindSlot, pBuffer, shaderSlot, shaderClass, resourceViewID);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, EHWShaderClass shaderClass, ResourceViewHandle resourceViewID)
{
	D3DShaderResource* pBufferViewDX11 = pBuffer->LookupSRV(resourceViewID);
	BindShaderResources(GetDX11CommandList()->GetD3D11DeviceContext(), shaderClass, shaderSlot, 1, &pBufferViewDX11);
}

void CDeviceGraphicsCommandInterfaceImpl::SetVertexBuffersImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams)
{
	for (uint32 s = 0; s < numStreams; ++s)
	{
		const CDeviceInputStream& current = vertexStreams[s];

		CRY_ASSERT(current.hStream != ~0u);
		{
			buffer_size_t offset;
			D3DBuffer* buffer = gcpRendD3D.m_DevBufMan.GetD3D(current.hStream, &offset);

			uint32 offset32 = offset;
			GetDX11CommandList()->GetD3D11DeviceContext()->IASetVertexBuffers(current.nSlot, 1, &buffer, &current.nStride, &offset32);
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetIndexBufferImpl(const CDeviceInputStream* indexStream)
{
	const CDeviceInputStream& current = indexStream[0];

	CRY_ASSERT(current.hStream != ~0u);
	{
		buffer_size_t offset;
		D3DBuffer* buffer = gcpRendD3D.m_DevBufMan.GetD3D(current.hStream, &offset);

#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
		GetDX11CommandList()->GetD3D11DeviceContext()->IASetIndexBuffer(buffer, DXGI_FORMAT_R16_UINT, offset);
#else
		GetDX11CommandList()->GetD3D11DeviceContext()->IASetIndexBuffer(buffer, (DXGI_FORMAT)current.nStride, offset);
#endif
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetStencilRefImpl(uint8 stencilRefValue)
{
	m_graphicsState.custom.bDepthStencilStateDirty = true;
}

void CDeviceGraphicsCommandInterfaceImpl::SetDepthBiasImpl(float constBias, float slopeBias, float biasClamp)
{
	m_graphicsState.custom.depthConstBias = constBias;
	m_graphicsState.custom.depthSlopeBias = slopeBias;
	m_graphicsState.custom.depthBiasClamp = biasClamp;

	m_graphicsState.custom.bRasterizerStateDirty = true;
}

void CDeviceGraphicsCommandInterfaceImpl::DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
	ApplyDepthStencilState();
	ApplyRasterizerState();

	if (InstanceCount > 1 || StartInstanceLocation > 0)
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}
	else
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->Draw(VertexCountPerInstance, StartVertexLocation);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
	STATIC_CHECK(false, "NOT IMPLEMENTED");
#endif

	ApplyDepthStencilState();
	ApplyRasterizerState();

	if (InstanceCount > 1 || StartInstanceLocation > 0)
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}
	else
	{
		GetDX11CommandList()->GetD3D11DeviceContext()->DrawIndexed(IndexCountPerInstance, StartIndexLocation, BaseVertexLocation);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DSurface* pView, const float color[4], UINT numRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearView(pView, color, pRects, numRects);
#else
	CRY_ASSERT(numRects == 0); // not supported on dx11
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearRenderTargetView(pView, color);
#endif
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DDepthSurface* pView, int clearFlags, float depth, uint8 stencil, uint32 numRects, const D3D11_RECT* pRects)
{
	CRY_ASSERT(numRects == 0); // not supported on dx11
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearDepthStencilView(pView, D3D11_CLEAR_FLAG(clearFlags), depth, stencil);
}

void CDeviceGraphicsCommandInterfaceImpl::DiscardContentsImpl(D3DResource* pResource, uint32 numRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	CRY_ASSERT(numRects == 0); // not supported on dx11.1
	GetDX11CommandList()->GetD3D11DeviceContext()->DiscardResource(pResource);
#endif
}

void CDeviceGraphicsCommandInterfaceImpl::DiscardContentsImpl(D3DBaseView* pView, uint32 numRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	if (numRects)
		GetDX11CommandList()->GetD3D11DeviceContext()->DiscardView1(pView, pRects, numRects);
	else
		GetDX11CommandList()->GetD3D11DeviceContext()->DiscardView(pView);
#endif
}

void CDeviceGraphicsCommandInterfaceImpl::BeginOcclusionQueryImpl(D3DOcclusionQuery* pQuery)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->Begin(pQuery);
}

void CDeviceGraphicsCommandInterfaceImpl::EndOcclusionQueryImpl(D3DOcclusionQuery* pQuery)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->End(pQuery);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceComputeCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceComputePSO* pDevicePSO)
{
	const CDeviceComputePSO_DX11* pDevicePsoDX11 = reinterpret_cast<const CDeviceComputePSO_DX11*>(pDevicePSO);

	GetDX11CommandList()->GetD3D11DeviceContext()->CSSetShader(static_cast<ID3D11ComputeShader*>(pDevicePsoDX11->m_pDeviceShader), nullptr, 0);
}

void CDeviceComputeCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources)
{
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11* RESTRICT_POINTER>(pResources);
	auto pContext = GetDX11CommandList()->GetD3D11DeviceContext();

	// Bind SRVs
	pResourcesDX11->compiledSRVs.ForEach(
		eHWSC_Compute, 
		m_sharedState.shaderResourceView[eHWSC_Compute], 
		[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 viewCount, ID3D11ShaderResourceView* const * ppSRVs)
		{
			BindShaderResources(pContext, shaderClass, startSlot, viewCount, ppSRVs);
		}
	);

	pResourcesDX11->compiledSamplers.ForEach(
		eHWSC_Compute, 
		m_sharedState.samplerState[eHWSC_Compute], 
		[pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 samplerCount, ID3D11SamplerState* const * ppSamplerStates)
		{
			BindSamplers(pContext, shaderClass, startSlot, samplerCount, ppSamplerStates);
		}
	);

	// Bind constant buffers
	for (int i = 0; i < pResourcesDX11->numCompiledCBs[eHWSC_Compute]; ++i)
	{
		const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[eHWSC_Compute][i];
		if (m_sharedState.constantBuffer[eHWSC_Compute][cb.slot].Set(cb.code))
		{
			BindConstantBuffer(pContext, eHWSC_Compute, cb.pBuffer, cb.slot, cb.offset, cb.size);
		}
	}

	// Bind UAVs
	auto bindUavsAndMarkBound = [this, pContext](EHWShaderClass shaderClass, uint32 startSlot, uint32 uavCount, ID3D11UnorderedAccessView* const * ppUavs)
	{
		BindUnorderedAccessViews(pContext, shaderClass, startSlot, uavCount, ppUavs);

		for (int s = startSlot, e = startSlot + uavCount; s < e; ++s)
			m_computeState.custom.boundUAVs[s] = true;
	};

	pResourcesDX11->compiledUAVs.ForEach(eHWSC_Compute, bindUavsAndMarkBound);
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot)
{
	if (m_sharedState.constantBuffer[eHWSC_Compute][shaderSlot].Set(pBuffer->GetCode()))
	{
		buffer_size_t offset = 0, size = 0;
		ID3D11Buffer* pBuf = pBuffer ? pBuffer->GetD3D(&offset, &size) : NULL;

		BindConstantBuffer(GetDX11CommandList()->GetD3D11DeviceContext(), eHWSC_Compute, pBuf, shaderSlot, offset, size);
	}
}

void CDeviceComputeCommandInterfaceImpl::SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, ResourceViewHandle resourceViewID)
{
	D3DShaderResource* pBufferViewDX11 = pBuffer->LookupSRV(resourceViewID);
	BindShaderResources(GetDX11CommandList()->GetD3D11DeviceContext(), eHWSC_Compute, shaderSlot, 1, &pBufferViewDX11);
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	CRY_FUNCTION_NOT_IMPLEMENTED;
}

void CDeviceComputeCommandInterfaceImpl::DispatchImpl(uint32 X, uint32 Y, uint32 Z)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->Dispatch(X, Y, Z);

	if (m_computeState.custom.boundUAVs.any())
	{
		for (int i = 0; i < m_computeState.custom.boundUAVs.size(); ++i)
		{
			const uint32 count(-1);
			ID3D11UnorderedAccessView* const nullView = nullptr;

			GetDX11CommandList()->GetD3D11DeviceContext()->CSSetUnorderedAccessViews(i, 1, &nullView, &count);
		}

		m_computeState.custom.boundUAVs = 0;
		memset(m_computeState.pResourceSets, 0x0, sizeof(m_computeState.pResourceSets));
	}
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearView(pView, Values, pRects, NumRects);
#else
	CRY_ASSERT(NumRects == 0); // not supported on dx11.0
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearUnorderedAccessViewFloat(pView, Values);
#endif
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	// If the format is integer, such as DXGI_FORMAT_R8G8B8A8_UINT, the runtime interprets inputs as integral floats.
	// Therefore, 235.0f maps to 235 (rounds to zero, out of range/INF values clamp to target range, and NaN to 0).
	const FLOAT FValues[4] = { FLOAT(Values[0]), FLOAT(Values[1]), FLOAT(Values[2]), FLOAT(Values[3]) };
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearView(pView, FValues, pRects, NumRects);
#else
	CRY_ASSERT(NumRects == 0); // not supported on dx11.0
	GetDX11CommandList()->GetD3D11DeviceContext()->ClearUnorderedAccessViewUint(pView, Values);
#endif
}

void CDeviceComputeCommandInterfaceImpl::DiscardUAVContentsImpl(D3DResource* pResource, uint32 numRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	CRY_ASSERT(numRects == 0); // not supported on dx11.1
	GetDX11CommandList()->GetD3D11DeviceContext()->DiscardResource(pResource);
#endif
}

void CDeviceComputeCommandInterfaceImpl::DiscardUAVContentsImpl(D3DBaseView* pView, uint32 numRects, const D3D11_RECT* pRects)
{
#if (CRY_RENDERER_DIRECT3D >= 111)
	if (numRects)
		GetDX11CommandList()->GetD3D11DeviceContext()->DiscardView1(pView, pRects, numRects);
	else
		GetDX11CommandList()->GetD3D11DeviceContext()->DiscardView(pView);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceNvidiaCommandInterfaceImpl::SetModifiedWModeImpl(bool enabled, uint32 numViewports, const float* pA, const float* pB)
{
	// GetDX11CommandList()->GetD3D11DeviceContext()->SetModifiedWMode(enabled, numViewports, pA, pB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceCopyCommandInterfaceImpl::CopySubresourcesRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags, UINT NumSubresources)
{
	if (NumSubresources <= 1)
	{
#if (CRY_RENDERER_DIRECT3D >= 111)
		GetDX11CommandList()->GetD3D11DeviceContext()->CopySubresourceRegion1(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags);
#else
		GetDX11CommandList()->GetD3D11DeviceContext()->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
#endif
	}
	else
	{
		D3D11_RESOURCE_DIMENSION srcDim;  pSrcResource->GetType(&srcDim);
		D3D11_RESOURCE_DIMENSION dstDim;  pDstResource->GetType(&dstDim);
		UINT srcMipLevels = 0;
		UINT dstMipLevels = 0;
		if (srcDim == D3D11_RESOURCE_DIMENSION_TEXTURE1D) { D3D11_TEXTURE1D_DESC srcDesc; ((ID3D11Texture1D*)pSrcResource)->GetDesc(&srcDesc); srcMipLevels = srcDesc.MipLevels; }
		if (srcDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) { D3D11_TEXTURE2D_DESC srcDesc; ((ID3D11Texture2D*)pSrcResource)->GetDesc(&srcDesc); srcMipLevels = srcDesc.MipLevels; }
		if (srcDim == D3D11_RESOURCE_DIMENSION_TEXTURE3D) { D3D11_TEXTURE3D_DESC srcDesc; ((ID3D11Texture3D*)pSrcResource)->GetDesc(&srcDesc); srcMipLevels = srcDesc.MipLevels; }
		if (dstDim == D3D11_RESOURCE_DIMENSION_TEXTURE1D) { D3D11_TEXTURE1D_DESC dstDesc; ((ID3D11Texture1D*)pDstResource)->GetDesc(&dstDesc); dstMipLevels = dstDesc.MipLevels; }
		if (dstDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) { D3D11_TEXTURE2D_DESC dstDesc; ((ID3D11Texture2D*)pDstResource)->GetDesc(&dstDesc); dstMipLevels = dstDesc.MipLevels; }
		if (dstDim == D3D11_RESOURCE_DIMENSION_TEXTURE3D) { D3D11_TEXTURE3D_DESC dstDesc; ((ID3D11Texture3D*)pDstResource)->GetDesc(&dstDesc); dstMipLevels = dstDesc.MipLevels; }

		D3D11_BOX srcBoxBck = { 0 };
		D3D11_BOX srcRegion = { 0 };
		D3D11_BOX dstRegion = { DstX, DstY, DstZ };
		if (pSrcBox)
		{
			srcBoxBck = *pSrcBox;
			srcRegion = *pSrcBox;
			pSrcBox = &srcRegion;
		}

		// NOTE: too complex case which is not supported as it leads to fe. [slice,mip] sequences like [0,4],[0,5],[0,6],[1,0],[1,1],...
		// which we don't support because the offsets and dimensions are relative to a intermediate mip-level, while crossing the
		// slice-boundary forces us to extrapolate dimensions to larger mips, which is probably not what is wanted in the first place.
		CRY_ASSERT(!srcMipLevels || (SrcSubresource / srcMipLevels == (SrcSubresource + NumSubresources - 1) / srcMipLevels));
		CRY_ASSERT(!dstMipLevels || (DstSubresource / dstMipLevels == (DstSubresource + NumSubresources - 1) / dstMipLevels));

		for (UINT n = 0; n < NumSubresources; ++n)
		{
			const UINT srcSlice = (SrcSubresource + n) / (srcMipLevels);
			const UINT dstSlice = (DstSubresource + n) / (dstMipLevels);
			const UINT srcLevel = (SrcSubresource + n) % (srcMipLevels);
			const UINT dstLevel = (DstSubresource + n) % (dstMipLevels);

			// reset dimensions/coordinates when crossing slice-boundary
			if (!srcLevel)
				srcRegion = srcBoxBck;
			if (!dstLevel)
				dstRegion = { DstX, DstY, DstZ };

#if (CRY_RENDERER_DIRECT3D >= 111)
			GetDX11CommandList()->GetD3D11DeviceContext()->CopySubresourceRegion1(pDstResource, DstSubresource + n, dstRegion.left, dstRegion.top, dstRegion.front, pSrcResource, SrcSubresource + n, pSrcBox, CopyFlags);
#else
			GetDX11CommandList()->GetD3D11DeviceContext()->CopySubresourceRegion(pDstResource, DstSubresource + n, dstRegion.left, dstRegion.top, dstRegion.front, pSrcResource, SrcSubresource + n, pSrcBox);
#endif

			srcRegion.left >>= 1;
			srcRegion.top >>= 1;
			srcRegion.front >>= 1;

			srcRegion.right >>= 1; if (srcRegion.right == srcRegion.left) srcRegion.right = srcRegion.left + 1;
			srcRegion.bottom >>= 1; if (srcRegion.bottom == srcRegion.top) srcRegion.bottom = srcRegion.top + 1;
			srcRegion.back >>= 1; if (srcRegion.back == srcRegion.front) srcRegion.back = srcRegion.front + 1;

			dstRegion.left >>= 1;
			dstRegion.top >>= 1;
			dstRegion.front >>= 1;
		}
	}
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DResource* pSrc, D3DResource* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst, pSrc);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceBuffer* pSrc, CDeviceBuffer* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst->GetBaseBuffer(), pSrc->GetBaseBuffer());
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DBuffer* pSrc, D3DBuffer* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst, pSrc);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceTexture* pSrc, CDeviceTexture* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst->GetBaseTexture(), pSrc->GetBaseTexture());
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceTexture* pSrc, D3DTexture* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst, pSrc->GetBaseTexture());
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DTexture* pSrc, D3DTexture* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst, pSrc);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DTexture* pSrc, CDeviceTexture* pDst)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->CopyResource(pDst->GetBaseTexture(), pSrc);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceBuffer* pSrc, CDeviceBuffer* pDst, const SResourceRegionMapping& region)
{
	D3D11_BOX box = { region.SourceOffset.Left, region.SourceOffset.Top, region.SourceOffset.Front, region.SourceOffset.Left + region.Extent.Width, region.SourceOffset.Top + region.Extent.Height, region.SourceOffset.Front + region.Extent.Depth };

	CopySubresourcesRegion1(pDst->GetBaseBuffer(), region.DestinationOffset.Subresource, region.DestinationOffset.Left, region.DestinationOffset.Top, region.DestinationOffset.Front, pSrc->GetBaseBuffer(), region.SourceOffset.Subresource, &box, region.Flags, region.Extent.Subresources);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DBuffer* pSrc, D3DBuffer* pDst, const SResourceRegionMapping& region)
{
	D3D11_BOX box = { region.SourceOffset.Left, region.SourceOffset.Top, region.SourceOffset.Front, region.SourceOffset.Left + region.Extent.Width, region.SourceOffset.Top + region.Extent.Height, region.SourceOffset.Front + region.Extent.Depth };

	CopySubresourcesRegion1(pDst, region.DestinationOffset.Subresource, region.DestinationOffset.Left, region.DestinationOffset.Top, region.DestinationOffset.Front, pSrc, region.SourceOffset.Subresource, &box, region.Flags, region.Extent.Subresources);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceTexture* pSrc, CDeviceTexture* pDst, const SResourceRegionMapping& region)
{
	D3D11_BOX box = { region.SourceOffset.Left, region.SourceOffset.Top, region.SourceOffset.Front, region.SourceOffset.Left + region.Extent.Width, region.SourceOffset.Top + region.Extent.Height, region.SourceOffset.Front + region.Extent.Depth };

	// If CopySubresourceRegion is used with Multisampled or D3D11_BIND_DEPTH_STENCIL Resources, then the whole Subresource must be copied. DstX, DstY, and DstZ must all be 0, while pSrcBox must be NULL.
	D3D11_BOX* pBox = &box;
	if (pSrc->IsResolvable())
	{
		pBox = nullptr;

		CRY_ASSERT(region.DestinationOffset.Left == 0);
		CRY_ASSERT(region.DestinationOffset.Top == 0);
		CRY_ASSERT(region.DestinationOffset.Front == 0);
	}

	CopySubresourcesRegion1(pDst->GetBaseTexture(), region.DestinationOffset.Subresource, region.DestinationOffset.Left, region.DestinationOffset.Top, region.DestinationOffset.Front, pSrc->GetBaseTexture(), region.SourceOffset.Subresource, pBox, region.Flags, region.Extent.Subresources);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(D3DTexture* pSrc, CDeviceTexture* pDst, const SResourceRegionMapping& region)
{
	D3D11_BOX box = { region.SourceOffset.Left, region.SourceOffset.Top, region.SourceOffset.Front, region.SourceOffset.Left + region.Extent.Width, region.SourceOffset.Top + region.Extent.Height, region.SourceOffset.Front + region.Extent.Depth };

	CopySubresourcesRegion1(pDst->GetBaseTexture(), region.DestinationOffset.Subresource, region.DestinationOffset.Left, region.DestinationOffset.Top, region.DestinationOffset.Front, pSrc, region.SourceOffset.Subresource, &box, region.Flags, region.Extent.Subresources);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CConstantBuffer* pDst, const SResourceMemoryAlignment& memoryLayout)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetD3D(), 0, nullptr, pSrc, 0, 0);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CDeviceBuffer* pDst, const SResourceMemoryAlignment& memoryLayout)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetBaseBuffer(), 0, nullptr, pSrc, memoryLayout.rowStride, memoryLayout.planeStride);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CDeviceTexture* pDst, const SResourceMemoryAlignment& memoryLayout)
{
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetBaseTexture(), 0, nullptr, pSrc, memoryLayout.rowStride, memoryLayout.planeStride);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CConstantBuffer* pDst, const SResourceMemoryMapping& region)
{
	CRY_ASSERT((region.Extent.Subresources == 1) && ((region.ResourceOffset.Left == 0) || (CRY_RENDERER_DIRECT3D >= 111))); // TODO: batch
	D3D11_BOX box = { region.ResourceOffset.Left, region.ResourceOffset.Top, region.ResourceOffset.Front, region.ResourceOffset.Left + region.Extent.Width, region.ResourceOffset.Top + region.Extent.Height, region.ResourceOffset.Front + region.Extent.Depth };
#if (CRY_RENDERER_DIRECT3D >= 111)
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource1(pDst->GetD3D(), region.ResourceOffset.Subresource, CRY_RENDERER_DIRECT3D >= 111 ? &box : nullptr, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride, region.Flags);
#else
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetD3D(), region.ResourceOffset.Subresource, CRY_RENDERER_DIRECT3D >= 111 ? &box : nullptr, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride);
#endif
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CDeviceBuffer* pDst, const SResourceMemoryMapping& region)
{
	CRY_ASSERT((region.Extent.Subresources == 1) && (CRY_RENDERER_DIRECT3D < 111)); // TODO: batch
	D3D11_BOX box = { region.ResourceOffset.Left, region.ResourceOffset.Top, region.ResourceOffset.Front, region.ResourceOffset.Left + region.Extent.Width, region.ResourceOffset.Top + region.Extent.Height, region.ResourceOffset.Front + region.Extent.Depth };
#if (CRY_RENDERER_DIRECT3D >= 111)
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource1(pDst->GetBaseBuffer(), region.ResourceOffset.Subresource, &box, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride, region.Flags);
#else
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetBaseBuffer(), region.ResourceOffset.Subresource, &box, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride);
#endif
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(const void* pSrc, CDeviceTexture* pDst, const SResourceMemoryMapping& region)
{
	CRY_ASSERT(region.Extent.Subresources == 1); // TODO: batch
	D3D11_BOX box = { region.ResourceOffset.Left, region.ResourceOffset.Top, region.ResourceOffset.Front, region.ResourceOffset.Left + region.Extent.Width, region.ResourceOffset.Top + region.Extent.Height, region.ResourceOffset.Front + region.Extent.Depth };
#if (CRY_RENDERER_DIRECT3D >= 111)
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource1(pDst->GetBaseTexture(), region.ResourceOffset.Subresource, &box, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride, region.Flags);
#else
	GetDX11CommandList()->GetD3D11DeviceContext()->UpdateSubresource(pDst->GetBaseTexture(), region.ResourceOffset.Subresource, &box, pSrc, region.MemoryLayout.rowStride, region.MemoryLayout.planeStride);
#endif
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceBuffer* pSrc, void* pDst, const SResourceMemoryAlignment& memoryLayout)
{
	assert(0);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceTexture* pSrc, void* pDst, const SResourceMemoryAlignment& memoryLayout)
{
	assert(0);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceBuffer* pSrc, void* pDst, const SResourceMemoryMapping& region)
{
	assert(0);
}

void CDeviceCopyCommandInterfaceImpl::CopyImpl(CDeviceTexture* pSrc, void* pDst, const SResourceMemoryMapping& region)
{
	assert(0);
}
