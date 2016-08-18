// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/CryCustomTypes.h>
#include "DriverD3D.h"
#include "../Common/PostProcess/PostProcessUtils.h"
#include <CryRenderer/RenderElements/RendElement.h>

CRenderPrimitive::SPrimitiveGeometry CRenderPrimitive::s_primitiveGeometryCache[CRenderPrimitive::ePrim_Count];
int CRenderPrimitive::s_nPrimitiveGeometryCacheUsers = 0;


SCompiledRenderPrimitive::SCompiledRenderPrimitive() 
	: m_stencilRef(0)
	, m_pVertexInputSet(nullptr)
	, m_pIndexInputSet(nullptr)
{
}

SCompiledRenderPrimitive::SCompiledRenderPrimitive(SCompiledRenderPrimitive&& other)
	: m_stencilRef(std::move(other.m_stencilRef))
	, m_pVertexInputSet(std::move(other.m_pVertexInputSet))
	, m_pIndexInputSet(std::move(other.m_pIndexInputSet))
	, m_pPipelineState(std::move(other.m_pPipelineState))
	, m_pResourceLayout(std::move(other.m_pResourceLayout))
	, m_pResources(std::move(other.m_pResources))
	, m_instances(std::move(other.m_instances))
{
}

void SCompiledRenderPrimitive::Reset()
{
	m_stencilRef = 0;
	m_pVertexInputSet = nullptr;
	m_pIndexInputSet = nullptr;

	m_pResources.reset();
	m_instances.clear();

	m_pPipelineState.reset();
	m_pResourceLayout.reset();
}

CRenderPrimitive::SPrimitiveGeometry::SPrimitiveGeometry()
	: primType(eptTriangleList)
	, vertexFormat(eVF_Unknown)
	, vertexBaseOffset(0)
	, vertexOrIndexCount(0)
	, vertexOrIndexOffset(0)
{
	SStreamInfo emptyStream = { ~0u, 0, 0 };
	vertexStream = emptyStream;
	indexStream = emptyStream;
}

CRenderPrimitive::CRenderPrimitive(CRenderPrimitive&& other)
	: SCompiledRenderPrimitive(std::move(other))
	, m_flags(std::move(other.m_flags))
	, m_dirtyMask(std::move(other.m_dirtyMask))
	, m_renderState(std::move(other.m_renderState))
	, m_stencilState(std::move(other.m_stencilState))
	, m_stencilReadMask(std::move(other.m_stencilReadMask))
	, m_stencilWriteMask(std::move(other.m_stencilWriteMask))
	, m_cullMode(std::move(other.m_cullMode))
	, m_pShader(std::move(other.m_pShader))
	, m_techniqueName(std::move(other.m_techniqueName))
	, m_rtMask(std::move(other.m_rtMask))
	, m_primitiveType(std::move(other.m_primitiveType))
	, m_primitiveGeometry(std::move(other.m_primitiveGeometry))
	, m_constantManager(std::move(other.m_constantManager))
	, m_currentPsoUpdateCount(std::move(other.m_currentPsoUpdateCount))
{
}

CRenderPrimitive::CRenderPrimitive(EPrimitiveFlags flags)
	: m_flags(flags)
	, m_dirtyMask(eDirty_All)
	, m_renderState(0)
	, m_stencilState(STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP))
	, m_stencilReadMask(0xFF)
	, m_stencilWriteMask(0xFF)
	, m_cullMode(eCULL_Back)
	, m_pShader(nullptr)
	, m_rtMask(0)
	, m_primitiveType(ePrim_Triangle)
	, m_currentPsoUpdateCount(0)
{
	m_pResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	m_instances.resize(1);
}

void CRenderPrimitive::Reset(EPrimitiveFlags flags)
{
	// Allocate before free, so that the re-allocated memory-pointer doesn't become the same/identical
	CDeviceResourceSetPtr pResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);

	SCompiledRenderPrimitive::Reset();

	m_flags = flags;
	m_dirtyMask = eDirty_All;
	m_renderState = 0;
	m_stencilState = STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
	m_stencilReadMask = 0xFF;
	m_stencilWriteMask = 0xFF;
	m_cullMode = eCULL_Back;
	m_pShader = nullptr;
	m_rtMask = 0;
	m_primitiveType = ePrim_Triangle;
	m_currentPsoUpdateCount = 0;

	m_pResources = std::move(pResources);
	m_instances.resize(1);

	m_constantManager.Reset();
}

