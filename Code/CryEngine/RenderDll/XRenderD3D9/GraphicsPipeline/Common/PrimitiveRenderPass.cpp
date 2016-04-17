// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/CryCustomTypes.h>
#include "DriverD3D.h"
#include "../Common/PostProcess/PostProcessUtils.h"
#include <CryRenderer/RenderElements/RendElement.h>

CCompiledRenderPrimitive::SPrimitiveGeometry CCompiledRenderPrimitive::s_primitiveGeometryCache[CCompiledRenderPrimitive::ePrim_Count];
int CCompiledRenderPrimitive::s_nPrimitiveGeometryCacheUsers = 0;

CCompiledRenderPrimitive::SPrimitiveGeometry::SPrimitiveGeometry()
	: vertexFormat(eVF_Unknown)
	, vertexOrIndexCount(0)
{
	SStreamInfo emptyStream = { ~0u, 0 };
	vertexStream = emptyStream;
	indexStream = emptyStream;
}

CCompiledRenderPrimitive::CCompiledRenderPrimitive()
	: m_dirtyMask(eDirty_All)
	, m_renderState(0)
	, m_cullMode(eCULL_Back)
	, m_pShader(nullptr)
	, m_rtMask(0)
	, m_primitiveType(ePrim_Triangle)
	, m_currentPsoUpdateCount(0)
	, m_pVertexInputSet(nullptr)
	, m_pIndexInputSet(nullptr)
{
	m_pResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
}

#define ASSIGN_VALUE(dst, src, dirtyFlag) \
  if (!((dst) == (src)))                  \
  {                                       \
    m_dirtyMask |= (dirtyFlag);           \
    (dst) = (src);                        \
  }

void CCompiledRenderPrimitive::SetRenderState(int state)
{
	ASSIGN_VALUE(m_renderState, state, eDirty_RenderState);
}

void CCompiledRenderPrimitive::SetCullMode(ECull cullMode)
{
	ASSIGN_VALUE(m_cullMode, cullMode, eDirty_RenderState);
}

void CCompiledRenderPrimitive::SetTechnique(CShader* pShader, CCryNameTSCRC& techName, uint64 rtMask)
{
	ASSIGN_VALUE(m_pShader, pShader, eDirty_Technique);
	ASSIGN_VALUE(m_techniqueName, techName, eDirty_Technique);
	ASSIGN_VALUE(m_rtMask, rtMask, eDirty_Technique);

	if (m_pPipelineState && m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount())
		m_dirtyMask |= eDirty_Technique;
}

