// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "../DeviceObjects.h"
#include "DriverD3D.h"
#include "../../../Common/ReverseDepth.h"

#if !defined(CRY_USE_DX12_NATIVE) && !defined(CRY_USE_GNM_RENDERER)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define DX11_COMMANDLIST_REDUNDANT_STATE_FILTERING

class CDeviceGraphicsPSO_DX11 : public CDeviceGraphicsPSO
{
public:
	CDeviceGraphicsPSO_DX11();

	virtual bool Init(const CDeviceGraphicsPSODesc& psoDesc) final;

	_smart_ptr<ID3D11RasterizerState>                 m_pRasterizerState;
	uint32                                            m_RasterizerStateIndex;
	_smart_ptr<ID3D11BlendState>                      m_pBlendState;
	_smart_ptr<ID3D11DepthStencilState>               m_pDepthStencilState;
	_smart_ptr<ID3D11InputLayout>                     m_pInputLayout;

	std::array<void*, eHWSC_Num>                      m_pDeviceShaders;

	std::array<std::array<uint8, MAX_TMU>, eHWSC_Num> m_Samplers;
	std::array<std::array<uint8, MAX_TMU>, eHWSC_Num> m_SRVs;

	std::array<uint8, eHWSC_Num>                      m_NumSamplers;
	std::array<uint8, eHWSC_Num>                      m_NumSRVs;

	// Do we still need these?
	uint64           m_ShaderFlags_RT;
	uint32           m_ShaderFlags_MD;
	uint32           m_ShaderFlags_MDV;

	D3DPrimitiveType m_PrimitiveTopology;
};

CDeviceGraphicsPSO_DX11::CDeviceGraphicsPSO_DX11()
{
	m_PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_ShaderFlags_RT = 0;
	m_ShaderFlags_MD = 0;
	m_ShaderFlags_MDV = 0;
	m_RasterizerStateIndex = 0;

	m_NumSamplers.fill(0);
	m_NumSRVs.fill(0);
}

bool CDeviceGraphicsPSO_DX11::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
	CD3D9Renderer* rd = gcpRendD3D;
	m_bValid = false;
	m_nUpdateCount++;

	m_pRasterizerState = nullptr;
	m_pBlendState = nullptr;
	m_pDepthStencilState = nullptr;
	m_pInputLayout = nullptr;

	m_NumSamplers.fill(0);
	m_NumSRVs.fill(0);

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);

	// validate shaders first
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (hwShaders[shaderClass].pHwShader && (hwShaders[shaderClass].pHwShaderInstance == NULL || hwShaders[shaderClass].pDeviceShader == NULL))
			return false;

		m_pDeviceShaders[shaderClass] = hwShaders[shaderClass].pDeviceShader;

		// TODO: remove
		m_pHwShaders[shaderClass] = hwShaders[shaderClass].pHwShader;
		m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	D3D11_BLEND_DESC blendDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

	uint32 rasterStateIndex = rd->GetOrCreateRasterState(rasterizerDesc);
	uint32 blendStateIndex = rd->GetOrCreateBlendState(blendDesc);
	uint32 depthStateIndex = rd->GetOrCreateDepthState(depthStencilDesc);

	if (rasterStateIndex == uint32(-1) || blendStateIndex == uint32(-1) || depthStateIndex == uint32(-1))
		return false;

	m_pDepthStencilState = rd->m_StatesDP[depthStateIndex].pState;
	m_pRasterizerState = rd->m_StatesRS[rasterStateIndex].pState;
	m_RasterizerStateIndex = rasterStateIndex;
	m_pBlendState = rd->m_StatesBL[blendStateIndex].pState;

	// input layout
	if (psoDesc.m_VertexFormat != eVF_Unknown)
	{
		if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
		{
	#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
			// Try to find InputLayout in the cache
			const uint32 cacheID = CD3D9Renderer::FX_GetInputLayoutCacheId(pVsInstance->m_VStreamMask_Decl, psoDesc.m_VertexFormat);
			m_pInputLayout = pVsInstance->GetCachedInputLayout(cacheID);
			if (!m_pInputLayout)
			{
	#endif
			uint8 streamMask = psoDesc.CombineVertexStreamMasks(uint8(pVsInstance->m_VStreamMask_Decl), psoDesc.m_ObjectStreamMask);

			const bool bMorph = false;
			const bool bInstanced = (streamMask & VSM_INSTANCED) != 0;
			SOnDemandD3DVertexDeclarationCache& declCache = rd->m_RP.m_D3DVertexDeclarationCache[streamMask >> 1][bMorph || bInstanced][psoDesc.m_VertexFormat];

			if (!declCache.m_pDeclaration)
			{
				SOnDemandD3DVertexDeclaration Decl;
				rd->m_RP.OnDemandVertexDeclaration(Decl, streamMask >> 1, psoDesc.m_VertexFormat, bMorph, bInstanced);

				if (!Decl.m_Declaration.empty())
				{
					auto hr = rd->GetDevice().CreateInputLayout(&Decl.m_Declaration[0], Decl.m_Declaration.Num(), pVsInstance->m_pShaderData, pVsInstance->m_nDataSize, &declCache.m_pDeclaration);
					if (!SUCCEEDED(hr))
						return false;
				}
			}

			m_pInputLayout = declCache.m_pDeclaration;

	#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
		}
		if (m_pInputLayout)
		{
			pVsInstance->SetCachedInputLayout(m_pInputLayout, cacheID);
		}
	#endif
		}

		if (!m_pInputLayout)
			return false;
	}

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (auto pShaderInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[shaderClass].pHwShaderInstance))
		{
			for (const auto& smp : pShaderInstance->m_Samplers)
				m_Samplers[shaderClass][m_NumSamplers[shaderClass]++] = uint8(smp.m_dwCBufSlot);

			for (const auto& tex : pShaderInstance->m_Textures)
				m_SRVs[shaderClass][m_NumSRVs[shaderClass]++] = uint8(tex.m_dwCBufSlot);
		}
	}

	m_PrimitiveTopology = rd->FX_ConvertPrimitiveType(psoDesc.m_PrimitiveType);
	m_ShaderFlags_RT = psoDesc.m_ShaderFlags_RT;
	m_ShaderFlags_MD = psoDesc.m_ShaderFlags_MD;
	m_ShaderFlags_MDV = psoDesc.m_ShaderFlags_MDV;

	#if defined(ENABLE_PROFILING_CODE)
	m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
	#endif

	m_bValid = true;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX11 : public CDeviceComputePSO
{
public:
	CDeviceComputePSO_DX11();

	virtual bool Init(const CDeviceComputePSODesc& psoDesc) final;

	std::array<void*, eHWSC_Num> m_pDeviceShaders;
};