void CRenderPrimitive::AllocateTypedConstantBuffer(EConstantBufferShaderSlot shaderSlot, int size, EShaderStage shaderStages)
{
	SetInlineConstantBuffer(shaderSlot, gcpRendD3D->m_DevBufMan.CreateConstantBuffer(size), shaderStages);
}

bool CRenderPrimitive::IsDirty() const
{
	if (m_dirtyMask != eDirty_None)
		return true;

	if (m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount())
		return true;

	if (m_pResources && m_pResources->IsDirty())
		return true;

	return false;
}

CRenderPrimitive::EDirtyFlags CRenderPrimitive::Compile(uint32 renderTargetCount, CTexture* const* pRenderTargets, const SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews, CDeviceResourceSetPtr pOutputResources)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	auto& instance = m_instances.front();
	
	if (m_dirtyMask & eDirty_Geometry)
	{
		m_dirtyMask |= eDirty_InstanceData;

		if (m_primitiveType != ePrim_Custom)
		{
			m_primitiveGeometry = s_primitiveGeometryCache[m_primitiveType];
		}

		m_pIndexInputSet = CCryDeviceWrapper::GetObjectFactory().CreateIndexStreamSet(&m_primitiveGeometry.indexStream);
		m_pVertexInputSet = CCryDeviceWrapper::GetObjectFactory().CreateVertexStreamSet(1, &m_primitiveGeometry.vertexStream);

		m_dirtyMask &= ~eDirty_Geometry;
	}

	if (m_dirtyMask & eDirty_InstanceData)
	{
		instance.vertexBaseOffset = m_primitiveGeometry.vertexBaseOffset;
		instance.vertexOrIndexCount = m_primitiveGeometry.vertexOrIndexCount;
		instance.vertexOrIndexOffset = m_primitiveGeometry.vertexOrIndexOffset;
	}

	if (m_dirtyMask & eDirty_Resources)
	{
		m_pResources->Build();

		if (!m_pResources->IsValid())
			return m_dirtyMask;

		if (m_pResources->IsLayoutDirty())
			m_dirtyMask |= eDirty_ResourceLayout;
	}

	if (m_dirtyMask & (eDirty_Technique | eDirty_ResourceLayout))
	{
		int bindSlot = 0;
		SDeviceResourceLayoutDesc resourceLayoutDesc;

		// check for valid shader reflection first
		if (m_flags & eFlags_ReflectConstantBuffersFromShader)
		{
			if (!m_constantManager.IsShaderReflectionValid())
				return m_dirtyMask;
		}

		if (pOutputResources)
		{
			resourceLayoutDesc.SetResourceSet(bindSlot++, pOutputResources);
		}

		instance.constantBuffers = m_constantManager.GetBuffers();

		for (auto& cb : instance.constantBuffers)
			resourceLayoutDesc.SetConstantBuffer(bindSlot++, cb.shaderSlot, cb.shaderStages);

		if (!m_pResources->IsEmpty())
		{
			resourceLayoutDesc.SetResourceSet(bindSlot++, m_pResources);
		}

		m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(resourceLayoutDesc);

		if (!m_pResourceLayout)
			return m_dirtyMask;
	}

	if (m_dirtyMask & (eDirty_Technique | eDirty_RenderState | eDirty_ResourceLayout))
	{
		CDeviceGraphicsPSODesc psoDesc(renderTargetCount, pRenderTargets, pDepthTarget, pRenderTargetViews);
		psoDesc.m_pResourceLayout = m_pResourceLayout.get();
		psoDesc.m_pShader = m_pShader;
		psoDesc.m_technique = m_techniqueName;
		psoDesc.m_ShaderFlags_RT = m_rtMask;
		psoDesc.m_ShaderFlags_MD = 0;
		psoDesc.m_ShaderFlags_MDV = 0;
		psoDesc.m_PrimitiveType = m_primitiveGeometry.primType;
		psoDesc.m_VertexFormat = m_primitiveGeometry.vertexFormat;
		psoDesc.m_RenderState = m_renderState;
		psoDesc.m_StencilState = m_stencilState;
		psoDesc.m_StencilReadMask = m_stencilReadMask;
		psoDesc.m_StencilWriteMask = m_stencilWriteMask;
		psoDesc.m_CullMode = m_cullMode;
		m_pPipelineState = CCryDeviceWrapper::GetObjectFactory().CreateGraphicsPSO(psoDesc);

		if (!m_pPipelineState || !m_pPipelineState->IsValid())
			return m_dirtyMask;

		m_currentPsoUpdateCount = m_pPipelineState->GetUpdateCount();
	}

	m_dirtyMask = eDirty_None;
	return m_dirtyMask;
}

