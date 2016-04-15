// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/CryCustomTypes.h>
#include "../DeviceManager/DeviceWrapper12.h"
#include "DriverD3D.h"

#if defined(CRY_USE_DX12)
	#define CRY_USE_DX12_NATIVE
#endif

#if defined(CRY_USE_DX12_NATIVE)

using namespace NCryDX12;
D3D12_SHADER_BYTECODE g_EmptyShader;

static NCryDX12::CDevice* GetDevice()
{
	return reinterpret_cast<CCryDX12Device*>(gcpRendD3D->GetDevice().GetRealDevice())->GetDX12Device();
}

D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc(const CTexture* pTexture)
{
	D3D12_SRV_DIMENSION viewDimension[eTT_MaxTexType] =
	{
		D3D12_SRV_DIMENSION_TEXTURE1D,                      // eTT_1D
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_2D
		D3D12_SRV_DIMENSION_TEXTURE3D,                      // eTT_3D
		D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_Cube
		D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,               // eTT_CubeArray
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Dyn2D
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_User
		D3D12_SRV_DIMENSION_TEXTURECUBE,                    // eTT_NearestCube
		D3D12_SRV_DIMENSION_TEXTURE2DARRAY,                 // eTT_2DArray
		D3D12_SRV_DIMENSION_TEXTURE2DMS,                    // eTT_2DMS
		D3D12_SRV_DIMENSION_TEXTURE2D,                      // eTT_Auto2D
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroStruct(srvDesc);

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = pTexture ? viewDimension[pTexture->GetTexType()] : D3D12_SRV_DIMENSION_TEXTURE2D;

	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC GetNullSrvDesc(const CGpuBuffer& buffer)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroStruct(srvDesc);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	return srvDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceSet_DX12 : public CDeviceResourceSet
{
public:
	friend CDeviceGraphicsCommandList;
	friend CDeviceComputeCommandList;

	CDeviceResourceSet_DX12(CDevice* pDevice, CDeviceResourceSet::EFlags flags)
		: CDeviceResourceSet(flags)
		, m_pDevice(pDevice)
		, m_DescriptorBlockHandle(nullptr)
	{}

	CDeviceResourceSet_DX12(const CDeviceResourceSet& other)
		: CDeviceResourceSet(other)
		, m_pDevice(reinterpret_cast<const CDeviceResourceSet_DX12&>(other).m_pDevice)
		, m_DescriptorBlockHandle(nullptr)
	{}

	~CDeviceResourceSet_DX12()
	{
		if (m_DescriptorBlockHandle != nullptr)
		{
			gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(m_DescriptorBlockHandle);
		}
	}

	virtual void            Build() final;

	const CDescriptorBlock& GetDescriptorBlock() const { return m_DescriptorBlock; }

protected:
	DX12_PTR(CDevice) m_pDevice;
	SDescriptorBlock* m_DescriptorBlockHandle;
	CDescriptorBlock  m_DescriptorBlock;

	std::vector<_smart_ptr<CCryDX12Resource<ID3D11Resource>>> m_ResourcesInUse;
};

// requires no command-list, just a device
void CDeviceResourceSet_DX12::Build()
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	// NOTE: will deadlock multi-threaded command-lists when uploads occur on the core command-list (which has a fence-value larger than the active command-list)
	// TODO: call from somewhere safe
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	auto pCoreCommandList = pContext->GetCoreGraphicsCommandList();

	// Issue the barriers on the core command-list, which executes directly before the Draw()s in multi-threaded jobs
	CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList()->PrepareResourcesForUse(0, this);

	m_ResourcesInUse.clear();

	if (m_DescriptorBlockHandle != NULL)
	{
		gcpRendD3D->m_DevBufMan.ReleaseDescriptorBlock(m_DescriptorBlockHandle);
	}

	// CBV_SRV_UAV heap, SMP heap not yet supported
	m_DescriptorBlockHandle = gcpRendD3D->m_DevBufMan.CreateDescriptorBlock(m_ConstantBuffers.size() + m_Textures.size() + m_Buffers.size());
	m_DescriptorBlock = CDescriptorBlock(*m_DescriptorBlockHandle);
	m_bValid = true;

	for (const auto& it : m_ConstantBuffers)
	{
		auto cbData = it.second;
		if (!cbData.resource || !cbData.resource->m_buffer)
		{
			CRY_ASSERT(false && "An invalid constant buffer has been added to the ResourceSet. You must either insert a null-buffer if this was intended, or fix the code that causes this.");
			m_bValid = false;
			m_pDevice->GetD3D12Device()->CreateConstantBufferView(nullptr, m_DescriptorBlock.GetHandleOffsetCPU(0));
			m_DescriptorBlock.IncrementCursor();
			continue;
		}

		size_t start, length;
		NCryDX12::CView& pCBV = reinterpret_cast<CCryDX12Buffer*>(cbData.resource->GetD3D(&start, &length))->GetDX12View();
		assert(pCBV.GetType() == EVT_ConstantBufferView);

		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;

			cbvDesc = pCBV.GetCBVDesc();
			cbvDesc.BufferLocation += start;
			cbvDesc.SizeInBytes = length > 0 ? length : cbvDesc.SizeInBytes - start;
			cbvDesc.SizeInBytes = min(cbvDesc.SizeInBytes, D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * NCryDX12::CONSTANT_BUFFER_ELEMENT_SIZE);

			m_pDevice->GetD3D12Device()->CreateConstantBufferView(&cbvDesc, m_DescriptorBlock.GetHandleOffsetCPU(0));
		}

		{
			m_ResourcesInUse.push_back(reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(cbData.resource->m_buffer));
		}

		m_DescriptorBlock.IncrementCursor();
	}

	for (const auto& it : m_Textures)
	{
		SResourceView::KeyType srvKey = std::get<0>(it.second.resource);
		CTexture* pTexture = std::get<1>(it.second.resource);

		if (!pTexture || !pTexture->GetDevTexture() || !pTexture->GetDevTexture()->GetBaseTexture())
		{
			CRY_ASSERT(false && "An invalid texture has been added to the ResourceSet. You must either insert a null-texture if this was intended, or fix the code that causes this.");
			m_bValid = false;
			m_pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &GetNullSrvDesc(pTexture), m_DescriptorBlock.GetHandleOffsetCPU(0));
			m_DescriptorBlock.IncrementCursor();
			continue;
		}

		{
			NCryDX12::CView& srv = reinterpret_cast<CCryDX12ShaderResourceView*>(pTexture->GetShaderResourceView(srvKey))->GetDX12View();

			m_pDevice->GetD3D12Device()->CopyDescriptorsSimple(1, m_DescriptorBlock.GetHandleOffsetCPU(0), srv.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		{
			m_ResourcesInUse.push_back(reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pTexture->GetDevTexture()->GetBaseTexture()));
		}

		m_DescriptorBlock.IncrementCursor();
	}

	for (const auto& it : m_Buffers)
	{
		auto sbData = it.second;
		if (!sbData.resource.GetBuffer() || !sbData.resource.GetSRV())
		{
			CRY_ASSERT(false && "An invalid buffer has been added to the ResourceSet. You must either insert a null-buffer if this was intended, or fix the code that causes this.");
			m_bValid = false;
			m_pDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &GetNullSrvDesc(sbData.resource), m_DescriptorBlock.GetHandleOffsetCPU(0));
			m_DescriptorBlock.IncrementCursor();
			continue;
		}

		{
			NCryDX12::CView& srv = reinterpret_cast<CCryDX12ShaderResourceView*>(sbData.resource.GetSRV())->GetDX12View();

			m_pDevice->GetD3D12Device()->CopyDescriptorsSimple(1, m_DescriptorBlock.GetHandleOffsetCPU(0), srv.GetDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		{
			m_ResourcesInUse.push_back(reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(sbData.resource.GetBuffer()));
		}

		m_DescriptorBlock.IncrementCursor();
	}

	assert(m_DescriptorBlock.GetCursor() == m_DescriptorBlock.GetCapacity());

	// set descriptor block cursor to block start again
	m_DescriptorBlock.Reset();

	m_bDirty = !m_bValid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceResourceLayout_DX12 : public CDeviceResourceLayout
{
public:
	CDeviceResourceLayout_DX12(CDevice* pDevice, UsedBindSlotSet requiredResourceBindings)
		: CDeviceResourceLayout(requiredResourceBindings)
		, m_pDevice(pDevice)
	{}

	bool            Init(const SDeviceResourceLayoutDesc& desc);
	CRootSignature* GetRootSignature() const { return m_pRootSignature.get(); }

protected:
	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CRootSignature) m_pRootSignature;
};

bool CDeviceResourceLayout_DX12::Init(const SDeviceResourceLayoutDesc& desc)
{
	// *INDENT-OFF* - code formatter doesn't work so well on lambdas
	auto getShaderVisibility = [&](::EShaderStage shaderStages)
	{
		D3D12_SHADER_VISIBILITY shaderVisibility[eHWSC_Num + 1] =
		{
			D3D12_SHADER_VISIBILITY_VERTEX,     // eHWSC_Vertex
			D3D12_SHADER_VISIBILITY_PIXEL,      // eHWSC_Pixel
			D3D12_SHADER_VISIBILITY_GEOMETRY,   // eHWSC_Geometry
			D3D12_SHADER_VISIBILITY_ALL,        // eHWSC_Compute
			D3D12_SHADER_VISIBILITY_DOMAIN,     // eHWSC_Domain
			D3D12_SHADER_VISIBILITY_HULL,       // eHWSC_Hull
			D3D12_SHADER_VISIBILITY_ALL,        // eHWSC_Num
		};

		EHWShaderClass shaderClass = eHWSC_Num;

		if ((int(shaderStages) & (int(shaderStages)-1)) == 0) // only bound to a single shader stage?
		{
			for (shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass+1))
			{
				if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
					break;
			}
		}

		return shaderVisibility[shaderClass];
	};
	// *INDENT-ON*

	SResourceMappings resourceMappings;

	// inline constants
	if (desc.m_InlineConstantCount > 0)
	{
		resourceMappings.m_RootParameters[0].InitAsConstants(desc.m_InlineConstantCount, InlineConstantsShaderSlot);

		++resourceMappings.m_NumRootParameters;

		assert(resourceMappings.m_NumRootParameters <= 64);
	}

	// inline constant buffers
	for (auto it : desc.m_ConstantBuffers)
	{
		const int bindSlot = it.first;
		const SDeviceResourceLayoutDesc::SInlineConstantBuffer& cb = it.second;

		resourceMappings.m_RootParameters[bindSlot].InitAsConstantBufferView(cb.shaderSlot, 0, getShaderVisibility(cb.shaderStages));

		++resourceMappings.m_NumRootParameters;

		assert(resourceMappings.m_NumRootParameters <= 64);
	}

	// descriptor table resource sets
	for (auto it : desc.m_ResourceSets)
	{
		const int bindSlot = it.first;
		CDeviceResourceSet* pResourceSet = it.second.get();
		auto shaderVisibility = getShaderVisibility(pResourceSet->GetShaderStages());

		uint32 startDesc = resourceMappings.m_DescRangeCursor;
		uint32 tableOffset = 0;

		for (auto it : pResourceSet->m_ConstantBuffers)
		{
			const int shaderSlot = it.first;

			CD3DX12_DESCRIPTOR_RANGE cbDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, shaderSlot, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = cbDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		for (auto it : pResourceSet->m_Textures)
		{
			const int shaderSlot = it.first;

			CD3DX12_DESCRIPTOR_RANGE texDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, shaderSlot, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = texDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		for (auto it : pResourceSet->m_Buffers)
		{
			const int shaderSlot = it.first;

			CD3DX12_DESCRIPTOR_RANGE bufferDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, shaderSlot, 0, tableOffset);
			resourceMappings.m_DescRanges[resourceMappings.m_DescRangeCursor] = bufferDescRange;

			++resourceMappings.m_DescRangeCursor;
			++tableOffset;

			assert(resourceMappings.m_DescRangeCursor <= 128);
		}

		if ((resourceMappings.m_DescRangeCursor - startDesc) > 0)
		{
			resourceMappings.m_RootParameters[bindSlot].InitAsDescriptorTable(
			  resourceMappings.m_DescRangeCursor - startDesc, &resourceMappings.m_DescRanges[startDesc], shaderVisibility);

			++resourceMappings.m_NumRootParameters;

			assert(resourceMappings.m_NumRootParameters <= 64);
		}

		for (auto itSampler : pResourceSet->m_Samplers)
		{
			int shaderSlot = itSampler.first;
			const CDeviceResourceSet::SSamplerData& samplerData = itSampler.second;

			auto pDeviceState = reinterpret_cast<CCryDX12SamplerState*>(CTexture::s_TexStates[samplerData.resource].m_pDeviceState);
			auto samplerDesc = pDeviceState->GetDX12SamplerState().GetSamplerDesc();

			// copy parameters from sampler desc first
			memcpy(&resourceMappings.m_StaticSamplers[resourceMappings.m_NumStaticSamplers], &samplerDesc, sizeof(samplerDesc));

			// fill in remaining parameters
			auto& staticSamplerDesc = resourceMappings.m_StaticSamplers[resourceMappings.m_NumStaticSamplers];
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			staticSamplerDesc.MinLOD = samplerDesc.MinLOD;
			staticSamplerDesc.MaxLOD = samplerDesc.MaxLOD;
			staticSamplerDesc.ShaderRegister = shaderSlot;
			staticSamplerDesc.RegisterSpace = 0;
			staticSamplerDesc.ShaderVisibility = shaderVisibility;

			++resourceMappings.m_NumStaticSamplers;

			assert(resourceMappings.m_NumStaticSamplers <= 16);
		}
	}

	m_pDevice->GetRootSignatureCache().GetOrCreateRootSignature(resourceMappings, true, m_pDevice->GetNodeMask(), m_pRootSignature);
	return m_pRootSignature != nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceGraphicsPSO_DX12 : public CDeviceGraphicsPSO
{
public:

	CDeviceGraphicsPSO_DX12(CDevice* pDevice)
		: m_pDevice(pDevice)
	{}

	virtual bool             Init(const CDeviceGraphicsPSODesc& desc) final;

	CGraphicsPSO*            GetGraphicsPSO() const       { return m_pGraphicsPSO.get(); }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

protected:
	SOnDemandD3DVertexDeclaration m_InputLayout;
	D3D12_PRIMITIVE_TOPOLOGY      m_PrimitiveTopology;

	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CGraphicsPSO) m_pGraphicsPSO;
};

bool CDeviceGraphicsPSO_DX12::Init(const CDeviceGraphicsPSODesc& psoDesc)
{
	m_bValid = false;
	m_nUpdateCount++;

	if (psoDesc.m_pResourceLayout == NULL)
		return false;

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, psoDesc.m_bAllowTesselation);

	// validate shaders first
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (hwShaders[shaderClass].pHwShader && hwShaders[shaderClass].pHwShaderInstance == NULL)
			return false;

		// TODO: remove
		m_pHwShaders[shaderClass] = hwShaders[shaderClass].pHwShader;
		m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	D3D11_BLEND_DESC blendDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	psoDesc.FillDescs(rasterizerDesc, blendDesc, depthStencilDesc);

	// prepare pso init params
	CGraphicsPSO::SInitParams psoInitParams;
	ZeroStruct(psoInitParams);

	// root signature
	psoInitParams.m_Desc.pRootSignature = reinterpret_cast<CDeviceResourceLayout_DX12*>(psoDesc.m_pResourceLayout)->GetRootSignature()->GetD3D12RootSignature();

	// blend state
	{
		psoInitParams.m_Desc.BlendState.AlphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;
		psoInitParams.m_Desc.BlendState.IndependentBlendEnable = blendDesc.IndependentBlendEnable;

		for (int i = 0; i < CRY_ARRAY_COUNT(blendDesc.RenderTarget); ++i)
		{
			uint32 RenderTargetWriteMask = i < psoDesc.m_RenderTargetFormats.size() ? CTexture::WriteMaskFromTexFormat(psoDesc.m_RenderTargetFormats[i]) : 0;

			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendEnable = blendDesc.RenderTarget[i].BlendEnable;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].LogicOpEnable = false;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].SrcBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlend;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].DestBlend = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlend;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendOp = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOp;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].SrcBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].SrcBlendAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].DestBlendAlpha = (D3D12_BLEND)blendDesc.RenderTarget[i].DestBlendAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].BlendOpAlpha = (D3D12_BLEND_OP)blendDesc.RenderTarget[i].BlendOpAlpha;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
			psoInitParams.m_Desc.BlendState.RenderTarget[i].RenderTargetWriteMask = blendDesc.RenderTarget[i].RenderTargetWriteMask & RenderTargetWriteMask;
		}
	}

	// depth stencil and rasterizer state
	memcpy(&psoInitParams.m_Desc.DepthStencilState, &depthStencilDesc, sizeof(depthStencilDesc));
	memcpy(&psoInitParams.m_Desc.RasterizerState, &rasterizerDesc, sizeof(rasterizerDesc));

	auto extractShaderBytecode = [&](EHWShaderClass shaderClass)
	{
		return hwShaders[shaderClass].pHwShader
		       ? reinterpret_cast<CCryDX12Shader*>(hwShaders[shaderClass].pDeviceShader)->GetD3D12ShaderBytecode()
		       : g_EmptyShader;
	};

	psoInitParams.m_Desc.NodeMask = m_pDevice->GetNodeMask();
	psoInitParams.m_Desc.VS = extractShaderBytecode(eHWSC_Vertex);
	psoInitParams.m_Desc.DS = extractShaderBytecode(eHWSC_Domain);
	psoInitParams.m_Desc.HS = extractShaderBytecode(eHWSC_Hull);
	psoInitParams.m_Desc.GS = extractShaderBytecode(eHWSC_Geometry);
	psoInitParams.m_Desc.PS = extractShaderBytecode(eHWSC_Pixel);

	psoInitParams.m_Desc.SampleMask = UINT_MAX;
	psoInitParams.m_Desc.SampleDesc.Count = 1;

	// primitive topology
	{
		m_PrimitiveTopology = gcpRendD3D->FX_ConvertPrimitiveType(psoDesc.m_PrimitiveType);

		struct
		{
			ERenderPrimitiveType          primitiveType;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;
		}
		topologyTypes[] =
		{
			{ eptUnknown,                D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED },
			{ eptTriangleList,           D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
			{ eptTriangleStrip,          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  },
			{ eptLineList,               D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
			{ eptLineStrip,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE      },
			{ eptPointList,              D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT     },
			{ ept1ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept2ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept3ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
			{ ept4ControlPointPatchList, D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     },
		};

		psoInitParams.m_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		for (int i = 0; i < CRY_ARRAY_COUNT(topologyTypes); ++i)
		{
			if (topologyTypes[i].primitiveType == psoDesc.m_PrimitiveType)
			{
				psoInitParams.m_Desc.PrimitiveTopologyType = topologyTypes[i].primitiveTopology;
				break;
			}
		}
	}

	// input layout
	m_InputLayout.m_Declaration.SetUse(0);
	if (CHWShader_D3D::SHWSInstance* pVsInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(hwShaders[eHWSC_Vertex].pHwShaderInstance))
	{
		uint8 streamMask = psoDesc.CombineVertexStreamMasks(uint8(pVsInstance->m_VStreamMask_Decl), psoDesc.m_ObjectStreamMask);

		const bool bMorph = false;
		const bool bInstanced = (streamMask & VSM_INSTANCED) != 0;

		gcpRendD3D->EF_OnDemandVertexDeclaration(m_InputLayout, streamMask >> 1, psoDesc.m_VertexFormat, bMorph, bInstanced);

		psoInitParams.m_Desc.InputLayout.pInputElementDescs = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(&m_InputLayout.m_Declaration[0]);
		psoInitParams.m_Desc.InputLayout.NumElements = m_InputLayout.m_Declaration.size();
	}

	// render targets
	for (int i = 0; i < psoDesc.m_RenderTargetFormats.size(); ++i)
	{
		psoInitParams.m_Desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

		if (psoDesc.m_RenderTargetFormats[i] != eTF_Unknown)
		{
			psoInitParams.m_Desc.RTVFormats[i] = CTexture::DeviceFormatFromTexFormat(psoDesc.m_RenderTargetFormats[i]);
			psoInitParams.m_Desc.NumRenderTargets = i + 1;
		}
	}

	if (psoDesc.m_DepthStencilFormat != eTF_Unknown)
		psoInitParams.m_Desc.DSVFormat = CTexture::DeviceFormatFromTexFormat(psoDesc.m_DepthStencilFormat);

	#if defined(ENABLE_PROFILING_CODE)
	m_PrimitiveTypeForProfiling = psoDesc.m_PrimitiveType;
	#endif

	m_pDevice->GetPSOCache().GetOrCreatePSO(psoInitParams, m_pGraphicsPSO);

	return (m_bValid = (m_pGraphicsPSO != nullptr));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDeviceComputePSO_DX12 : public CDeviceComputePSO
{
public:

	CDeviceComputePSO_DX12(CDevice* pDevice)
		: m_pDevice(pDevice)
	{}

	virtual bool Init(const CDeviceComputePSODesc& desc) final;

	CComputePSO* GetComputePSO() const { return m_pComputePSO.get(); }

protected:

	DX12_PTR(CDevice) m_pDevice;
	DX12_PTR(CComputePSO) m_pComputePSO;
};

bool CDeviceComputePSO_DX12::Init(const CDeviceComputePSODesc& psoDesc)
{
	m_bValid = false;

	if (psoDesc.m_pResourceLayout == NULL)
		return false;

	auto hwShaders = SDeviceObjectHelpers::GetShaderInstanceInfo(psoDesc.m_pShader, psoDesc.m_technique, psoDesc.m_ShaderFlags_RT, psoDesc.m_ShaderFlags_MD, psoDesc.m_ShaderFlags_MDV, nullptr, false);

	// validate shaders first
	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (hwShaders[shaderClass].pHwShader && hwShaders[shaderClass].pHwShaderInstance == NULL)
			return false;

		// TODO: remove
		m_pHwShaders[shaderClass] = hwShaders[shaderClass].pHwShader;
		m_pHwShaderInstances[shaderClass] = hwShaders[shaderClass].pHwShaderInstance;
	}

	// prepare pso init params
	CComputePSO::SInitParams psoInitParams;
	ZeroStruct(psoInitParams);

	// root signature
	psoInitParams.m_Desc.pRootSignature = reinterpret_cast<CDeviceResourceLayout_DX12*>(psoDesc.m_pResourceLayout)->GetRootSignature()->GetD3D12RootSignature();

	// *INDENT-OFF*
	auto extractShaderBytecode = [&](EHWShaderClass shaderClass)
	{
		return hwShaders[shaderClass].pHwShader
			? reinterpret_cast<CCryDX12Shader*>(hwShaders[shaderClass].pDeviceShader)->GetD3D12ShaderBytecode()
			: g_EmptyShader;
	};
	// *INDENT-ON*

	psoInitParams.m_Desc.NodeMask = m_pDevice->GetNodeMask();
	psoInitParams.m_Desc.CS = extractShaderBytecode(eHWSC_Compute);

	m_pDevice->GetPSOCache().GetOrCreatePSO(psoInitParams, m_pComputePSO);

	return (m_bValid = (m_pComputePSO != nullptr));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceGraphicsCommandList_DX12 : public CDeviceGraphicsCommandList
{
public:
	CDeviceGraphicsCommandList_DX12(NCryDX12::CCommandList* pCommandList)
		: m_pCommandList(pCommandList)
	{
		Reset();
	}

	D3D12_PRIMITIVE_TOPOLOGY m_CurrentPrimitiveTopology;
	DX12_PTR(NCryDX12::CCommandList) m_pCommandList;
};

	#define GET_DX12_GRAPHICS_COMMANDLIST(abstractCommandList)       reinterpret_cast<CDeviceGraphicsCommandList_DX12*>(abstractCommandList)
	#define GET_DX12_GRAPHICS_COMMANDLIST_CONST(abstractCommandList) reinterpret_cast<const CDeviceGraphicsCommandList_DX12*>(abstractCommandList)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandList::PrepareRenderTargetsForUse(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews) const
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

	const NCryDX12::CView* pDSV = nullptr;
	const NCryDX12::CView* pRTV[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

	// Get current depth stencil views
	if (pDepthTarget)
	{
		if (auto pView = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthTarget->pSurface))
		{
			pDSV = &pView->GetDX12View();

			D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
			pView->GetDesc(&Desc);
			D3D12_RESOURCE_STATES State = Desc.Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;
			NCryDX12::CResource& Resource = pDSV->GetDX12Resource();

			if (Resource.NeedsTransitionBarrier(pCommandListDX12, *pDSV, State))
			{
				pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
				pCommandListDX12->EndResourceStateTransition(Resource, *pDSV, State);
				pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
			}
		}
	}

	// Get current render target views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			const bool bHasCustomView = pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargtView;
			SResourceView::KeyType rtvKey = bHasCustomView ? pRenderTargetViews[i] : SResourceView::RenderTargetView(pTargets[i]->GetDstFormat()).m_Desc.Key;
			CRY_ASSERT(SResourceView(rtvKey).m_Desc.eViewType == SResourceView::eRenderTargetView);

			if (auto pView = reinterpret_cast<CCryDX12RenderTargetView*>(pTargets[i]->GetResourceView(rtvKey).m_pDeviceResourceView))
			{
				pRTV[i] = &pCommandListDX12->PatchRenderTargetView(pView->GetDX12View());

				D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_RENDER_TARGET;
				NCryDX12::CResource& Resource = pRTV[i]->GetDX12Resource();

				if (Resource.NeedsTransitionBarrier(pCommandListDX12, *pRTV[i], State))
				{
					pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
					pCommandListDX12->EndResourceStateTransition(Resource, *pRTV[i], State);
					pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
				}
			}
		}
	}
}