CDeviceComputePSO_DX11::CDeviceComputePSO_DX11()
{
}

bool CDeviceComputePSO_DX11::Init(const CDeviceComputePSODesc& psoDesc)
{
	CD3D9Renderer* rd = gcpRendD3D;
	m_bValid = false;
	m_nUpdateCount++;

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, false);

	if (hwShaders[eHWSC_Compute].pHwShader && (hwShaders[eHWSC_Compute].pHwShaderInstance == NULL || hwShaders[eHWSC_Compute].pDeviceShader == NULL))
		return false;

	m_pDeviceShaders[eHWSC_Compute] = hwShaders[eHWSC_Compute].pDeviceShader;

	// TODO: remove
	m_pHwShader = hwShaders[eHWSC_Compute].pHwShader;
	m_pHwShaderInstance = hwShaders[eHWSC_Compute].pHwShaderInstance;

	m_bValid = true;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: optimize compiled resource layout for cache friendliness
struct CDeviceResourceSet_DX11 : CDeviceResourceSet
{
	static const void* const InvalidPointer;

	struct SCompiledConstantBuffer
	{
		D3DBuffer* pBuffer;
		uint32 offset;
		uint32 size;
		uint64 code;
		int slot;
	};

	struct SCompiledBufferSRV
	{
		ID3D11ShaderResourceView* pSrv;
		int slot;
	};

	struct SCompiledUAV
	{
		ID3D11UnorderedAccessView* pUav;
		CDeviceManager::SHADER_TYPE shaderType;
		int slot;
	};

	CDeviceResourceSet_DX11(CDeviceResourceSet::EFlags flags)
		: CDeviceResourceSet(flags)
	{}

	CDeviceResourceSet_DX11(const CDeviceResourceSet& other)
		: CDeviceResourceSet(other)
	{
		ClearCompiledData();
	}

	virtual bool BuildImpl(EFlags updatedFlags) final;

	void         ClearCompiledData();

	// set via reflection from shader
	std::array<std::array<ID3D11ShaderResourceView*, MAX_TMU>, eHWSC_Num>                  compiledTextureSRVs;
	std::array<std::array<ID3D11SamplerState*, MAX_TMU>, eHWSC_Num>                        compiledSamplers;

	// set directly
	std::array<std::array<SCompiledConstantBuffer, eConstantBufferShaderSlot_Count>, eHWSC_Num> compiledCBs;
	std::array<uint8, eHWSC_Num> numCompiledCBs;

	std::array<std::array<SCompiledBufferSRV, ResourceSetBufferCount>, eHWSC_Num> compiledBufferSRVs;
	std::array<uint8, eHWSC_Num> numCompiledBufferSRVs;

	std::array<SCompiledUAV, 8> compiledUAVs;
	uint8 numCompiledUAVs;

	std::vector<_smart_ptr<ID3D11ShaderResourceView>>                                          m_SRVInUse;
	std::vector<_smart_ptr<ID3D11UnorderedAccessView>>                                         m_UAVInUse;
	std::vector<_smart_ptr<ID3D11SamplerState>>                                                m_SamplersInUse;
	std::vector<_smart_ptr<D3DBuffer>>                                                         m_CBInUse;

};

const void* const CDeviceResourceSet_DX11::InvalidPointer = (const void* const)0xBADA55;

void CDeviceResourceSet_DX11::ClearCompiledData()
{
	// Releasing resources is allowed to be done by any thread, just not concurrently
	m_SRVInUse.clear();
	m_UAVInUse.clear();
	m_SamplersInUse.clear();
	m_CBInUse.clear();

	for (auto& stage : compiledTextureSRVs)
		stage.fill((ID3D11ShaderResourceView*)InvalidPointer);

	for (auto& stage : compiledSamplers)
		stage.fill((ID3D11SamplerState*)InvalidPointer);

	ZeroMemory(&compiledCBs, sizeof(compiledCBs));
	ZeroMemory(&numCompiledCBs, sizeof(numCompiledCBs));

	ZeroMemory(&compiledBufferSRVs, sizeof(compiledBufferSRVs));
	ZeroMemory(&numCompiledBufferSRVs, sizeof(numCompiledBufferSRVs));

	ZeroMemory(&compiledUAVs, sizeof(compiledUAVs));
	ZeroMemory(&numCompiledUAVs, sizeof(numCompiledUAVs));
}