void CRenderPrimitive::AddPrimitiveGeometryCacheUser()
{
	if (s_nPrimitiveGeometryCacheUsers == 0)
	{
		CD3D9Renderer* const __restrict rd = gcpRendD3D;

		// ePrim_Triangle
		{
			SPrimitiveGeometry& primitiveGeometry = s_primitiveGeometryCache[ePrim_Triangle];

			// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
			CryStackAllocWithSizeVector(SVF_P3F_C4B_T2F, 3, fullscreenTriVertices, CDeviceBufferManager::AlignBufferSizeForStreaming);
			
			SPostEffectsUtils::GetFullScreenTri(fullscreenTriVertices, 0, 0, 1.0f);
			primitiveGeometry.vertexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, 3 * sizeof(SVF_P3F_C4B_T2F));
			primitiveGeometry.vertexStream.nStride = sizeof(SVF_P3F_C4B_T2F);
			primitiveGeometry.primType = eptTriangleList;
			primitiveGeometry.vertexFormat = eVF_P3F_C4B_T2F;
			primitiveGeometry.vertexOrIndexOffset = 0;
			primitiveGeometry.vertexOrIndexCount = 3;

			gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.vertexStream.hStream, fullscreenTriVertices, fullscreenTriVerticesSize);
		}

		t_arrDeferredMeshVertBuff vertices;
		t_arrDeferredMeshIndBuff indices;

		auto initPrimitiveGeometryStreams = [](SPrimitiveGeometry& primitiveGeometry, t_arrDeferredMeshVertBuff boxVertices, t_arrDeferredMeshIndBuff boxIndices)
		{
			primitiveGeometry.vertexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, boxVertices.size() * sizeof(boxVertices[0]));
			primitiveGeometry.vertexStream.nStride = sizeof(boxVertices[0]);
			primitiveGeometry.indexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_INDEX_BUFFER, BU_STATIC, boxIndices.size() * sizeof(boxIndices[0]));
			primitiveGeometry.indexStream.nStride = Index16;
			primitiveGeometry.primType = eptTriangleList;
			primitiveGeometry.vertexFormat = eVF_P3F_C4B_T2F;
			primitiveGeometry.vertexOrIndexOffset = 0;
			primitiveGeometry.vertexOrIndexCount = boxIndices.size();

			gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.vertexStream.hStream, boxVertices);
			gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.indexStream.hStream, boxIndices);
		};

		// Box Primitives
		{
			CDeferredRenderUtils::CreateUnitBox(indices, vertices);
			initPrimitiveGeometryStreams(s_primitiveGeometryCache[ePrim_UnitBox], vertices, indices);

			CDeferredRenderUtils::CreateSimpleLightFrustumMesh(indices, vertices);
			initPrimitiveGeometryStreams(s_primitiveGeometryCache[ePrim_CenteredBox], vertices, indices);
		}

		// Projector Primitives
		{
			const uint nProjectorMeshStep = 10;

			for (int i = ePrim_Projector; i <= ePrim_Projector2; ++i)
			{
				const uint nFrustTess = 11 + nProjectorMeshStep * (i - ePrim_Projector);

				CDeferredRenderUtils::CreateUnitFrustumMesh(nFrustTess, nFrustTess, indices, vertices);
				initPrimitiveGeometryStreams(s_primitiveGeometryCache[i], vertices, indices);
			}

			for (int i = ePrim_ClipProjector; i <= ePrim_ClipProjector2; ++i)
			{
				uint nFrustTess = 41 + nProjectorMeshStep * (i - ePrim_ClipProjector);

				CDeferredRenderUtils::CreateUnitFrustumMesh(nFrustTess, nFrustTess, indices, vertices);
				initPrimitiveGeometryStreams(s_primitiveGeometryCache[i], vertices, indices);
			}
		}
	}

	++s_nPrimitiveGeometryCacheUsers;
}