void CDeviceGraphicsCommandList::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResourceSet) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();
	CDeviceResourceSet_DX12* pResources = reinterpret_cast<CDeviceResourceSet_DX12*>(pResourceSet);

	for (auto& it : pResources->m_ConstantBuffers)
	{
		if (CConstantBuffer* pBuffer = it.second.resource)
		{
			if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->m_buffer))
			{
				NCryDX12::CResource& Resource = pResource->GetDX12Resource();

				if (Resource.InitHasBeenDeferred())
				{
					Resource.InitDeferred(pCommandListDX12);
				}

				if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
				{
					pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
					pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
					pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
				}
			}
		}
	}

	for (auto& it : pResources->m_Textures)
	{
		if (CTexture* pTexture = std::get<1>(it.second.resource))
		{
			if (CDeviceTexture* pDevTexture = pTexture->GetDevTexture())
			{
				if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pDevTexture->GetBaseTexture()))
				{
					NCryDX12::CResource& Resource = pResource->GetDX12Resource();

					if (Resource.InitHasBeenDeferred())
					{
						Resource.InitDeferred(pCommandListDX12);
					}

					if (auto pSrv = reinterpret_cast<CCryDX12ShaderResourceView*>(pTexture->GetShaderResourceView(std::get<0>(it.second.resource))))
					{
						if (Resource.NeedsTransitionBarrier(pCommandListDX12, pSrv->GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
						{
							pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
							pCommandListDX12->EndResourceStateTransition(Resource, pSrv->GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
							pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
						}
					}
				}
			}
		}
	}

	for (auto& it : pResources->m_Buffers)
	{
		if (ID3D11Buffer* pBuffer = it.second.resource.GetBuffer())
		{
			if (auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer))
			{
				NCryDX12::CResource& Resource = pResource->GetDX12Resource();

				if (Resource.InitHasBeenDeferred())
				{
					Resource.InitDeferred(pCommandListDX12);
				}

				if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
				{
					pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
					pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
				}
			}
		}
	}
}

