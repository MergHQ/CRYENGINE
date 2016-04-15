#include "StdAfx.h"
#include "FullscreenPass.h"
#include "DriverD3D.h"
#include "../Common/PostProcess/PostProcessUtils.h"

CFullscreenPass::CFullscreenPass()
{
	m_inputVars[0] = m_inputVars[1] = m_inputVars[2] = m_inputVars[3] = 0;

	m_bRequirePerViewCB = false;
	m_bRequireWorldPos = false;
	m_bValidConstantBuffers = true;
	m_bPendingConstantUpdate = false;

	m_vertexBuffer = ~0u;

	m_primitives.push_back(CCompiledRenderPrimitive());
	m_primitiveCount = m_primitives.size();
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
	auto& renderPrimitive = GetPrimitive();

	m_prevRTMask = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT = renderPrimitive.GetShaderRtMask();

	auto pShader = renderPrimitive.GetShader();
	auto shaderTechnique = renderPrimitive.GetShaderTechnique();
	auto shaderRtMask = renderPrimitive.GetShaderRtMask();

	if (renderPrimitive.IsDirty())
	{
		m_bValidConstantBuffers = false;
		std::vector<SDeviceObjectHelpers::SConstantBufferBindInfo> inlineConstantBuffers;

		if (SDeviceObjectHelpers::GetConstantBuffersFromShader(inlineConstantBuffers, pShader, shaderTechnique, shaderRtMask, 0, 0))
		{
			renderPrimitive.SetInlineConstantBuffers(std::move(inlineConstantBuffers));
			m_bValidConstantBuffers = true;
		}
	}

	if (m_bValidConstantBuffers)
	{
		uint32 numPasses;
		pShader->FXSetTechnique(shaderTechnique);
		pShader->FXBegin(&numPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		m_bPendingConstantUpdate = true;
		SDeviceObjectHelpers::BeginUpdateConstantBuffers(renderPrimitive.GetInlineConstantBuffers());
	}
}

void CFullscreenPass::Execute()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	auto& renderPrimitive = GetPrimitive();

	m_bDirty |= renderPrimitive.IsDirty();

	if (!m_bValidConstantBuffers)
		return;

	// update viewport
	{
		D3DViewPort viewport;
		viewport.TopLeftX = viewport.TopLeftY = 0;
		viewport.Width = float(m_pRenderTargets[0] ? m_pRenderTargets[0]->GetWidth() : m_pDepthTarget->nWidth);
		viewport.Height = float(m_pRenderTargets[0] ? m_pRenderTargets[0]->GetHeight() : m_pDepthTarget->nHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		if (m_pRenderTargets[0] && m_renderTargetViews[0] != SResourceView::DefaultRendertargtView)
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
		SDeviceObjectHelpers::EndUpdateConstantBuffers(renderPrimitive.GetInlineConstantBuffers());

		renderPrimitive.GetShader()->FXEndPass();
		renderPrimitive.GetShader()->FXEnd();
		rd->m_RP.m_FlagsShader_RT = m_prevRTMask;

		m_bPendingConstantUpdate = false;
	}

	if (m_bRequirePerViewCB)
	{
		D3D11_RECT viewportRect =
		{
			LONG(m_viewport.TopLeftX),
			LONG(m_viewport.TopLeftY),
			LONG(m_viewport.TopLeftX + m_viewport.Width),
			LONG(m_viewport.TopLeftY + m_viewport.Height)
		};

		CHWShader_D3D::mfSetPV(&viewportRect);
	}

	if (m_bRequireWorldPos)
	{
		if (m_vertexBuffer == ~0u)
		{
			m_vertexBuffer = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, 3 * sizeof(SVF_P3F_T2F_T3F));
		}

		SVF_P3F_T2F_T3F fullscreenTriWPOSVertices[3];
		SPostEffectsUtils::GetFullScreenTriWPOS(fullscreenTriWPOSVertices, 0, 0);
		rd->m_DevBufMan.UpdateBuffer(m_vertexBuffer, fullscreenTriWPOSVertices, 3 * sizeof(SVF_P3F_T2F_T3F));

		renderPrimitive.SetCustomVertexStream(m_vertexBuffer, eVF_P3F_T2F_T3F, sizeof(SVF_P3F_T2F_T3F));
		renderPrimitive.SetCustomIndexStream(~0u, 0);
		renderPrimitive.SetDrawInfo(3);
	}

	CPrimitiveRenderPass::Execute();
}
