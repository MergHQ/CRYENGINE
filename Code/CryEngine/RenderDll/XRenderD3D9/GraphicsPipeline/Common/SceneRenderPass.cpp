// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneRenderPass.h"

#include "DriverD3D.h"

#include "Common/RenderView.h"
#include "Common/ReverseDepth.h"
#include "CompiledRenderObject.h"
#include "GraphicsPipelineStage.h"

CSceneRenderPass::CSceneRenderPass()
	: m_passFlags(ePassFlags_None)
{
	m_pDepthTarget = nullptr;
	m_pResourceLayout = nullptr;
	m_pPerPassResources = nullptr;
	m_bNearestViewport = false;
	m_szLabel = "";

	for (uint32 i = 0; i < CRY_ARRAY_COUNT(m_pColorTargets); ++i)
		m_pColorTargets[i] = nullptr;
}

void CSceneRenderPass::SetupPassContext(uint32 stageID, uint32 stagePassID, EShaderTechniqueID technique, uint32 filter)
{
	assert(stageID < MAX_PIPELINE_SCENE_STAGES);
	m_stageID = stageID;
	m_passID = stagePassID;
	m_technique = technique;
	m_batchFilter = filter;
}

void CSceneRenderPass::SetPassResources(CDeviceResourceLayoutPtr pResourceLayout, CDeviceResourceSetPtr pPerPassResources)
{
	m_pResourceLayout = pResourceLayout;
	m_pPerPassResources = pPerPassResources;
}

void CSceneRenderPass::SetRenderTargets(SDepthTexture* pDepthTarget, CTexture* pColorTarget0, CTexture* pColorTarget1, CTexture* pColorTarget2, CTexture* pColorTarget3)
{
	m_pDepthTarget = pDepthTarget;
	m_pColorTargets[0] = pColorTarget0;
	m_pColorTargets[1] = pColorTarget1;
	m_pColorTargets[2] = pColorTarget2;
	m_pColorTargets[3] = pColorTarget3;
}

void CSceneRenderPass::SetViewport(const D3DViewPort& viewport)
{
	m_viewPort[0] =
	  m_viewPort[1] = viewport;

	if (m_passFlags & CSceneRenderPass::ePassFlags_RenderNearest)
	{
		m_viewPort[1].MinDepth = 0;
		m_viewPort[1].MaxDepth = CRenderer::CV_r_DrawNearZRange;
		if (m_passFlags & CSceneRenderPass::ePassFlags_ReverseDepth)
			m_viewPort[1] = ReverseDepthHelper::Convert(m_viewPort[1]);
	}

	D3DRectangle scissorRect = {
		LONG(m_viewPort[0].TopLeftX),
		LONG(m_viewPort[0].TopLeftY),
		LONG(m_viewPort[0].TopLeftX + m_viewPort[0].Width),
		LONG(m_viewPort[0].TopLeftY + m_viewPort[0].Height)
	};

	m_scissorRect = scissorRect;
}

void CSceneRenderPass::EnableNearestViewport(bool bNearest)
{
	m_bNearestViewport = bNearest && (m_passFlags & CSceneRenderPass::ePassFlags_RenderNearest);
}

void CSceneRenderPass::ExchangeRenderTarget(uint32 slot, CTexture* pNewColorTarget)
{
	assert(slot >= 0 && slot < CRY_ARRAY_COUNT(m_pColorTargets));

	// Only allow exchanging RT when the formats match, otherwise compiled PSOs might become invalid
	if (pNewColorTarget && m_pColorTargets[slot] && pNewColorTarget->GetTextureDstFormat() == m_pColorTargets[slot]->GetTextureDstFormat())
	{
		m_pColorTargets[slot] = pNewColorTarget;
	}
	else
	{
		assert(0);
	}
}

void CSceneRenderPass::ExtractRenderTargetFormats(CDeviceGraphicsPSODesc& psoDesc)
{
	assert(m_pDepthTarget || m_pColorTargets[0]);

	if (m_pDepthTarget)
	{
		D3D11_TEXTURE2D_DESC depthTargetDesc;
		m_pDepthTarget->pTarget->GetDesc(&depthTargetDesc);
		psoDesc.m_DepthStencilFormat = CTexture::TexFormatFromDeviceFormat(depthTargetDesc.Format);
	}

	for (uint32 i = 0; i < CRY_ARRAY_COUNT(m_pColorTargets); ++i)
	{
		if (m_pColorTargets[i])
		{
			assert(m_pColorTargets[i]->GetTextureDstFormat() != eTF_Unknown);
			psoDesc.m_RenderTargetFormats[i] = m_pColorTargets[i]->GetTextureDstFormat();
		}
	}
}

// Forward declaration
void UpdateNearestState(const CSceneRenderPass& pass, CDeviceGraphicsCommandListRef commandList, bool bNearestRenderingRequired, bool& bRenderNearestState);