void CDeviceGraphicsCommandList::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

	auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->m_buffer);
	NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();

	if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
	{
		pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
		pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
	}
}

void CDeviceGraphicsCommandList::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	PrepareInlineConstantBufferForUseImpl(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceGraphicsCommandList::PrepareVertexBuffersForUse(uint32 streamCount, const CDeviceInputStream* vertexStreams) const
{
	if (m_CurrentVertexStreams != vertexStreams)
	{
		NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

		for (uint32 i = 0; i < streamCount; ++i)
		{
			// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
			size_t offset;
			if (auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(vertexStreams[i].hStream != ~0u ? gcpRendD3D.m_DevBufMan.GetD3D(vertexStreams[i].hStream, &offset) : nullptr))
			{
				NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();
				if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
				{
					pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
					pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
					pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
				}
			}
		}
	}
}

void CDeviceGraphicsCommandList::PrepareIndexBufferForUse(const CDeviceInputStream* indexStream) const
{
	if (m_CurrentIndexStream != indexStream)
	{
		NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

		// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
		size_t offset;
		if (auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(indexStream->hStream != ~0u ? gcpRendD3D.m_DevBufMan.GetD3D(indexStream->hStream, &offset) : nullptr))
		{
			NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();
			if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_INDEX_BUFFER))
			{
				pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
				pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceGraphicsCommandList::SetRenderTargets(uint32 targetCount, CTexture* const* pTargets, const SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

	const NCryDX12::CView* pDSV = nullptr;
	const NCryDX12::CView* pRTV[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

	// Get current depth stencil views
	if (pDepthTarget)
	{
		if (auto pView = reinterpret_cast<CCryDX12DepthStencilView*>(pDepthTarget->pSurface))
		{
			pDSV = &pView->GetDX12View();

			D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
			pView->GetDesc(&Desc);
			D3D12_RESOURCE_STATES State = Desc.Flags & D3D11_DSV_READ_ONLY_DEPTH ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;

			assert(!pDSV->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, *pDSV, State));
		}
	}

	// Get current render target views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			SResourceView::KeyType rtvKey = SResourceView::RenderTargetView(pTargets[i]->GetDstFormat()).m_Desc.Key;
			if (pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargtView)
				rtvKey = pRenderTargetViews[i];

			CRY_ASSERT(SResourceView(rtvKey).m_Desc.eViewType == SResourceView::eRenderTargetView);

			if (auto pView = reinterpret_cast<CCryDX12RenderTargetView*>(pTargets[i]->GetResourceView(rtvKey).m_pDeviceResourceView))
			{
				pRTV[i] = &pCommandListDX12->PatchRenderTargetView(pView->GetDX12View());

				assert(!pRTV[i]->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, *pRTV[i], D3D12_RESOURCE_STATE_RENDER_TARGET));
			}
		}
	}

	// TODO: if we know early that the resource(s) will be RENDER_TARGET/DEPTH_READ|WRITE we can begin the barrier early and end it here
	pCommandListDX12->BindAndSetOutputViews(targetCount, pRTV, pDSV);
}