void CRenderPrimitive::RemovePrimitiveGeometryCacheUser()
{
	if (--s_nPrimitiveGeometryCacheUsers == 0)
	{
		for (EPrimitiveType primType = ePrim_First; primType != ePrim_Count; primType = EPrimitiveType(primType + 1))
		{
			if (s_primitiveGeometryCache[primType].vertexStream.hStream != ~0u) gRenDev->m_DevBufMan.Destroy(s_primitiveGeometryCache[primType].vertexStream.hStream);
			if (s_primitiveGeometryCache[primType].indexStream.hStream != ~0u) gRenDev->m_DevBufMan.Destroy(s_primitiveGeometryCache[primType].indexStream.hStream);

			s_primitiveGeometryCache[primType] = SPrimitiveGeometry();
		}
	}
}

CPrimitiveRenderPass::CPrimitiveRenderPass()
	: m_numRenderTargets(0)
	, m_pDepthTarget(nullptr)
	, m_scissorEnabled(false)
{
	m_pRenderTargets.fill(nullptr);
	m_renderTargetViews.fill(SResourceView::KeyType(SResourceView::DefaultRendertargetView));

	ZeroStruct(m_viewport);
	ZeroStruct(m_scissor);

	CRenderPrimitive::AddPrimitiveGeometryCacheUser();
}

CPrimitiveRenderPass::~CPrimitiveRenderPass()
{
	CRenderPrimitive::RemovePrimitiveGeometryCacheUser();
}

CPrimitiveRenderPass::CPrimitiveRenderPass(CPrimitiveRenderPass&& other)
	: m_numRenderTargets(std::move(other.m_numRenderTargets))
	, m_pDepthTarget(std::move(other.m_pDepthTarget))
	, m_scissorEnabled(std::move(other.m_scissorEnabled))
{
	m_pRenderTargets = std::move(other.m_pRenderTargets);
	m_renderTargetViews = std::move(other.m_renderTargetViews);

	m_viewport = std::move(other.m_viewport);
	m_scissor = std::move(other.m_scissor);

	m_compiledPrimitives = std::move(other.m_compiledPrimitives);
}

CPrimitiveRenderPass& CPrimitiveRenderPass::operator=(CPrimitiveRenderPass&& other)
{
	m_numRenderTargets = std::move(other.m_numRenderTargets);
	m_pDepthTarget = std::move(other.m_pDepthTarget);
	m_scissorEnabled = std::move(other.m_scissorEnabled);

	m_pRenderTargets = std::move(other.m_pRenderTargets);
	m_renderTargetViews = std::move(other.m_renderTargetViews);

	m_viewport = std::move(other.m_viewport);
	m_scissor = std::move(other.m_scissor);

	m_compiledPrimitives = std::move(other.m_compiledPrimitives);

	return *this;
}

void CPrimitiveRenderPass::SetRenderTarget(uint32 slot, CTexture* pRenderTarget, SResourceView::KeyType rendertargetView)
{
	CRY_ASSERT(slot < m_pRenderTargets.size());

	m_pRenderTargets[slot] = pRenderTarget;
	m_renderTargetViews[slot] = rendertargetView;
}