void CSceneRenderPass::DrawRenderItems_GP2(SGraphicsPipelinePassContext& passContext)
{
	PROFILE_FRAME(GBuffer_ProcessBatchesList);

	int listStart = passContext.rendItems.start;
	int listEnd = passContext.rendItems.end;

	if (listEnd - listStart == 0)
		return;
	if (CRenderer::CV_r_NoDraw == 2) // Completely skip filling of the command list.
		return;

	CD3D9Renderer* rd = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

	CDeviceGraphicsCommandListPtr pCommandList = CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

	PrepareRenderPassForUse(*pCommandList);
	BeginRenderPass(*pCommandList);

	auto& renderItems = passContext.pRenderView->GetRenderItems(rRP.m_nPassGroupID);
	const uint32 drawParamsIndex = (passContext.pRenderView->GetType() == CRenderView::eViewType_Shadow) ? 1 : 0;

	CShader* pShader = NULL;
	CShaderResources* pRes = NULL;
	CShaderResources* pPrevRes = NULL;
	CRenderObject* pPrevObject = NULL;
	int nTech;

	CCompiledRenderObject compiledObject;

	for (int i = listStart; i < listEnd; i++)
	{
		SRendItem& ri = renderItems[i];
		if (!(ri.nBatchFlags & passContext.batchFilter))
			continue;

		CRenderObject* pObject = ri.pObj;
		CRendElementBase* pRE = ri.pElem;

		SRendItem::mfGet(ri.SortVal, nTech, pShader, pRes);

		// Update initialized or outdated resources
		if (pRes->m_pDeformInfo)
			pRes->RT_UpdateConstants(pShader);
		if (!pRes->m_pCompiledResourceSet)
			continue;
		if (pRes->m_pCompiledResourceSet->IsDirty())
			pRes->m_pCompiledResourceSet->Build();
		if (!pRes->m_pCompiledResourceSet->IsValid())
			continue;

		SShaderItem shaderItem;
		shaderItem.m_nTechnique = nTech;
		shaderItem.m_pShader = pShader;
		shaderItem.m_pShaderResources = pRes;

		compiledObject.Init(shaderItem, pRE);

		pObject->m_bInstanceDataDirty = false;  // Enforce recompilation of entire object
		if (compiledObject.Compile(pObject, rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime))
		{
			if (!compiledObject.DrawVerification(passContext))
				continue;

			compiledObject.DrawToCommandList(*pCommandList, compiledObject.m_pso[passContext.stageID][passContext.passID], drawParamsIndex);
		}
	}

	EndRenderPass(*pCommandList);
}

void CSceneRenderPass::PrepareRenderPassForUse(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList)
{
	uint32 targetCount;
	for (targetCount = 0; targetCount < CRY_ARRAY_COUNT(m_pColorTargets); ++targetCount)
	{
		if (!m_pColorTargets[targetCount])
			break;
	}

	commandList.PrepareRenderTargetsForUse(targetCount, m_pColorTargets, m_pDepthTarget);
	commandList.PrepareResourcesForUse(EResourceLayoutSlot_PerPassRS, m_pPerPassResources.get());
}

void CSceneRenderPass::BeginRenderPass(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList)
{
	// Note: Function has to be threadsafe since it can be called from several worker threads
	// Todo: Enable merging of different label of the same group, eg. ZPREPASS[0] + ZPREPASS[1] => ZPREPASS
	// if (m_szLabel) { PROFILE_LABEL_PUSH(m_szLabel); }

	uint32 targetCount;
	for (targetCount = 0; targetCount < CRY_ARRAY_COUNT(m_pColorTargets); ++targetCount)
	{
		if (!m_pColorTargets[targetCount])
			break;
	}

	commandList.Reset();
	commandList.SetRenderTargets(targetCount, m_pColorTargets, m_pDepthTarget);
	commandList.SetViewports(1, &GetViewport(m_bNearestViewport));
	commandList.SetScissorRects(1, &m_scissorRect);
	commandList.SetResourceLayout(m_pResourceLayout);
	commandList.SetResources(EResourceLayoutSlot_PerPassRS, m_pPerPassResources.get());
}

void CSceneRenderPass::EndRenderPass(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList)
{
	// Note: Function has to be threadsafe since it can be called from several worker threads
	// Todo: Enable merging of different label of the same group, eg. ZPREPASS[0] + ZPREPASS[1] => ZPREPASS
	// if (m_szLabel) { PROFILE_LABEL_POP(m_szLabel); }

	// Nothing to cleanup at the moment
}

void CSceneRenderPass::DrawRenderItems(CRenderView* pRenderView, ERenderListID list, int listStart, int listEnd, int profilingListID)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& rp = pRenderer->m_RP;

	uint32 nBatchFlags = pRenderView->GetBatchFlags(list);

	if (m_batchFilter != FB_MASK && !(nBatchFlags & m_batchFilter))
		return;

	SGraphicsPipelinePassContext passContext(pRenderView, this, m_technique, m_batchFilter);

	passContext.nProcessThreadID = rp.m_nProcessThreadID;
	passContext.nFrameID = rp.m_TI[rp.m_nProcessThreadID].m_nFrameID;
	passContext.pPipelineStats = &rp.m_PS[passContext.nProcessThreadID];
	passContext.stageID = m_stageID;
	passContext.passID = m_passID;

	passContext.renderNearest = m_bNearestViewport;
	passContext.renderListId = list;
	passContext.rendItems.start = listStart < 0 ? 0 : listStart;
	passContext.rendItems.end = listEnd < 0 ? pRenderView->GetRenderItems(list).size() : listEnd;

	rp.m_nPassGroupID = profilingListID < 0 ? list : profilingListID;
	rp.m_nPassGroupDIP = profilingListID < 0 ? list : profilingListID;

	CHWShader_D3D::mfCommitParamsGlobal();

	if (pRenderer->m_nGraphicsPipeline >= 2)
	{
		gcpRendD3D->DrawRenderItems(passContext);
	}
	else
	{
		DrawRenderItems_GP2(passContext);
	}
}