void CDeviceGraphicsCommandList::SetViewports(uint32 vpCount, const D3DViewPort* pViewports)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

	// D3D11_VIEWPORT := D3D12_VIEWPORT
	pCommandListDX12->SetViewports(vpCount, (D3D12_VIEWPORT*)pViewports);
}

void CDeviceGraphicsCommandList::SetScissorRects(uint32 rcCount, const D3DRectangle* pRects)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

	// D3D11_RECT := D3D12_RECT
	pCommandListDX12->SetScissorRects(rcCount, (D3D12_RECT*)pRects);
}

void CDeviceGraphicsCommandList::SetPipelineStateImpl(const CDeviceGraphicsPSOConstPtr& devicePSO)
{
	auto pDeviceCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
	NCryDX12::CCommandList* pCommandListDX12 = pDeviceCommandListDX12->m_pCommandList.get();
	const CDeviceGraphicsPSO_DX12* pDevicePSO = reinterpret_cast<const CDeviceGraphicsPSO_DX12*>(devicePSO.get());

	pCommandListDX12->SetPSO(pDevicePSO->GetGraphicsPSO());

	D3D12_PRIMITIVE_TOPOLOGY PSOPrimitiveTopology = pDevicePSO->GetPrimitiveTopology();
	if (pDeviceCommandListDX12->m_CurrentPrimitiveTopology != PSOPrimitiveTopology)
	{
		pDeviceCommandListDX12->m_CurrentPrimitiveTopology = PSOPrimitiveTopology;

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumTopologySets);
	#endif

		pCommandListDX12->SetPrimitiveTopology(PSOPrimitiveTopology);
	}
}