bool CDeviceResourceSet_DX11::BuildImpl(EFlags updatedFlags)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());
	CRY_ASSERT(m_Buffers.size() <= ResourceSetBufferCount);

	ClearCompiledData();

	bool bBuildSuccess = true;

	for (const auto& it : m_Textures)
	{
		bool bIsValid = false;

		const STextureData& textureData = it.second;
		if (CTexture* pTex = textureData.resource)
		{
			if (pTex->GetDevTexture())
			{
				SResourceView::KeyType srvKey = textureData.view;
				DeviceResourceBinding::SShaderSlot slot = it.first;

				CRY_ASSERT(slot.type == DeviceResourceBinding::eShaderSlotType_TextureAndBuffer || slot.type == DeviceResourceBinding::eShaderSlotType_UnorderedAccessView);
				CRY_ASSERT(SResourceView::IsShaderResourceView(srvKey) || SResourceView::IsUnorderedAccessView(srvKey));

				if (SResourceView::IsShaderResourceView(srvKey))
				{
					auto pSrv = reinterpret_cast<ID3D11ShaderResourceView*>(pTex->GetShaderResourceView(srvKey));
					m_SRVInUse.push_back(pSrv);

					for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
					{
						if (textureData.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
							compiledTextureSRVs[shaderClass][slot.slotNumber] = pSrv;
					}

					bIsValid = pSrv != nullptr;
				}
				else
				{
					CRY_ASSERT((textureData.shaderStages & (EShaderStage_Vertex | EShaderStage_Domain | EShaderStage_Hull | EShaderStage_Geometry)) == 0);

					D3DUAV* pUav = pTex->GetDeviceUAV();
					m_UAVInUse.push_back(pUav);

					if (textureData.shaderStages & EShaderStage_Compute)
					{
						SCompiledUAV compiledUAV = { pTex->GetDeviceUAV(), CDeviceManager::TYPE_CS, slot.slotNumber };
						compiledUAVs[numCompiledUAVs++] = compiledUAV;

						CRY_ASSERT(numCompiledUAVs <= D3D11_PS_CS_UAV_REGISTER_COUNT);
					}

					if (textureData.shaderStages & EShaderStage_Pixel)
					{
						SCompiledUAV compiledUAV = { pTex->GetDeviceUAV(), CDeviceManager::TYPE_PS, slot.slotNumber };
						compiledUAVs[numCompiledUAVs++] = compiledUAV;
					}

					bIsValid = pUav != nullptr;
				}
			}
		}

		bBuildSuccess &= bIsValid;
	}

	for (const auto& it : m_Buffers)
	{
		bool bIsValid = false;

		const SBufferData& bufferData = it.second;
		SResourceView::KeyType srvKey = bufferData.view;
		DeviceResourceBinding::SShaderSlot slot = it.first;

		CRY_ASSERT(slot.type == DeviceResourceBinding::eShaderSlotType_TextureAndBuffer || slot.type == DeviceResourceBinding::eShaderSlotType_UnorderedAccessView);
		CRY_ASSERT(SResourceView::IsShaderResourceView(srvKey) || SResourceView::IsUnorderedAccessView(srvKey));

		if (SResourceView::IsShaderResourceView(srvKey))
		{
			SCompiledBufferSRV compiledBuffer = { bufferData.resource.GetSRV(), slot.slotNumber };
			m_SRVInUse.push_back(compiledBuffer.pSrv);

			for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
			{
				if (bufferData.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
					compiledBufferSRVs[shaderClass][numCompiledBufferSRVs[shaderClass]++] = compiledBuffer;
			}

			bIsValid = (compiledBuffer.pSrv != nullptr);
		}
		else
		{
			D3DUAV* pUav = bufferData.resource.GetDeviceUAV();
			m_UAVInUse.push_back(pUav);

			if (bufferData.shaderStages & EShaderStage_Compute)
			{
				SCompiledUAV compiledUAV = { pUav, CDeviceManager::TYPE_CS, slot.slotNumber };
				compiledUAVs[numCompiledUAVs++] = compiledUAV;
			}

			if (bufferData.shaderStages & EShaderStage_Pixel)
			{
				SCompiledUAV compiledUAV = { pUav, CDeviceManager::TYPE_PS, slot.slotNumber };
				compiledUAVs[numCompiledUAVs++] = compiledUAV;
			}

			bIsValid = pUav != nullptr;
		}

		bBuildSuccess &= (bIsValid || (it.second.resource.m_flags & DX11BUF_NULL_RESOURCE)); // If an intentional null resource was passed, the ResourceSet remains valid.
	}

	for (const auto& it : m_Samplers)
	{
		const SSamplerData& samplerData = it.second;
		ID3D11SamplerState* pSamplerState = nullptr;
		DeviceResourceBinding::SShaderSlot slot = it.first;

		CRY_ASSERT(slot.type == DeviceResourceBinding::eShaderSlotType_Sampler);

		if (samplerData.resource >= 0 && samplerData.resource < CTexture::s_TexStates.size())
		{
			const auto& ts = CTexture::s_TexStates[samplerData.resource];
			pSamplerState = reinterpret_cast<ID3D11SamplerState*>(ts.m_pDeviceState);

			m_SamplersInUse.push_back(pSamplerState);
		}

		for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
		{
			if (samplerData.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
				compiledSamplers[shaderClass][slot.slotNumber] = pSamplerState;
		}

		bBuildSuccess &= pSamplerState != nullptr;
	}

	for (const auto& it : m_ConstantBuffers)
	{
		CRY_ASSERT(it.first.type == DeviceResourceBinding::eShaderSlotType_ConstantBuffer);

		SCompiledConstantBuffer compiledCB;
		ZeroStruct(compiledCB);

		if (it.second.resource)
		{
			size_t offset, size;
			compiledCB.pBuffer = it.second.resource->GetD3D(&offset, &size);
			compiledCB.code = it.second.resource->GetCode();
			compiledCB.offset = offset;
			compiledCB.size = size;
			compiledCB.slot = it.first.slotNumber;
		}

		for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
		{
			if (it.second.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
				compiledCBs[shaderClass][numCompiledCBs[shaderClass]++] = compiledCB;

			m_CBInUse.push_back(compiledCB.pBuffer);
		}

		bBuildSuccess &= (compiledCB.pBuffer != nullptr) || (it.second.resource && it.second.resource->m_intentionallyNull); // If an intentional null resource was passed, the ResourceSet remains valid.
	}

	return bBuildSuccess;
}

class CDeviceResourceLayout_DX11 : public CDeviceResourceLayout
{
public:
	CDeviceResourceLayout_DX11(UsedBindSlotSet requiredResourceBindings)
		: CDeviceResourceLayout(requiredResourceBindings)
	{}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(CRY_PLATFORM_ORBIS)

CDeviceTimestampGroup::CDeviceTimestampGroup()
	: m_numTimestamps(0)
	, m_pDisjointQuery(nullptr)
	, m_frequency(0L)
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
		gcpRendD3D->GetDevice().CreateQuery(&queryTimestampDesc, &m_timestampQueries[i]);
	}
	gcpRendD3D->GetDevice().CreateQuery(&queryDisjointDesc, &m_pDisjointQuery);
}

void CDeviceTimestampGroup::BeginMeasurement()
{
	m_numTimestamps = 0;
	gcpRendD3D->GetDeviceContext().Begin(m_pDisjointQuery);
}

void CDeviceTimestampGroup::EndMeasurement()
{
	gcpRendD3D->GetDeviceContext().End(m_pDisjointQuery);
}

uint32 CDeviceTimestampGroup::IssueTimestamp()
{
	assert(m_numTimestamps < m_timestampQueries.size());
	gcpRendD3D->GetDeviceContext().End(m_timestampQueries[m_numTimestamps]);
	return m_numTimestamps++;
}

bool CDeviceTimestampGroup::ResolveTimestamps()
{
	if (m_numTimestamps == 0)
		return true;

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
	if (gcpRendD3D->GetDeviceContext().GetData(m_pDisjointQuery, &disjointData, m_pDisjointQuery->GetDataSize(), 0) == S_OK)
	{
		m_frequency = disjointData.Frequency;
	}
	else
	{
		return false;
	}

	for (int i = m_numTimestamps - 1; i >= 0; i--)
	{
		if (gcpRendD3D->GetDeviceContext().GetData(m_timestampQueries[i], &m_timeValues[i], m_timestampQueries[i]->GetDataSize(), 0) != S_OK)
		{
			return false;
		}
	}

	return true;
}

#endif

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
#if defined(ENABLE_FRAME_PROFILER_LABELS)
	PROFILE_LABEL_GPU(label);
#endif
}

void CDeviceCommandListImpl::BeginProfilerEvent(const char* label)
{
#if defined(ENABLE_FRAME_PROFILER_LABELS)
	PROFILE_LABEL_PUSH_GPU(label);
#endif
}

void CDeviceCommandListImpl::EndProfilerEvent(const char* label)
{
#if defined(ENABLE_FRAME_PROFILER_LABELS)	
	PROFILE_LABEL_POP_GPU(label);
#endif
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

	m_sharedState.numSRVs.fill(0);
	m_sharedState.numSamplers.fill(0);

	m_graphicsState.custom.depthStencilState = nullptr;
	m_graphicsState.custom.rasterizerState = nullptr;
	m_graphicsState.custom.rasterizerStateIndex = 0;	
	m_graphicsState.custom.blendState = nullptr;
	m_graphicsState.custom.inputLayout = nullptr;
	m_graphicsState.custom.topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_graphicsState.custom.bDepthStencilStateDirty = true;
	m_graphicsState.custom.depthConstBias = 0.0f;
	m_graphicsState.custom.depthSlopeBias = 0.0f;
	m_graphicsState.custom.depthBiasClamp = 0.0f;
	m_graphicsState.custom.bRasterizerStateDirty = true;

	m_computeState.custom.boundUAVs = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandInterfaceImpl::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const
{
	if (pResources->GetFlags() & CDeviceResourceSet::EFlags_Multibuffered)
	{
		pResources->Build();
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetRenderTargetsImpl(uint32 targetCount, CTexture* const* pTargets, const SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews /*= nullptr*/)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	ID3D11RenderTargetView* pRTV[CD3D9Renderer::RT_STACK_WIDTH] = { NULL };

	for (int i = 0; i < targetCount; ++i)
	{
		SResourceView::KeyType rtvKey = SResourceView::RenderTargetView(pTargets[i]->GetDstFormat()).m_Desc.Key;
		if (pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargetView)
			rtvKey = pRenderTargetViews[i];

		CRY_ASSERT(SResourceView(rtvKey).m_Desc.eViewType == SResourceView::eRenderTargetView);

		if (auto pView = reinterpret_cast<ID3D11RenderTargetView*>(pTargets[i]->GetResourceView(rtvKey)))
			pRTV[i] = pView;
	}

	rd->GetDeviceContext().OMSetRenderTargets(targetCount, pRTV, pDepthTarget ? pDepthTarget->pSurface : NULL);
}

void CDeviceGraphicsCommandInterfaceImpl::SetViewportsImpl(uint32 vpCount, const D3DViewPort* pViewports)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->GetDeviceContext().RSSetViewports(vpCount, pViewports);
}

void CDeviceGraphicsCommandInterfaceImpl::SetScissorRectsImpl(uint32 rcCount, const D3DRectangle* pRects)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->GetDeviceContext().RSSetScissorRects(rcCount, pRects);
}

void CDeviceGraphicsCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceGraphicsPSO* devicePSO)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	const CDeviceGraphicsPSO_DX11* pDevicePSO = reinterpret_cast<const CDeviceGraphicsPSO_DX11*>(devicePSO);

	// RasterState, BlendState
	if (m_graphicsState.custom.blendState.Set(pDevicePSO->m_pBlendState.get()))
	{
		rd->m_DevMan.SetBlendState(pDevicePSO->m_pBlendState, NULL, 0xffffffff);
	}

	// Rasterizer state: NOTE - we don't know the final depth bias values yet so only mark dirty here and set in actual draw call
	if (m_graphicsState.custom.rasterizerState.Set(pDevicePSO->m_pRasterizerState.get()))
	{
		m_graphicsState.custom.rasterizerStateIndex = pDevicePSO->m_RasterizerStateIndex;
		m_graphicsState.custom.bRasterizerStateDirty = true;
	}

	// Depth stencil state: NOTE - we don't know the final stencil ref value yet so only mark dirty here and set in actual draw call
	if (m_graphicsState.custom.depthStencilState.Set(pDevicePSO->m_pDepthStencilState.get()))
	{
		m_graphicsState.custom.bDepthStencilStateDirty = true;
	}

	// Shaders
	const std::array<void*, eHWSC_Num>& shaders = pDevicePSO->m_pDeviceShaders;
	if (m_sharedState.shader[eHWSC_Vertex].Set(shaders[eHWSC_Vertex]))     rd->m_DevMan.BindShader(CDeviceManager::TYPE_VS, (ID3D11Resource*)shaders[eHWSC_Vertex]);
	if (m_sharedState.shader[eHWSC_Pixel].Set(shaders[eHWSC_Pixel]))       rd->m_DevMan.BindShader(CDeviceManager::TYPE_PS, (ID3D11Resource*)shaders[eHWSC_Pixel]);
	if (m_sharedState.shader[eHWSC_Geometry].Set(shaders[eHWSC_Geometry])) rd->m_DevMan.BindShader(CDeviceManager::TYPE_GS, (ID3D11Resource*)shaders[eHWSC_Geometry]);
	if (m_sharedState.shader[eHWSC_Domain].Set(shaders[eHWSC_Domain]))     rd->m_DevMan.BindShader(CDeviceManager::TYPE_DS, (ID3D11Resource*)shaders[eHWSC_Domain]);
	if (m_sharedState.shader[eHWSC_Hull].Set(shaders[eHWSC_Hull]))         rd->m_DevMan.BindShader(CDeviceManager::TYPE_HS, (ID3D11Resource*)shaders[eHWSC_Hull]);

	// input layout and topology
	if (m_graphicsState.custom.inputLayout.Set(pDevicePSO->m_pInputLayout.get()))   rd->m_DevMan.BindVtxDecl(pDevicePSO->m_pInputLayout);
	if (m_graphicsState.custom.topology.Set(pDevicePSO->m_PrimitiveTopology))       rd->m_DevMan.BindTopology(pDevicePSO->m_PrimitiveTopology);

	// update valid shader mask
	m_sharedState.validShaderStages = EShaderStage_None;
	if (pDevicePSO->m_pDeviceShaders[eHWSC_Vertex])   m_sharedState.validShaderStages |= EShaderStage_Vertex;
	if (pDevicePSO->m_pDeviceShaders[eHWSC_Pixel])    m_sharedState.validShaderStages |= EShaderStage_Pixel;
	if (pDevicePSO->m_pDeviceShaders[eHWSC_Geometry]) m_sharedState.validShaderStages |= EShaderStage_Geometry;
	if (pDevicePSO->m_pDeviceShaders[eHWSC_Domain])   m_sharedState.validShaderStages |= EShaderStage_Domain;
	if (pDevicePSO->m_pDeviceShaders[eHWSC_Hull])     m_sharedState.validShaderStages |= EShaderStage_Hull;

	m_sharedState.srvs = pDevicePSO->m_SRVs;
	m_sharedState.samplers = pDevicePSO->m_Samplers;

	m_sharedState.numSRVs = pDevicePSO->m_NumSRVs;
	m_sharedState.numSamplers = pDevicePSO->m_NumSamplers;

	// TheoM TODO: REMOVE once shaders are set up completely via pso
	rd->m_RP.m_FlagsShader_RT = pDevicePSO->m_ShaderFlags_RT;
	rd->m_RP.m_FlagsShader_MD = pDevicePSO->m_ShaderFlags_MD;
	rd->m_RP.m_FlagsShader_MDV = pDevicePSO->m_ShaderFlags_MDV;
}

void CDeviceGraphicsCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

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

// *INDENT-OFF*
void BindGraphicsSRV(EHWShaderClass shaderClass, ID3D11ShaderResourceView* pSrv, uint32 slot)
{
	CD3D9Renderer *const __restrict rd = gcpRendD3D;

	switch (shaderClass)
	{
	case eHWSC_Vertex:   rd->m_DevMan.BindSRV(CDeviceManager::TYPE_VS, pSrv, slot); break;
	case eHWSC_Pixel:    rd->m_DevMan.BindSRV(CDeviceManager::TYPE_PS, pSrv, slot); break;
	case eHWSC_Geometry: rd->m_DevMan.BindSRV(CDeviceManager::TYPE_GS, pSrv, slot); break;
	case eHWSC_Domain:   rd->m_DevMan.BindSRV(CDeviceManager::TYPE_DS, pSrv, slot); break;
	case eHWSC_Hull:     rd->m_DevMan.BindSRV(CDeviceManager::TYPE_HS, pSrv, slot); break;
	default:
		CRY_ASSERT(false);
	}
}

void BindGraphicsSampler(EHWShaderClass shaderClass, ID3D11SamplerState* pSamplerState, uint32 slot)
{
	CD3D9Renderer *const __restrict rd = gcpRendD3D;

	switch (shaderClass)
	{
	case eHWSC_Vertex:   rd->m_DevMan.BindSampler(CDeviceManager::TYPE_VS, pSamplerState, slot); break;
	case eHWSC_Pixel:    rd->m_DevMan.BindSampler(CDeviceManager::TYPE_PS, pSamplerState, slot); break;
	case eHWSC_Geometry: rd->m_DevMan.BindSampler(CDeviceManager::TYPE_GS, pSamplerState, slot); break;
	case eHWSC_Domain:   rd->m_DevMan.BindSampler(CDeviceManager::TYPE_DS, pSamplerState, slot); break;
	case eHWSC_Hull:     rd->m_DevMan.BindSampler(CDeviceManager::TYPE_HS, pSamplerState, slot); break;
	default:
		CRY_ASSERT(false);
	}
}