void CPrimitiveRenderPass::SetOutputUAV(uint32 slot, CGpuBuffer* pBuffer)
{
	if (!m_pOutputResources)
	{
		m_pOutputResources     = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
		m_pOutputNULLResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	}

	CGpuBuffer nullBuffer;
	nullBuffer.Create(0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DX11BUF_NULL_RESOURCE | DX11BUF_BIND_UAV, nullptr);

	m_pOutputResources->SetBuffer(slot, pBuffer ? *pBuffer : CGpuBuffer(), true);
	m_pOutputNULLResources->SetBuffer(slot, nullBuffer, true);
}

void CPrimitiveRenderPass::SetDepthTarget(SDepthTexture* pDepthTarget)
{
	m_pDepthTarget = pDepthTarget;
}

void CPrimitiveRenderPass::SetViewport(const D3DViewPort& viewport)
{
	if (!m_scissorEnabled)
	{
		m_scissor.left   = LONG(viewport.TopLeftX);
		m_scissor.top    = LONG(viewport.TopLeftY);
		m_scissor.right  = LONG(viewport.TopLeftX + viewport.Width);
		m_scissor.bottom = LONG(viewport.TopLeftY + viewport.Height);
	}

	m_viewport = viewport;
}

void CPrimitiveRenderPass::SetScissor(bool bEnable, const D3DRectangle& scissor)
{
	if (m_scissorEnabled = bEnable)
	{
		m_scissor = scissor;
	}
	else
	{
		m_scissor.left   = LONG(m_viewport.TopLeftX);
		m_scissor.top    = LONG(m_viewport.TopLeftY);
		m_scissor.right  = LONG(m_viewport.TopLeftX + m_viewport.Width);
		m_scissor.bottom = LONG(m_viewport.TopLeftY + m_viewport.Height);
	}
}

bool CPrimitiveRenderPass::AddPrimitive(CRenderPrimitive* pPrimitive)
{
	if (!pPrimitive->IsDirty() || 
	     pPrimitive->Compile(m_pRenderTargets.size(), &m_pRenderTargets[0], m_pDepthTarget, nullptr, m_pOutputResources) == CRenderPrimitive::eDirty_None)
	{
		m_compiledPrimitives.push_back(pPrimitive);
		return true;
	}

	return false;
}

bool CPrimitiveRenderPass::AddPrimitive(SCompiledRenderPrimitive* pPrimitive)
{
	CRY_ASSERT(pPrimitive->m_pPipelineState->IsValid());
	CRY_ASSERT(pPrimitive->m_pResourceLayout);
	CRY_ASSERT(!pPrimitive->m_pResources || pPrimitive->m_pResources->IsValid());

	m_compiledPrimitives.push_back(pPrimitive);
	return true;
}

void CPrimitiveRenderPass::Prepare(CDeviceGraphicsCommandInterface* pCommandInterface)
{
	m_numRenderTargets = 0;
	while (m_numRenderTargets < m_pRenderTargets.size() && m_pRenderTargets[m_numRenderTargets])
		++m_numRenderTargets;

	pCommandInterface->PrepareRenderTargetsForUse(m_numRenderTargets, &m_pRenderTargets[0], m_pDepthTarget, &m_renderTargetViews[0]);

	uint32 bindSlot = 0;

	if (m_pOutputResources)
	{
		if (m_pOutputResources->IsDirty())
		{
			m_pOutputResources->Build();
			m_pOutputNULLResources->Build();
		}

		if (m_pOutputResources->IsValid())
			pCommandInterface->PrepareResourcesForUse(bindSlot++, m_pOutputResources.get(), EShaderStage_AllWithoutCompute);
	}

	for (auto pPrimitive : m_compiledPrimitives)
	{
		CRY_ASSERT(pPrimitive->m_pPipelineState->IsValid());
		CRY_ASSERT(pPrimitive->m_pResourceLayout);


		for (auto& instance : pPrimitive->m_instances)
		{
			uint32 cbBindSlot = bindSlot;
			for (auto& cb : instance.constantBuffers)
			{
				pCommandInterface->PrepareInlineConstantBufferForUse(cbBindSlot++, cb.pBuffer, cb.shaderSlot, cb.shaderStages);
			}
		}

		if (pPrimitive->m_pResources && !pPrimitive->m_pResources->IsEmpty())
		{
			CRY_ASSERT(pPrimitive->m_pResources->IsValid());
			uint32 resourcesBindSlot = bindSlot + pPrimitive->m_instances.front().constantBuffers.size();
			pCommandInterface->PrepareResourcesForUse(resourcesBindSlot, pPrimitive->m_pResources.get(), EShaderStage_AllWithoutCompute);
		}

		if (pPrimitive->m_pVertexInputSet)
		{
			pCommandInterface->PrepareVertexBuffersForUse(1, 0, pPrimitive->m_pVertexInputSet);
		}

		if (pPrimitive->m_pIndexInputSet)
		{
			pCommandInterface->PrepareIndexBufferForUse(pPrimitive->m_pIndexInputSet);
		}
	}
}