void CDeviceGraphicsCommandList::SetResourceLayoutImpl(const CDeviceResourceLayoutConstPtr& resourceLayout)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	const CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<const CDeviceResourceLayout_DX12*>(resourceLayout.get());

	pCommandListDX12->SetGraphicsRootSignature(pResourceLayoutDX12->GetRootSignature());
}

void CDeviceGraphicsCommandList::SetVertexBuffers(uint32 streamCount, const CDeviceInputStream* vertexStreams)
{
	if (m_CurrentVertexStreams.Set(vertexStreams))
	{
		NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

		pCommandListDX12->ClearVertexBufferHeap(streamCount);

		for (uint32 i = 0; i < streamCount; ++i)
		{
			// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
			size_t offset;
			if (auto* pBuffer = reinterpret_cast<CCryDX12Buffer*>(vertexStreams[i].hStream != ~0u ? gcpRendD3D.m_DevBufMan.GetD3D(vertexStreams[i].hStream, &offset) : nullptr))
			{
				assert(!pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
				CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundVertexBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

				pCommandListDX12->BindVertexBufferView(pBuffer->GetDX12View(), i, TRange<uint32>(offset, offset), vertexStreams[i].nStride);
			}
		}

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
		pCommandListDX12->SetVertexBufferHeap(streamCount);
	}
}

void CDeviceGraphicsCommandList::SetIndexBuffer(const CDeviceInputStream* indexStream)
{
	if (m_CurrentIndexStream.Set(indexStream))
	{
		NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

		// TODO: try not to call GetD3D() here, overhead: 1 call (and no inlinening) + 1 look-up + 2 accesses + branch
		size_t offset;
		auto* pBuffer = reinterpret_cast<CCryDX12Buffer*>(indexStream->hStream != ~0u ? gcpRendD3D.m_DevBufMan.GetD3D(indexStream->hStream, &offset) : nullptr);

		assert(pBuffer && !pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
		CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundIndexBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

		// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	#if !defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
		pCommandListDX12->BindAndSetIndexBufferView(pBuffer->GetDX12View(), DXGI_FORMAT_R16_UINT, offset);
	#else
		pCommandListDX12->BindAndSetIndexBufferView(pBuffer->GetDX12View(), (DXGI_FORMAT)indexStream->nStride, UINT(offset));
	#endif
	}
}

void CDeviceGraphicsCommandList::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResourceSet)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	const CDeviceResourceSet_DX12* pResources = reinterpret_cast<const CDeviceResourceSet_DX12*>(pResourceSet);

	for (auto& it : pResources->m_ConstantBuffers)
	{
		if (CConstantBuffer* pBuffer = it.second.resource)
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->m_buffer);

			assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
			CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundConstBuffers[pResource->GetDX12Resource().IsOffCard()]);
	#endif

			// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
			pCommandListDX12->TrackResourceCBVUsage(pResource->GetDX12Resource());
		}
	}

	for (auto& it : pResources->m_Textures)
	{
		if (CTexture* pTexture = std::get<1>(it.second.resource))
		{
			if (CDeviceTexture* pDevTexture = pTexture->GetDevTexture())
			{
				if (auto pSrv = reinterpret_cast<CCryDX12ShaderResourceView*>(pTexture->GetShaderResourceView(std::get<0>(it.second.resource))))
				{
					auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pDevTexture->GetBaseTexture());
					assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, pSrv->GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	#if defined(ENABLE_PROFILING_CODE)
					CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformTextures[pResource->GetDX12Resource().IsOffCard()]);
	#endif

					// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
					pCommandListDX12->TrackResourcePRVUsage(pResource->GetDX12Resource(), pSrv->GetDX12View());
				}
			}
		}
	}

	for (auto& it : pResources->m_Buffers)
	{
		if (ID3D11Buffer* pBuffer = it.second.resource.GetBuffer())
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer);

			assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	#if defined(ENABLE_PROFILING_CODE)
			CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformBuffers[pResource->GetDX12Resource().IsOffCard()]);
	#endif

			// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
			if (auto pView = reinterpret_cast<CCryDX12ShaderResourceView*>(it.second.resource.GetSRV()))
			{
				pCommandListDX12->TrackResourcePRVUsage(pResource->GetDX12Resource(), pView->GetDX12View());
			}
		}
	}

	const CDescriptorBlock& descriptorBlock = pResources->GetDescriptorBlock();
	pCommandListDX12->SetGraphicsDescriptorTable(bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceGraphicsCommandList::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	SetInlineConstantBuffer(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceGraphicsCommandList::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

	auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->m_buffer);

	assert(!pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
	CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundInlineBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

	// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	pCommandListDX12->TrackResourceCBVUsage(pBuffer->GetDX12Resource());

	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetDX12View().GetCBVDesc().BufferLocation + pConstantBuffer->m_offset;
	pCommandListDX12->SetGraphicsConstantBufferView(bindSlot, gpuAddress);
}

void CDeviceGraphicsCommandList::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->SetGraphics32BitConstants(bindSlot, constantCount, pConstants, 0);
}