void CCompiledRenderPrimitive::SetTexture(uint32 shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID)
{
	m_pResources->SetTexture(shaderSlot, pTexture, resourceViewID);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

void CCompiledRenderPrimitive::SetSampler(uint32 shaderSlot, int32 sampler)
{
	m_pResources->SetSampler(shaderSlot, sampler);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

void CCompiledRenderPrimitive::SetInlineConstantBuffers(std::vector<InlineConstantBuffer>&& inlineConstantBuffers)
{
	m_inlineConstantBuffers = std::move(inlineConstantBuffers);
	m_dirtyMask |= eDirty_Resources;
}

void CCompiledRenderPrimitive::SetPrimitiveType(EPrimitiveType primitiveType)
{
	ASSIGN_VALUE(m_primitiveType, primitiveType, eDirty_Geometry);
}

void CCompiledRenderPrimitive::SetCustomVertexStream(buffer_handle_t vertexBuffer, EVertexFormat vertexFormat, uint32 vertexStride)
{
	ASSIGN_VALUE(m_primitiveGeometry.vertexStream.hStream, vertexBuffer, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.vertexStream.nStride, vertexStride, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.vertexFormat, vertexFormat, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveType, ePrim_Custom, eDirty_Geometry);
}
void CCompiledRenderPrimitive::SetCustomIndexStream(buffer_handle_t indexBuffer, uint32 indexStride)
{
	ASSIGN_VALUE(m_primitiveGeometry.indexStream.hStream, indexBuffer, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.indexStream.nStride, indexStride, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveType, ePrim_Custom, eDirty_Geometry);
}

void CCompiledRenderPrimitive::SetDrawInfo(uint32 vertexOrIndexCount)
{
	ASSIGN_VALUE(m_primitiveGeometry.vertexOrIndexCount, vertexOrIndexCount, eDirty_None);
}

#undef ASSIGN_VALUE

bool CCompiledRenderPrimitive::IsDirty() const
{
	if (m_dirtyMask != eDirty_None)
		return true;

	if (m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount())
		return true;

	if (m_pResources && m_pResources->IsDirty())
		return true;

	return false;
}

CCompiledRenderPrimitive::EDirtyFlags CCompiledRenderPrimitive::Compile(CDeviceGraphicsPSODesc partialPsoDesc)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (m_dirtyMask & eDirty_Geometry)
	{
		if (m_primitiveType != ePrim_Custom)
		{
			m_primitiveGeometry = s_primitiveGeometryCache[m_primitiveType];
		}

		m_pIndexInputSet = CCryDeviceWrapper::GetObjectFactory().CreateIndexStreamSet(&m_primitiveGeometry.indexStream);
		m_pVertexInputSet = CCryDeviceWrapper::GetObjectFactory().CreateVertexStreamSet(1, &m_primitiveGeometry.vertexStream);

		m_dirtyMask &= ~eDirty_Geometry;
	}

	if (m_dirtyMask & eDirty_Resources)
	{
		m_pResources->Build();

		if (!m_pResources->IsValid())
			return m_dirtyMask;
	}

	if (m_dirtyMask & (eDirty_Technique | eDirty_Resources))
	{
		int bindSlot = 0;
		SDeviceResourceLayoutDesc resourceLayoutDesc;

		for (auto& cb : m_inlineConstantBuffers)
		{
			resourceLayoutDesc.SetConstantBuffer(bindSlot++, cb.shaderSlot, SHADERSTAGE_FROM_SHADERCLASS(cb.shaderClass));
		}
		resourceLayoutDesc.SetConstantBuffer(bindSlot++, eConstantBufferShaderSlot_PerView, EShaderStage_Vertex | EShaderStage_Pixel);
		resourceLayoutDesc.SetResourceSet(bindSlot++, m_pResources);

		m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(resourceLayoutDesc);

		if (!m_pResourceLayout)
			return m_dirtyMask;
	}

	if (m_dirtyMask & (eDirty_Technique | eDirty_RenderState | eDirty_Resources))
		partialPsoDesc.m_pResourceLayout = m_pResourceLayout.get();
	partialPsoDesc.m_pShader = m_pShader;
	partialPsoDesc.m_technique = m_techniqueName;
	partialPsoDesc.m_ShaderFlags_RT = m_rtMask;
	partialPsoDesc.m_ShaderFlags_MD = 0;
	partialPsoDesc.m_ShaderFlags_MDV = 0;
	partialPsoDesc.m_PrimitiveType = eptTriangleList;
	partialPsoDesc.m_VertexFormat = m_primitiveGeometry.vertexFormat;
	partialPsoDesc.m_RenderState = m_renderState;
	partialPsoDesc.m_CullMode = m_cullMode;
	m_pPipelineState = CCryDeviceWrapper::GetObjectFactory().CreateGraphicsPSO(partialPsoDesc);

	if (!m_pPipelineState || !m_pPipelineState->IsValid())
		return m_dirtyMask;

	m_currentPsoUpdateCount = m_pPipelineState->GetUpdateCount();

	m_dirtyMask = eDirty_None;
	return m_dirtyMask;
}

void CCompiledRenderPrimitive::AddPrimitiveGeometryCacheUser()
{
	if (s_nPrimitiveGeometryCacheUsers == 0)
	{
		CD3D9Renderer* const __restrict rd = gcpRendD3D;

		SPrimitiveGeometry& primitiveGeometry = s_primitiveGeometryCache[ePrim_Triangle];

		SVF_P3F_C4B_T2F fullscreenTriVertices[3];
		SPostEffectsUtils::GetFullScreenTri(fullscreenTriVertices, 0, 0, 1.0f);
		primitiveGeometry.vertexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, 3 * sizeof(SVF_P3F_C4B_T2F));
		primitiveGeometry.vertexStream.nStride = sizeof(SVF_P3F_C4B_T2F);
		primitiveGeometry.vertexFormat = eVF_P3F_C4B_T2F;
		primitiveGeometry.vertexOrIndexCount = 3;
		gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.vertexStream.hStream, fullscreenTriVertices, sizeof(fullscreenTriVertices));

		{
			SPrimitiveGeometry& primitiveGeometry = s_primitiveGeometryCache[ePrim_Box];

			t_arrDeferredMeshVertBuff boxVertices;
			t_arrDeferredMeshIndBuff boxIndices;
			CDeferredRenderUtils::CreateUnitBox(boxIndices, boxVertices);
			primitiveGeometry.vertexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, 8 * sizeof(SVF_P3F_C4B_T2F));
			primitiveGeometry.vertexStream.nStride = sizeof(SVF_P3F_C4B_T2F);
			primitiveGeometry.indexStream.hStream = gcpRendD3D->m_DevBufMan.Create(BBT_INDEX_BUFFER, BU_DYNAMIC, 36 * sizeof(uint16));
			primitiveGeometry.indexStream.nStride = sizeof(uint16);
			primitiveGeometry.vertexFormat = eVF_P3F_C4B_T2F;
			primitiveGeometry.vertexOrIndexCount = boxIndices.size();
			gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.vertexStream.hStream, &boxVertices[0], sizeof(boxVertices));
			gcpRendD3D->m_DevBufMan.UpdateBuffer(primitiveGeometry.indexStream.hStream, &boxIndices[0], sizeof(boxIndices));

		}
	}

	++s_nPrimitiveGeometryCacheUsers;
}