void CPrimitiveRenderPass::Execute()
{
	if (m_compiledPrimitives.empty())
		return;

	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	CDeviceCommandListPtr pCommandList = CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
	CDeviceGraphicsCommandInterface* pCommandInterface = pCommandList->GetGraphicsInterface();

#ifdef ENABLE_PROFILING_CODE
	pCommandList->BeginProfilingSection();
#endif

	// prepare primitives first
	Prepare(pCommandInterface);

	pCommandInterface->SetRenderTargets(m_numRenderTargets, &m_pRenderTargets[0], m_pDepthTarget, &m_renderTargetViews[0]);
	pCommandInterface->SetViewports(1, &m_viewport);
	pCommandInterface->SetScissorRects(1, &m_scissor);
	
	for (auto pPrimitive : m_compiledPrimitives)
	{
		uint32 bindSlot = 0;
		pCommandInterface->SetResourceLayout(pPrimitive->m_pResourceLayout.get());
		pCommandInterface->SetPipelineState(pPrimitive->m_pPipelineState.get());
		pCommandInterface->SetStencilRef(pPrimitive->m_stencilRef);
	
		if (pPrimitive->m_pVertexInputSet)
		{
			pCommandInterface->SetVertexBuffers(1, 0, pPrimitive->m_pVertexInputSet);
		}

		if (pPrimitive->m_pIndexInputSet)
		{
			pCommandInterface->SetIndexBuffer(pPrimitive->m_pIndexInputSet);
		}

		if (m_pOutputResources && m_pOutputResources->IsValid())
		{
			pCommandInterface->SetResources(bindSlot++, m_pOutputResources.get(), EShaderStage_AllWithoutCompute);
		}

		if (pPrimitive->m_pResources && !pPrimitive->m_pResources->IsEmpty())
		{
			uint32 resourcesBindSlot = bindSlot + pPrimitive->m_instances.front().constantBuffers.size();
			pCommandInterface->SetResources( resourcesBindSlot, pPrimitive->m_pResources.get(), EShaderStage_AllWithoutCompute);
		}

		for (auto& instance : pPrimitive->m_instances)
		{
			uint32 cbBindSlot = bindSlot;
			for (auto& cb : instance.constantBuffers)
			{
				pCommandInterface->SetInlineConstantBuffer(cbBindSlot++, cb.pBuffer, cb.shaderSlot, cb.shaderStages);
			}

			if (pPrimitive->m_pIndexInputSet)
			{
				pCommandInterface->DrawIndexed(instance.vertexOrIndexCount, 1, instance.vertexOrIndexOffset, instance.vertexBaseOffset, 0);
			}
			else
			{
				pCommandInterface->Draw(instance.vertexOrIndexCount, 1, instance.vertexOrIndexOffset, 0);
			}
		}
	}

	// unbind output resources
	if (m_pOutputNULLResources)
	{
		pCommandInterface->SetResources(0, m_pOutputNULLResources.get(), EShaderStage_AllWithoutCompute);
	}

#ifdef ENABLE_PROFILING_CODE
	rd->AddRecordedProfilingStats(pCommandList->EndProfilingSection(), EFSLIST_GENERAL);
#endif
}