void CDeviceGraphicsCommandList::SetStencilRefImpl(uint8 stencilRefValue)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->SetStencilRef(stencilRefValue);
}

void CDeviceGraphicsCommandList::DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandList::DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CDeviceGraphicsCommandList::ClearSurface(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRect)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();

	auto pViewDX12 = reinterpret_cast<CCryDX12RenderTargetView*>(pView);

	pCommandListDX12->ClearRenderTargetView(pViewDX12->GetDX12View(), Color, NumRects, pRect);
}

void CDeviceGraphicsCommandList::LockToThread()
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->Begin();
	pCommandListDX12->SetResourceAndSamplerStateHeaps();
}

void CDeviceGraphicsCommandList::Build()
{
	FUNCTION_PROFILER_RENDERER

	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->End();
}

void CDeviceGraphicsCommandList::ResetImpl()
{
	auto pDeviceCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(this);
	pDeviceCommandListDX12->m_CurrentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceComputeCommandList_DX12 : public CDeviceComputeCommandList
{
public:
	CDeviceComputeCommandList_DX12(NCryDX12::CCommandList* pCommandList)
		: m_pCommandList(pCommandList)
	{
		Reset();
	}

	DX12_PTR(NCryDX12::CCommandList) m_pCommandList;
};

	#define GET_DX12_COMPUTE_COMMANDLIST(abstractCommandList)       reinterpret_cast<CDeviceComputeCommandList_DX12*>(abstractCommandList)
	#define GET_DX12_COMPUTE_COMMANDLIST_CONST(abstractCommandList) reinterpret_cast<const CDeviceComputeCommandList_DX12*>(abstractCommandList)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceComputeCommandList::PrepareUATargetsForUse(uint32 targetCount, CGpuBuffer** pTargets) const
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST_CONST(this)->m_pCommandList.get();

	const NCryDX12::CView* pRTV[D3D12_UAV_SLOT_COUNT] = { nullptr };

	// Get current unordered access views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			if (auto pView = reinterpret_cast<CCryDX12UnorderedAccessView*>(pTargets[i]->GetUAV()))
			{
				pRTV[i] = &pView->GetDX12View();

				D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				NCryDX12::CResource& Resource = pRTV[i]->GetDX12Resource();

				if (Resource.NeedsTransitionBarrier(pCommandListDX12, *pRTV[i], State))
				{
					pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
					pCommandListDX12->EndResourceStateTransition(Resource, *pRTV[i], State);
					pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
				}
			}
		}
	}
}

void CDeviceComputeCommandList::PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResourceSet) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();
	CDeviceResourceSet_DX12* pResources = reinterpret_cast<CDeviceResourceSet_DX12*>(pResourceSet);

	for (auto& it : pResources->m_ConstantBuffers)
	{
		if (CConstantBuffer* pBuffer = it.second.resource)
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->m_buffer);
			NCryDX12::CResource& Resource = pResource->GetDX12Resource();

			if (Resource.InitHasBeenDeferred())
			{
				Resource.InitDeferred(pCommandListDX12);
			}

			if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
			{
				pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
				pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
			}
		}
	}

	for (auto& it : pResources->m_Textures)
	{
		if (CTexture* pTexture = std::get<1>(it.second.resource))
		{
			if (CDeviceTexture* pDevTexture = pTexture->GetDevTexture())
			{
				auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pDevTexture->GetBaseTexture());
				NCryDX12::CResource& Resource = pResource->GetDX12Resource();

				if (Resource.InitHasBeenDeferred())
				{
					Resource.InitDeferred(pCommandListDX12);
				}

				if (auto pView = reinterpret_cast<CCryDX12ShaderResourceView*>(pTexture->GetShaderResourceView(std::get<0>(it.second.resource))))
				{
					if (Resource.NeedsTransitionBarrier(pCommandListDX12, pView->GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
					{
						pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
						pCommandListDX12->EndResourceStateTransition(Resource, pView->GetDX12View(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
						pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
					}
				}
			}
		}
	}

	for (auto& it : pResources->m_Buffers)
	{
		if (ID3D11Buffer* pBuffer = it.second.resource.GetBuffer())
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer);
			NCryDX12::CResource& Resource = pResource->GetDX12Resource();

			if (Resource.InitHasBeenDeferred())
			{
				Resource.InitDeferred(pCommandListDX12);
			}

			if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
			{
				pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
				pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
			}
		}
	}
}

