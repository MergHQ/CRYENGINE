#include "StdAfx.h"
#include "FullscreenPass.h"
#include "DriverD3D.h"
#include "../Common/PostProcess/PostProcessUtils.h"

CFullscreenPass::CFullscreenPass()
	: m_primitive(CRenderPrimitive::eFlags_ReflectConstantBuffersFromShader)
	, m_prevRTMask(0)
{
	m_inputVars[0] = m_inputVars[1] = m_inputVars[2] = m_inputVars[3] = 0;

	m_bRequirePerViewCB = false;
	m_bRequireWorldPos = false;
	m_bPendingConstantUpdate = false;

	m_vertexBuffer = ~0u;

	m_clipZ = 0.0f;
}

CFullscreenPass::~CFullscreenPass()
{
	if (m_vertexBuffer != ~0u)
	{
		gRenDev->m_DevBufMan.Destroy(m_vertexBuffer);
	}
}

void CFullscreenPass::BeginConstantUpdate()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	m_prevRTMask = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT = m_primitive.GetShaderRtMask();

	m_primitive.GetConstantManager().BeginNamedConstantUpdate();
	m_bPendingConstantUpdate = true;
}

void CFullscreenPass::Execute()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	// update viewport
	{
		D3DViewPort viewport;
		viewport.TopLeftX = viewport.TopLeftY = 0;
		viewport.Width = float(m_pRenderTargets[0] ? m_pRenderTargets[0]->GetWidth() : m_pDepthTarget->nWidth);
		viewport.Height = float(m_pRenderTargets[0] ? m_pRenderTargets[0]->GetHeight() : m_pDepthTarget->nHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		if (m_pRenderTargets[0] && m_renderTargetViews[0] != SResourceView::DefaultRendertargetView)
		{
			auto firstRtv = SResourceView(m_renderTargetViews[0]).m_Desc;
			viewport.Width = float(int(viewport.Width) >> firstRtv.nMostDetailedMip);
			viewport.Height = float(int(viewport.Height) >> firstRtv.nMostDetailedMip);
		}

		SetViewport(viewport);
	}

	if (m_bPendingConstantUpdate)
	{
		// Engine viewport needs to be set so that data is available when filling reflected PB constants
		rd->RT_SetViewport((int)m_viewport.TopLeftX, (int)m_viewport.TopLeftY, (int)m_viewport.Width, (int)m_viewport.Height);

		// Unmap constant buffers and mark as bound
		m_primitive.GetConstantManager().EndNamedConstantUpdate();

		rd->m_RP.m_FlagsShader_RT = m_prevRTMask;
		m_bPendingConstantUpdate = false;
	}

	if (m_bRequirePerViewCB)
	{
		rd->GetGraphicsPipeline().UpdatePerViewConstantBuffer(&m_viewport);
		m_primitive.SetInlineConstantBuffer(eConstantBufferShaderSlot_PerView, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), EShaderStage_Vertex | EShaderStage_Pixel);
	}

	if (m_bRequireWorldPos)
	{
		if (m_vertexBuffer == ~0u)
		{
			m_vertexBuffer = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, 3 * sizeof(SVF_P3F_T2F_T3F));
		}

		bool bUseQuad = CVrProjectionManager::Instance()->GetProjectionType() == CVrProjectionManager::eVrProjection_LensMatched;
		
		if (bUseQuad)
		{
			SVF_P3F_T2F_T3F fullscreenQuadWPOSVertices[4];
			SPostEffectsUtils::GetFullScreenQuadWPOS(fullscreenQuadWPOSVertices, 0, 0, m_clipZ);
			rd->m_DevBufMan.UpdateBuffer(m_vertexBuffer, fullscreenQuadWPOSVertices, 4 * sizeof(SVF_P3F_T2F_T3F));
		}
		else
		{
			SVF_P3F_T2F_T3F fullscreenTriWPOSVertices[3];
			SPostEffectsUtils::GetFullScreenTriWPOS(fullscreenTriWPOSVertices, 0, 0, m_clipZ);
			rd->m_DevBufMan.UpdateBuffer(m_vertexBuffer, fullscreenTriWPOSVertices, 3 * sizeof(SVF_P3F_T2F_T3F));
		}

		m_primitive.SetCustomVertexStream(m_vertexBuffer, eVF_P3F_T2F_T3F, sizeof(SVF_P3F_T2F_T3F));
		m_primitive.SetCustomIndexStream(~0u, (RenderIndexType)0);

		if(bUseQuad)
			m_primitive.SetDrawInfo(eptTriangleStrip, 0, 0, 4);
		else
			m_primitive.SetDrawInfo(eptTriangleList, 0, 0, 3);
	}

	ClearPrimitives();

	if (AddPrimitive(&m_primitive))
	{
		CPrimitiveRenderPass::Execute();
	}
}