void CCompiledRenderPrimitive::RemovePrimitiveGeometryCacheUser()
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
	: m_primitiveCount(0)
	, m_bDirty(true)
{
	CCompiledRenderPrimitive::AddPrimitiveGeometryCacheUser();
}

CPrimitiveRenderPass::~CPrimitiveRenderPass()
{
	CCompiledRenderPrimitive::RemovePrimitiveGeometryCacheUser();
}

void CPrimitiveRenderPass::SetRenderTarget(uint32 slot, CTexture* pRenderTarget, SResourceView::KeyType rendertargetView)
{
	CRY_ASSERT(slot < m_pRenderTargets.size());
	if (m_pRenderTargets[slot] != pRenderTarget || m_renderTargetViews[slot] != rendertargetView)
	{
		m_pRenderTargets[slot] = pRenderTarget;
		m_renderTargetViews[slot] = rendertargetView;

		m_bDirty = true;
	}
}

void CPrimitiveRenderPass::SetDepthTarget(SDepthTexture* pDepthTarget)
{
	if (m_pDepthTarget != pDepthTarget)
	{
		m_pDepthTarget = pDepthTarget;
		m_bDirty = true;
	}
}

void CPrimitiveRenderPass::SetViewport(const D3DViewPort& viewport)
{
	if (m_viewport.Height != viewport.Height ||
	    m_viewport.Width != viewport.Width ||
	    m_viewport.TopLeftX != viewport.TopLeftX ||
	    m_viewport.TopLeftY != viewport.TopLeftY ||
	    m_viewport.MinDepth != viewport.MinDepth ||
	    m_viewport.MaxDepth != viewport.MaxDepth)
	{
		m_viewport = viewport;
		m_bDirty = true;
	}
}

bool CPrimitiveRenderPass::CompileResources()
{
	// Prefill pso desc with rendertarget info
	CDeviceGraphicsPSODesc psoDesc(m_pRenderTargets.size(), &m_pRenderTargets[0], m_pDepthTarget);
	bool bDirty = false;

	for (int i = 0; i < m_primitiveCount; ++i)
	{
		if (m_primitives[i].IsDirty())
		{
			auto primitiveDirtyMask = m_primitives[i].Compile(psoDesc);
			bDirty |= primitiveDirtyMask != CCompiledRenderPrimitive::eDirty_None;
		}
	}

	m_numRenderTargets = 0;
	while (m_numRenderTargets < m_pRenderTargets.size() && m_pRenderTargets[m_numRenderTargets])
		++m_numRenderTargets;

	return bDirty;
}