void CDeviceComputeCommandList::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST_CONST(this)->m_pCommandList.get();

	auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->m_buffer);
	NCryDX12::CResource& Resource = pBuffer->GetDX12Resource();

	if (Resource.NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
	{
		pCommandListDX12->MaxResourceFenceValue(Resource, CMDTYPE_ANY);
		pCommandListDX12->EndResourceStateTransition(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		pCommandListDX12->SetResourceFenceValue(Resource, CMDTYPE_WRITE);
	}
}

void CDeviceComputeCommandList::PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages) const
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	PrepareInlineConstantBufferForUseImpl(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceComputeCommandList::SetUATargets(uint32 targetCount, const CGpuBuffer* const* pTargets)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();

	const NCryDX12::CView* pRTV[D3D12_UAV_SLOT_COUNT] = { nullptr };

	// Get current unordered access views
	for (int i = 0; i < targetCount; ++i)
	{
		if (pTargets[i])
		{
			if (auto pView = reinterpret_cast<CCryDX12RenderTargetView*>(pTargets[i]->GetUAV()))
			{
				pRTV[i] = &pCommandListDX12->PatchRenderTargetView(pView->GetDX12View());

				assert(!pRTV[i]->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, pView->GetDX12View(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			}
		}
	}

	// TODO: if we know early that the resource(s) will be RENDER_TARGET/DEPTH_READ|WRITE we can begin the barrier early and end it here
	// pCommandListDX12->Bind...(targetCount, pRTV);
	__debugbreak();
}

void CDeviceComputeCommandList::SetPipelineStateImpl(const CDeviceComputePSOConstPtr& devicePSO)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	const CDeviceComputePSO_DX12* pDevicePSO = reinterpret_cast<const CDeviceComputePSO_DX12*>(devicePSO.get());

	pCommandListDX12->SetPSO(pDevicePSO->GetComputePSO());
}

void CDeviceComputeCommandList::SetResourceLayoutImpl(const CDeviceResourceLayoutConstPtr& resourceLayout)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	const CDeviceResourceLayout_DX12* pResourceLayoutDX12 = reinterpret_cast<const CDeviceResourceLayout_DX12*>(resourceLayout.get());

	pCommandListDX12->SetComputeRootSignature(pResourceLayoutDX12->GetRootSignature());
}

void CDeviceComputeCommandList::SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResourceSet)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);

	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	const CDeviceResourceSet_DX12* pResources = reinterpret_cast<const CDeviceResourceSet_DX12*>(pResourceSet);

	for (auto& it : pResources->m_ConstantBuffers)
	{
		if (CConstantBuffer* pBuffer = it.second.resource)
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer->m_buffer);

			assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
			CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundConstBuffers[pResource->GetDX12Resource().IsOffCard()]);
	#endif

			// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
			pCommandListDX12->TrackResourceCBVUsage(pResource->GetDX12Resource());
		}
	}

	for (auto& it : pResources->m_Textures)
	{
		if (CTexture* pTexture = std::get<1>(it.second.resource))
		{
			if (CDeviceTexture* pDevTexture = pTexture->GetDevTexture())
			{
				auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pDevTexture->GetBaseTexture());
				auto pView = reinterpret_cast<CCryDX12ShaderResourceView*>(std::get<0>(it.second.resource));
				assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, pView->GetDX12View(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	#if defined(ENABLE_PROFILING_CODE)
				CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformTextures[pResource->GetDX12Resource().IsOffCard()]);
	#endif

				// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
				pCommandListDX12->TrackResourceCRVUsage(pResource->GetDX12Resource());
			}
		}
	}

	for (auto& it : pResources->m_Buffers)
	{
		if (ID3D11Buffer* pBuffer = it.second.resource.GetBuffer())
		{
			auto pResource = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pBuffer);

			assert(!pResource->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	#if defined(ENABLE_PROFILING_CODE)
			CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundUniformBuffers[pResource->GetDX12Resource().IsOffCard()]);
	#endif

			// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
			pCommandListDX12->TrackResourceCRVUsage(pResource->GetDX12Resource());
		}
	}

	const CDescriptorBlock& descriptorBlock = pResources->GetDescriptorBlock();
	pCommandListDX12->SetComputeDescriptorTable(bindSlot, descriptorBlock.GetHandleOffsetGPU(0));
}

void CDeviceComputeCommandList::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	SetInlineConstantBuffer(bindSlot, pBuffer, shaderSlot, eHWSC_Num);
}

void CDeviceComputeCommandList::SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pConstantBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();

	auto pBuffer = reinterpret_cast<CCryDX12Buffer*>(pConstantBuffer->m_buffer);

	assert(!pBuffer->GetDX12Resource().NeedsTransitionBarrier(pCommandListDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	#if defined(ENABLE_PROFILING_CODE)
	CryInterlockedIncrement(&gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nNumBoundInlineBuffers[pBuffer->GetDX12Resource().IsOffCard()]);
	#endif

	// TODO: if we know early that the resource(s) will be GENERIC_READ we can begin the barrier early and end it here
	pCommandListDX12->TrackResourceCBVUsage(pBuffer->GetDX12Resource());

	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetDX12View().GetCBVDesc().BufferLocation + pConstantBuffer->m_offset;
	pCommandListDX12->SetComputeConstantBufferView(bindSlot, gpuAddress);
}

void CDeviceComputeCommandList::SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants)
{
	CRY_ASSERT(bindSlot <= EResourceLayoutSlot_Max);
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->SetCompute32BitConstants(bindSlot, constantCount, pConstants, 0);
}

void CDeviceComputeCommandList::DispatchImpl(uint32 X, uint32 Y, uint32 Z)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->Dispatch(X, Y, Z);
}

void CDeviceComputeCommandList::ClearUAV(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRect)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();

	auto pViewDX12 = reinterpret_cast<CCryDX12UnorderedAccessView*>(pView);

	pCommandListDX12->ClearUnorderedAccessView(pViewDX12->GetDX12View(), Values, NumRects, pRect);
}

void CDeviceComputeCommandList::ClearUAV(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRect)
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();

	auto pViewDX12 = reinterpret_cast<CCryDX12UnorderedAccessView*>(pView);

	pCommandListDX12->ClearUnorderedAccessView(pViewDX12->GetDX12View(), Values, NumRects, pRect);
}