void BindGraphicsConstantBuffer(EHWShaderClass shaderClass, D3DBuffer* pBuffer, uint32 slot, uint32 offset, uint32 size)
{
	CD3D9Renderer *const __restrict rd = gcpRendD3D;

	switch (shaderClass)
	{
	case eHWSC_Vertex:   rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_VS, pBuffer, slot, offset, size); break;
	case eHWSC_Pixel:    rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_PS, pBuffer, slot, offset, size); break;
	case eHWSC_Geometry: rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_GS, pBuffer, slot, offset, size); break;
	case eHWSC_Domain:   rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_DS, pBuffer, slot, offset, size); break;
	case eHWSC_Hull:     rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_HS, pBuffer, slot, offset, size); break;
	default:
		CRY_ASSERT(false);
	}
}
// *INDENT-ON*

void CDeviceGraphicsCommandInterfaceImpl::SetResources_RequestedByShaderOnly(const CDeviceResourceSet* pResources)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (m_sharedState.validShaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
		{
			// Bind SRVs
			// if (!pResourcesDX11->compiledSRVs.empty()) // currently this is always the case
			{
				for (int i = 0; i < m_sharedState.numSRVs[shaderClass]; ++i)
				{
					uint8 srvSlot = m_sharedState.srvs[shaderClass][i];
					ID3D11ShaderResourceView* pSrv = pResourcesDX11->compiledTextureSRVs[shaderClass][srvSlot];

					if (pSrv != CDeviceResourceSet_DX11::InvalidPointer)
					{
						if (m_sharedState.shaderResourceView[shaderClass][srvSlot].Set(pSrv))
						{
							BindGraphicsSRV(shaderClass, pSrv, srvSlot);
						}
					}
				}
			}

			// Bind Samplers
			if (!pResourcesDX11->compiledSamplers.empty())
			{
				for (int i = 0; i < m_sharedState.numSamplers[shaderClass]; ++i)
				{
					uint8 samplerSlot = m_sharedState.samplers[shaderClass][i];
					ID3D11SamplerState* pSamplerState = pResourcesDX11->compiledSamplers[shaderClass][samplerSlot];

					if (pSamplerState != CDeviceResourceSet_DX11::InvalidPointer)
					{
						if (m_sharedState.samplerState[shaderClass][samplerSlot].Set(pSamplerState))
						{
							BindGraphicsSampler(shaderClass, pSamplerState, samplerSlot);
						}
					}
				}
			}

			// Bind buffers
			for (int i = 0; i < pResourcesDX11->numCompiledBufferSRVs[shaderClass]; ++i)
			{
				const CDeviceResourceSet_DX11::SCompiledBufferSRV& buffer = pResourcesDX11->compiledBufferSRVs[shaderClass][i];
				BindGraphicsSRV(shaderClass, buffer.pSrv, buffer.slot);
			}

			// Bind constant buffers
			for (int i = 0; i < pResourcesDX11->numCompiledCBs[shaderClass]; ++i)
			{
				const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[shaderClass][i];
				if (m_sharedState.constantBuffer[shaderClass][cb.slot].Set(cb.code))
				{
					BindGraphicsConstantBuffer(shaderClass, cb.pBuffer, cb.slot, cb.offset, cb.size);
				}
			}

			// Bind UAVs
			for (int i = 0; i < pResourcesDX11->numCompiledUAVs; ++i)
			{
				const CDeviceResourceSet_DX11::SCompiledUAV& uav = pResourcesDX11->compiledUAVs[i];
				rd->m_DevMan.BindUAV(uav.shaderType, uav.pUav, 0, uav.slot);
			}
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetResources_All(const CDeviceResourceSet* pResources)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		// Bind SRVs
		for (int slot = 0; slot < pResourcesDX11->compiledTextureSRVs[shaderClass].size(); ++slot)
		{
			ID3D11ShaderResourceView* pSrv = pResourcesDX11->compiledTextureSRVs[shaderClass][slot];

			if (pSrv != CDeviceResourceSet_DX11::InvalidPointer)
			{
				if (m_sharedState.shaderResourceView[shaderClass][slot].Set(pSrv))
				{
					BindGraphicsSRV(shaderClass, pSrv, slot);
				}
			}
		}

		// Bind Samplers
		for (int slot = 0; slot < pResourcesDX11->compiledSamplers[shaderClass].size(); ++slot)
		{
			ID3D11SamplerState* pSamplerState = pResourcesDX11->compiledSamplers[shaderClass][slot];

			if (pSamplerState != CDeviceResourceSet_DX11::InvalidPointer)
			{
				if (m_sharedState.samplerState[shaderClass][slot].Set(pSamplerState))
				{
					BindGraphicsSampler(shaderClass, pSamplerState, slot);
				}
			}
		}

		// Bind buffers
		for (int i = 0; i < pResourcesDX11->numCompiledBufferSRVs[shaderClass]; ++i)
		{
			const CDeviceResourceSet_DX11::SCompiledBufferSRV& buffer = pResourcesDX11->compiledBufferSRVs[shaderClass][i];
			{
				BindGraphicsSRV(shaderClass, buffer.pSrv, buffer.slot);
			}
		}

		// Bind constant buffers
		for (int i = 0; i < pResourcesDX11->numCompiledCBs[shaderClass]; ++i)
		{
			const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[shaderClass][i];
			if (m_sharedState.constantBuffer[shaderClass][cb.slot].Set(cb.code))
			{
				BindGraphicsConstantBuffer(shaderClass, cb.pBuffer, cb.slot, cb.offset, cb.size);
			}
		}

		// Bind UAVs
		for (int i = 0; i < pResourcesDX11->numCompiledUAVs; ++i)
		{
			const CDeviceResourceSet_DX11::SCompiledUAV& uav = pResourcesDX11->compiledUAVs[i];
			rd->m_DevMan.BindUAV(uav.shaderType, uav.pUav, 0, uav.slot);
		}
	}
}
// *INDENT-ON*