void CPrimitiveRenderPass::AddPrimitive(const CCompiledRenderPrimitive& primitive)
{
	if (m_primitiveCount >= m_primitives.size())
		m_primitives.push_back(CCompiledRenderPrimitive());

	m_primitives[m_primitiveCount++] = primitive;
	m_bDirty |= primitive.IsDirty();
}

void CPrimitiveRenderPass::Execute()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	CDeviceGraphicsCommandListPtr pCommandList = CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

	if (IsDirty())
	{
		m_bDirty = CompileResources();
	}

	D3D11_RECT viewportRect =
	{
		LONG(m_viewport.TopLeftX),
		LONG(m_viewport.TopLeftY),
		LONG(m_viewport.TopLeftX + m_viewport.Width),
		LONG(m_viewport.TopLeftY + m_viewport.Height)
	};

	for (int i = 0; i < m_primitiveCount; ++i)
	{
		auto& curPrimitive = m_primitives[i];

		if (!curPrimitive.IsDirty())
		{
			// Prepare resources
			{
				uint32 bindSlot = 0;
				pCommandList->PrepareRenderTargetsForUse(m_numRenderTargets, &m_pRenderTargets[0], m_pDepthTarget, &m_renderTargetViews[0]);
				for (auto& cb : curPrimitive.m_inlineConstantBuffers)
				{
					pCommandList->PrepareInlineConstantBufferForUse(bindSlot++, cb.pBuffer, cb.shaderSlot, cb.shaderClass);
				}
				pCommandList->PrepareInlineConstantBufferForUse(bindSlot++, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), eConstantBufferShaderSlot_PerView, (EShaderStage)(EShaderStage_Vertex | EShaderStage_Pixel));
				pCommandList->PrepareResourcesForUse(bindSlot++, curPrimitive.m_pResources.get());
				pCommandList->PrepareVertexBuffersForUse(1, curPrimitive.m_pVertexInputSet);

				if (curPrimitive.m_pIndexInputSet)
				{
					pCommandList->PrepareIndexBufferForUse(curPrimitive.m_pIndexInputSet);
				}
			}

			uint32 bindSlot = 0;
			pCommandList->SetRenderTargets(m_numRenderTargets, &m_pRenderTargets[0], m_pDepthTarget, &m_renderTargetViews[0]);
			pCommandList->SetViewports(1, &m_viewport);
			pCommandList->SetScissorRects(1, &viewportRect);
			pCommandList->SetResourceLayout(curPrimitive.m_pResourceLayout);
			pCommandList->SetPipelineState(curPrimitive.m_pPipelineState);
			for (auto& cb : curPrimitive.m_inlineConstantBuffers)
			{
				pCommandList->SetInlineConstantBuffer(bindSlot++, cb.pBuffer, cb.shaderSlot, cb.shaderClass);
			}
			pCommandList->SetInlineConstantBuffer(bindSlot++, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), eConstantBufferShaderSlot_PerView, EShaderStage_Vertex | EShaderStage_Pixel);
			pCommandList->SetResources(bindSlot++, curPrimitive.m_pResources.get());
			pCommandList->SetVertexBuffers(1, curPrimitive.m_pVertexInputSet);

			if (curPrimitive.m_pIndexInputSet)
			{
				pCommandList->SetIndexBuffer(curPrimitive.m_pIndexInputSet);
				pCommandList->DrawIndexed(curPrimitive.m_primitiveGeometry.vertexOrIndexCount, 1, 0, 0, 0);
			}
			else
			{
				pCommandList->Draw(curPrimitive.m_primitiveGeometry.vertexOrIndexCount, 1, 0, 0);
			}
		}
	}
}