void CDeviceComputeCommandList::LockToThread()
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->Begin();
	pCommandListDX12->SetResourceAndSamplerStateHeaps();
}

void CDeviceComputeCommandList::Build()
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COMPUTE_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->End();
}

void CDeviceComputeCommandList::ResetImpl()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDeviceCopyCommandList_DX12 : public CDeviceCopyCommandList
{
public:
	DX12_PTR(NCryDX12::CCommandList) m_pCommandList;
};

	#define GET_DX12_COPY_COMMANDLIST(abstractCommandList) reinterpret_cast<CDeviceCopyCommandList_DX12*>(abstractCommandList)

CDeviceCopyCommandList::ECopyType CDeviceCopyCommandList::DetermineCopyType(CDeviceCopyCommandList::ECopyType eCurrent, D3DResource* pResource)
{
	auto pRes = reinterpret_cast<CCryDX12Resource<ID3D11Resource>*>(pResource);
	NCryDX12::CResource& rResource = pRes->GetDX12Resource();

	if (rResource.IsBackBuffer())
		return eCT_GraphicsResources;
	if (rResource.IsOffCard())
		return eCT_OffCardResources;

	if (rResource.IsTarget())
		return eCT_GraphicsResources;
	if (rResource.IsGeneric()) // could be non-compute shader too
		return eCT_GenericResources;

	if (eCurrent == eCT_OffCardResources)
		return eCT_GraphicsResources;

	return eCurrent;
}

void CDeviceCopyCommandList::LockToThread()
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COPY_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->Begin();
	pCommandListDX12->SetResourceAndSamplerStateHeaps();
}

void CDeviceCopyCommandList::Build()
{
	NCryDX12::CCommandList* pCommandListDX12 = GET_DX12_COPY_COMMANDLIST(this)->m_pCommandList.get();
	pCommandListDX12->End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceObjectFactory::CDeviceObjectFactory()
{
	m_pCoreCommandList = std::make_shared<CDeviceGraphicsCommandList_DX12>(nullptr);
}

CDeviceGraphicsPSOPtr CDeviceObjectFactory::CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceGraphicsPSO_DX12>(GetDevice());
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSOImpl(const CDeviceComputePSODesc& psoDesc) const
{
	auto pResult = std::make_shared<CDeviceComputePSO_DX12>(GetDevice());
	pResult->Init(psoDesc);

	return pResult;
}

CDeviceResourceSetPtr CDeviceObjectFactory::CreateResourceSet(CDeviceResourceSet::EFlags flags) const
{
	return std::make_shared<CDeviceResourceSet_DX12>(GetDevice(), flags);
}

CDeviceResourceSetPtr CDeviceObjectFactory::CloneResourceSet(const CDeviceResourceSetPtr pSrcResourceSet) const
{
	return std::make_shared<CDeviceResourceSet_DX12>(*pSrcResourceSet);
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayoutImpl(const SDeviceResourceLayoutDesc& resourceLayoutDesc) const
{
	auto pResult = std::make_shared<CDeviceResourceLayout_DX12>(GetDevice(), resourceLayoutDesc.GetRequiredResourceBindings());
	if (pResult->Init(resourceLayoutDesc))
		return pResult;

	return nullptr;
}

CDeviceGraphicsCommandListPtr CDeviceObjectFactory::GetCoreGraphicsCommandList() const
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(m_pCoreCommandList.get());
	pCommandListDX12->m_pCommandList = pContext->GetCoreGraphicsCommandList();

	return m_pCoreCommandList;
}

// Acquire one or more command-lists which are independent of the core command-list
// Only one thread is allowed to call functions on this command-list (DX12 restriction).
// The thread that gets the permition is the one calling Begin() on it AFAIS
CDeviceGraphicsCommandListUPtr CDeviceObjectFactory::AcquireGraphicsCommandList()
{
	// In theory this whole function need to be atomic, instead it voids the core command-list(s),
	// Synchronization between different threads acquiring command-lists is deferred to the higher level.
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);
	pContext->CeaseAllCommandQueues(false);

	DX12_PTR(CCommandList) pCL;
	pQueue.AcquireCommandList(pCL);

	pContext->ResumeAllCommandQueues();
	return CryMakeUnique<CDeviceGraphicsCommandList_DX12>(pCL);
}

std::vector<CDeviceGraphicsCommandListUPtr> CDeviceObjectFactory::AcquireGraphicsCommandLists(uint32 listCount)
{
	// In theory this whole function need to be atomic, instead it voids the core command-list(s),
	// Synchronization between different threads acquiring command-lists is deferred to the higher level.
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);
	pContext->CeaseAllCommandQueues(false);

	std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists;
	DX12_PTR(CCommandList) pCLs[256];

	// Allocate in chunks of 256
	for (uint32 n = 0; n < listCount; n += 256U)
	{
		const uint32 chunkCount = std::min(listCount - n, 256U);
		pQueue.AcquireCommandLists(chunkCount, pCLs);

		for (uint32 b = 0; b < chunkCount; ++b)
			pCommandLists.emplace_back(CryMakeUnique<CDeviceGraphicsCommandList_DX12>(pCLs[b]));
	}

	pContext->ResumeAllCommandQueues();
	return std::move(pCommandLists);
}

// Command-list sinks, will automatically submit command-lists in allocation-order
void CDeviceObjectFactory::ForfeitGraphicsCommandList(CDeviceGraphicsCommandListUPtr pCommandList)
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);

	CDeviceGraphicsCommandList_DX12* pCommandListDX12 = GET_DX12_GRAPHICS_COMMANDLIST(pCommandList.get());
	DX12_PTR(CCommandList) pCL = pCommandListDX12->m_pCommandList;
	pQueue.ForfeitCommandList(pCL);
}

void CDeviceObjectFactory::ForfeitGraphicsCommandLists(std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists)
{
	auto pContext = reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext());
	NCryDX12::CCommandListPool& pQueue = pContext->GetCoreCommandListPool(CMDQUEUE_GRAPHICS);

	const uint32 listCount = pCommandLists.size();
	DX12_PTR(CCommandList) pCLs[256];

	// Deallocate in chunks of 256
	for (uint32 n = 0; n < listCount; n += 256U)
	{
		const uint32 chunkCount = std::min(listCount - n, 256U);
		for (uint32 b = 0; b < chunkCount; ++b)
			pCLs[b] = GET_DX12_GRAPHICS_COMMANDLIST(pCommandLists[b].get())->m_pCommandList;
		pQueue.ForfeitCommandLists(chunkCount, pCLs);
	}
}

void CDeviceObjectFactory::ReleaseResourcesImpl()
{
	auto pDevice = reinterpret_cast<CCryDX12Device*>(gcpRendD3D->GetDevice().GetRealDevice());
	pDevice->FlushAndWaitForGPU();
}

#endif