void CDeviceGraphicsCommandInterfaceImpl::ApplyDepthStencilState()
{
	if (m_graphicsState.custom.bDepthStencilStateDirty)
	{
		gcpRendD3D->m_DevMan.SetDepthStencilState(m_graphicsState.custom.depthStencilState.cachedValue, m_graphicsState.stencilRef.cachedValue);
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
			auto& rsDesc = gcpRendD3D->m_StatesRS[m_graphicsState.custom.rasterizerStateIndex].Desc;
			rsDesc.DepthBias = int(m_graphicsState.custom.depthConstBias);
			rsDesc.SlopeScaledDepthBias = m_graphicsState.custom.depthSlopeBias;
			rsDesc.DepthBiasClamp = m_graphicsState.custom.depthBiasClamp;

			uint32 newRasterizerStateIndex = gcpRendD3D->GetOrCreateRasterState(rsDesc);
			if (newRasterizerStateIndex >= 0)
			{
				pRasterizerState = gcpRendD3D->m_StatesRS[newRasterizerStateIndex].pState;
			}
		}

		gcpRendD3D->m_DevMan.SetRasterState(pRasterizerState);
		m_graphicsState.custom.bRasterizerStateDirty = false;
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
		{
			SetInlineConstantBufferImpl(bindSlot, pBuffer, shaderSlot, shaderClass);
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	if (m_sharedState.constantBuffer[shaderClass][shaderSlot].Set(pBuffer->GetCode()))
	{
		size_t offset, size;
		D3DBuffer* pBufferDX11 = pBuffer->GetD3D(&offset, &size);

		BindGraphicsConstantBuffer(shaderClass, pBufferDX11, shaderSlot, offset, size);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetVertexBuffersImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams)
{
	for (uint32 s = 0; s < numStreams; ++s)
	{
		const CDeviceInputStream& current = vertexStreams[s];

		CRY_ASSERT(current.hStream != ~0u);
		{
			size_t offset;
			D3DBuffer* buffer = gcpRendD3D.m_DevBufMan.GetD3D(current.hStream, &offset);

			gcpRendD3D.m_DevMan.BindVB(buffer, current.nSlot, offset, current.nStride);
		}
	}
}

void CDeviceGraphicsCommandInterfaceImpl::SetIndexBufferImpl(const CDeviceInputStream* indexStream)
{
	const CDeviceInputStream& current = indexStream[0];

	CRY_ASSERT(current.hStream != ~0u);
	{
		size_t offset;
		D3DBuffer* buffer = gcpRendD3D.m_DevBufMan.GetD3D(current.hStream, &offset);

	#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
		gcpRendD3D.m_DevMan.BindIB(buffer, offset, DXGI_FORMAT_R16_UINT);
	#else
		gcpRendD3D.m_DevMan.BindIB(buffer, offset, (DXGI_FORMAT)current.nStride);
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
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	ApplyDepthStencilState();
	ApplyRasterizerState();

	if (InstanceCount > 1)
	{
		rd->m_DevMan.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}
	else
	{
		rd->m_DevMan.Draw(VertexCountPerInstance, StartVertexLocation);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
	STATIC_CHECK(false, "NOT IMPLEMENTED");
	#endif

	ApplyDepthStencilState();
	ApplyRasterizerState();

	if (InstanceCount > 1)
	{
		rd->m_DevMan.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}
	else
	{
		rd->m_DevMan.DrawIndexed(IndexCountPerInstance, StartIndexLocation, BaseVertexLocation);
	}
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DSurface* pView, const float color[4], UINT numRects, const D3D11_RECT* pRects)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->FX_ClearTarget(pView, ColorF(color[0], color[1], color[2], color[3]), numRects, pRects);
}

void CDeviceGraphicsCommandInterfaceImpl::ClearSurfaceImpl(D3DDepthSurface* pView, int clearFlags, float depth, uint8 stencil, uint32 numRects, const D3D11_RECT* pRects)
{
	CRY_ASSERT(numRects == 0); // not supported on dx11
	gcpRendD3D->GetDeviceContext().ClearDepthStencilView(pView, D3D11_CLEAR_FLAG(clearFlags), depth, stencil);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceComputeCommandInterfaceImpl::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const
{
	if (pResources->GetFlags() & CDeviceResourceSet::EFlags_Multibuffered)
	{
		pResources->Build();
	}
}

void CDeviceComputeCommandInterfaceImpl::SetPipelineStateImpl(const CDeviceComputePSO* pDevicePSO)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	const CDeviceComputePSO_DX11* pDevicePsoDX11 = reinterpret_cast<const CDeviceComputePSO_DX11*>(pDevicePSO);

	// Shader
	const std::array<void*, eHWSC_Num>& shaders = pDevicePsoDX11->m_pDeviceShaders;
	rd->m_DevMan.BindShader(CDeviceManager::TYPE_CS, (ID3D11Resource*)shaders[eHWSC_Compute]);
}

void CDeviceComputeCommandInterfaceImpl::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	auto pResourcesDX11 = reinterpret_cast<const CDeviceResourceSet_DX11*>(pResources);

	// Bind SRVs
	for (int slot = 0; slot < pResourcesDX11->compiledTextureSRVs[eHWSC_Compute].size(); ++slot)
	{
		ID3D11ShaderResourceView* pSrv = pResourcesDX11->compiledTextureSRVs[eHWSC_Compute][slot];

		if (pSrv != CDeviceResourceSet_DX11::InvalidPointer)
		{
			if (m_sharedState.shaderResourceView[eHWSC_Compute][slot].Set(pSrv))
			{
				rd->m_DevMan.BindSRV(CDeviceManager::TYPE_CS, pSrv, slot);
			}
		}
	}

	// Bind Samplers
	for (int slot = 0; slot < pResourcesDX11->compiledSamplers[eHWSC_Compute].size(); ++slot)
	{
		ID3D11SamplerState* pSamplerState = pResourcesDX11->compiledSamplers[eHWSC_Compute][slot];

		if (pSamplerState != CDeviceResourceSet_DX11::InvalidPointer)
		{
			if (m_sharedState.samplerState[eHWSC_Compute][slot].Set(pSamplerState))
			{
				rd->m_DevMan.BindSampler(CDeviceManager::TYPE_CS, pSamplerState, slot);
			}
		}
	}

	// Bind buffers
	for (int i = 0; i < pResourcesDX11->numCompiledBufferSRVs[eHWSC_Compute]; ++i)
	{
		const CDeviceResourceSet_DX11::SCompiledBufferSRV& buffer = pResourcesDX11->compiledBufferSRVs[eHWSC_Compute][i];
		rd->m_DevMan.BindSRV(CDeviceManager::TYPE_CS, buffer.pSrv, buffer.slot);
	}

	// Bind constant buffers
	for (int i = 0; i < pResourcesDX11->numCompiledCBs[eHWSC_Compute]; ++i)
	{
		const CDeviceResourceSet_DX11::SCompiledConstantBuffer& cb = pResourcesDX11->compiledCBs[eHWSC_Compute][i];
		if (m_sharedState.constantBuffer[eHWSC_Compute][cb.slot].Set(cb.code))
		{
			rd->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_CS, cb.pBuffer, cb.slot, cb.offset, cb.size);
		}
	}

	// Bind UAVs
	for (int i = 0; i < pResourcesDX11->numCompiledUAVs; ++i)
	{
		const CDeviceResourceSet_DX11::SCompiledUAV& uav = pResourcesDX11->compiledUAVs[i];
		rd->m_DevMan.BindUAV(uav.shaderType, uav.pUav, 0, uav.slot);

		m_computeState.custom.boundUAVs[uav.slot] = true;
	}
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	//if (pCmdList->m_CurrentCB[shaderClass][shaderSlot].Set(pBuffer->GetCode()))
	{
		gcpRendD3D->m_DevMan.BindConstantBuffer(CDeviceManager::TYPE_CS, pBuffer, shaderSlot);
	}
}

void CDeviceComputeCommandInterfaceImpl::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	__debugbreak();
}

void CDeviceComputeCommandInterfaceImpl::DispatchImpl(uint32 X, uint32 Y, uint32 Z)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->m_DevMan.Dispatch(X, Y, Z);

	if (m_computeState.custom.boundUAVs.any())
	{
		for (int i = 0; i < m_computeState.custom.boundUAVs.size(); ++i)
		{
			if (m_computeState.custom.boundUAVs[i])
			{
				rd->m_DevMan.BindUAV(CDeviceManager::TYPE_CS, nullptr, 0, i);
			}
		}

		rd->m_DevMan.CommitDeviceStates();

		m_computeState.custom.boundUAVs = 0;
		memset(m_computeState.pResources, 0x0, sizeof(m_computeState.pResources));
	}
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
#if !defined(DEVICE_SUPPORTS_D3D11_1)
	if (NumRects) __debugbreak();
#endif
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->GetDeviceContext().ClearUnorderedAccessViewFloat(pView, Values);
}

void CDeviceComputeCommandInterfaceImpl::ClearUAVImpl(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects)
{
#if !defined(DEVICE_SUPPORTS_D3D11_1)
	if (NumRects) __debugbreak();
#endif
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->GetDeviceContext().ClearUnorderedAccessViewUint(pView, Values);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceNvidiaCommandInterfaceImpl::SetModifiedWModeImpl(bool enabled, uint32 numViewports, const float* pA, const float* pB)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	rd->GetDeviceContext().SetModifiedWMode(enabled, numViewports, pA, pB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceObjectFactory::CDeviceObjectFactory()
{
	m_pCoreCommandList = std::make_shared<CDeviceCommandList>();
}

CDeviceGraphicsPSOPtr CDeviceObjectFactory::CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceGraphicsPSO_DX11>();
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSOImpl(const CDeviceComputePSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceComputePSO_DX11>();
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceResourceSetPtr CDeviceObjectFactory::CreateResourceSet(CDeviceResourceSet::EFlags flags) const
{
	return std::make_shared<CDeviceResourceSet_DX11>(flags);
}

CDeviceResourceSetPtr CDeviceObjectFactory::CloneResourceSet(const CDeviceResourceSetPtr pSrcResourceSet) const
{
	return std::make_shared<CDeviceResourceSet_DX11>(*pSrcResourceSet);
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayoutImpl(const SDeviceResourceLayoutDesc& resourceLayoutDesc) const
{
	return std::make_shared<CDeviceResourceLayout_DX11>(resourceLayoutDesc.GetRequiredResourceBindings());
}

CDeviceCommandListPtr CDeviceObjectFactory::GetCoreCommandList() const
{
	return m_pCoreCommandList;
}

// Acquire one or more command-lists which are independent of the core command-list
// Only one thread is allowed to call functions on this command-list (DX12 restriction).
// The thread that gets the permition is the one calling Begin() on it AFAIS
CDeviceCommandListUPtr CDeviceObjectFactory::AcquireCommandList(EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// TODO: implement deferred contexts
	__debugbreak();

	return CryMakeUnique<CDeviceCommandList>();
}

std::vector<CDeviceCommandListUPtr> CDeviceObjectFactory::AcquireCommandLists(uint32 listCount, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// TODO: implement deferred contexts
	__debugbreak();

	return std::vector<CDeviceCommandListUPtr>(size_t(listCount));
}

void CDeviceObjectFactory::ForfeitCommandList(CDeviceCommandListUPtr pCommandList, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// TODO: implement deferred contexts
	__debugbreak();

	// pContext->ExecuteCommandList(ID3D11CommandList)
}

void CDeviceObjectFactory::ForfeitCommandLists(std::vector<CDeviceCommandListUPtr> pCommandLists, EQueueType eQueueType /*= eQueue_Graphics*/)
{
	// TODO: implement deferred contexts
	__debugbreak();

	// pContext->ExecuteCommandList(ID3D11CommandList)
}

void CDeviceObjectFactory::ReleaseResourcesImpl()
{

}

#endif
