// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneRenderPass.h"

#include "DriverD3D.h"

#include "Common/RenderView.h"
#include "Common/ReverseDepth.h"
#include "CompiledRenderObject.h"
#include "GraphicsPipelineStage.h"

int CSceneRenderPass::s_recursionCounter = 0;

CSceneRenderPass::CSceneRenderPass()
	: m_passFlags(ePassFlags_None)
	, m_depthConstBias(0.0f)
	, m_depthSlopeBias(0.0f)
	, m_depthBiasClamp(0.0f)
{
	m_pDepthTarget = nullptr;
	m_pResourceLayout = nullptr;
	m_pPerPassResources = nullptr;
	m_szLabel = "";
	m_numRenderItemGroups = 0;
	m_profilerSectionIndex = ~0u;

	for (uint32 i = 0; i < CRY_ARRAY_COUNT(m_pColorTargets); ++i)
		m_pColorTargets[i] = nullptr;
}

void CSceneRenderPass::SetupPassContext(uint32 stageID, uint32 stagePassID, EShaderTechniqueID technique, uint32 filter, ERenderListID renderList, uint32 excludeFilter, bool drawCompiledRenderObject)
{
	// the scene render passes which draw CCompiledRenderObject must follow the strict rule of PSOs array and PSO cache in CCompiledRenderObject
	const bool drawable = (drawCompiledRenderObject && stageID < MAX_PIPELINE_SCENE_STAGES) || !drawCompiledRenderObject;
	assert(drawable);
	m_stageID = stageID;
	m_passID = stagePassID;
	m_technique = technique;
	m_batchFilter = drawable ? filter : 0;
	m_excludeFilter = excludeFilter;
	m_renderList = renderList;
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

void CSceneRenderPass::SetDepthBias(float constBias, float slopeBias, float biasClamp)
{ 
	m_depthConstBias = constBias; 
	m_depthSlopeBias = slopeBias; 
	m_depthBiasClamp = biasClamp; 
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
void UpdateNearestState(const CSceneRenderPass& pass, CDeviceCommandListRef commandList, bool bNearestRenderingRequired, bool& bRenderNearestState);

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

	CDeviceCommandListPtr pCommandList = CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
	passContext.pCommandList = pCommandList.get();

	PrepareRenderPassForUse(*pCommandList);
	BeginRenderPass(*pCommandList, passContext.renderNearest, passContext.profilerSectionIndex, true);

	auto& renderItems = passContext.pRenderView->GetRenderItems(rRP.m_nPassGroupID);

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

		if (ri.nBatchFlags & passContext.batchExcludeFilter)
			continue;

		CRenderObject* pObject = ri.pObj;
		CRenderElement* pRE = ri.pElem;

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
		if (compiledObject.Compile(pObject))
		{
			compiledObject.DrawToCommandList(passContext);
		}
	}

	EndRenderPass(*pCommandList, passContext.renderNearest, passContext.profilerSectionIndex, true);
}

void CSceneRenderPass::PrepareRenderPassForUse(CDeviceCommandListRef RESTRICT_REFERENCE commandList)
{
	uint32 targetCount;
	for (targetCount = 0; targetCount < CRY_ARRAY_COUNT(m_pColorTargets); ++targetCount)
	{
		if (!m_pColorTargets[targetCount])
			break;
	}
	CDeviceGraphicsCommandInterface* pCommandInterface = commandList.GetGraphicsInterface();
	pCommandInterface->PrepareRenderTargetsForUse(targetCount, m_pColorTargets, m_pDepthTarget);
	pCommandInterface->PrepareResourcesForUse(EResourceLayoutSlot_PerPassRS, m_pPerPassResources.get(), EShaderStage_AllWithoutCompute);

	if (m_passFlags & ePassFlags_VrProjectionPass)
	{
		if (CVrProjectionManager::IsMultiResEnabledStatic())
		{
			// we don't know the bNearest flag here, so just prepare for both cases
			CVrProjectionManager::Instance()->PrepareProjectionParameters(commandList, GetViewport(false));
			CVrProjectionManager::Instance()->PrepareProjectionParameters(commandList, GetViewport(true));
		}
	}
}

void CSceneRenderPass::BeginRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList, bool bNearest, uint32 profilerSectionIndex, bool bIssueGPUTimestamp) const
{
	// Note: Function has to be threadsafe since it can be called from several worker threads

	if (gcpRendD3D->m_pPipelineProfiler)
		gcpRendD3D->m_pPipelineProfiler->UpdateMultithreadedSection(profilerSectionIndex, true, 0, 0, bIssueGPUTimestamp, &commandList);

#if defined(ENABLE_PROFILING_CODE)
	commandList.BeginProfilingSection();
#endif

	uint32 targetCount;
	for (targetCount = 0; targetCount < CRY_ARRAY_COUNT(m_pColorTargets); ++targetCount)
	{
		if (!m_pColorTargets[targetCount])
			break;
	}

	D3D11_VIEWPORT viewport = GetViewport(bNearest);
	bool bViewportSet = false;

	commandList.Reset();

	CDeviceGraphicsCommandInterface* pCommandInterface = commandList.GetGraphicsInterface();
	pCommandInterface->BeginProfilerEvent(m_szLabel);
	pCommandInterface->SetRenderTargets(targetCount, m_pColorTargets, m_pDepthTarget);

	if (m_passFlags & ePassFlags_VrProjectionPass)
	{
		bViewportSet = CVrProjectionManager::Instance()->SetRenderingState(commandList, viewport,
			(m_passFlags & ePassFlags_UseVrProjectionState) != 0, (m_passFlags & ePassFlags_RequireVrProjectionConstants) != 0);
	}
	
	if (!bViewportSet)
	{
		pCommandInterface->SetViewports(1, &viewport);
		pCommandInterface->SetScissorRects(1, &m_scissorRect);
	}

	pCommandInterface->SetResourceLayout(m_pResourceLayout.get());
	pCommandInterface->SetResources(EResourceLayoutSlot_PerPassRS, m_pPerPassResources.get(), EShaderStage_AllWithoutCompute);

#if !defined(CRY_USE_DX12)
	pCommandInterface->SetDepthBias(m_depthConstBias, m_depthSlopeBias, m_depthBiasClamp);
#endif
}

void CSceneRenderPass::EndRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList, bool bNearest, uint32 profilerSectionIndex, bool bIssueGPUTimestamp) const
{
	// Note: Function has to be threadsafe since it can be called from several worker threads

	CDeviceGraphicsCommandInterface* pCommandInterface = commandList.GetGraphicsInterface();
	pCommandInterface->EndProfilerEvent(m_szLabel);

#if !defined(CRY_USE_DX12)
	pCommandInterface->SetDepthBias(0.0f, 0.0f, 0.0f);
#endif

#if defined(ENABLE_PROFILING_CODE)
	if (gcpRendD3D->m_pPipelineProfiler)
	{
		gcpRendD3D->m_pPipelineProfiler->UpdateMultithreadedSection(profilerSectionIndex, false, commandList.EndProfilingSection().numDIPs,
		                                                            commandList.EndProfilingSection().numPolygons, bIssueGPUTimestamp, &commandList);
	}
	
	gcpRendD3D->AddRecordedProfilingStats(commandList.EndProfilingSection(), m_renderList, true);
#endif

	if (m_passFlags & ePassFlags_UseVrProjectionState)
	{
		CDeviceGraphicsCommandInterface* pCommandInterface = commandList.GetGraphicsInterface();
		CVrProjectionManager::Instance()->RestoreState(commandList);
	}
}

void CSceneRenderPass::BeginExecution()
{
	assert(s_recursionCounter == 0);
	s_recursionCounter += 1;
	
	m_numRenderItemGroups = 0;
	
	if (gcpRendD3D->m_pPipelineProfiler)
		m_profilerSectionIndex = gcpRendD3D->m_pPipelineProfiler->InsertMultithreadedSection(m_szLabel);
}

void CSceneRenderPass::EndExecution()
{
	s_recursionCounter -= 1;
}


void CSceneRenderPass::DrawRenderItems(CRenderView* pRenderView, ERenderListID list, int listStart, int listEnd, int profilingListID)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& rp = pRenderer->m_RP;

	assert(s_recursionCounter == 1);
	
	uint32 nBatchFlags = pRenderView->GetBatchFlags(list);

	if (m_batchFilter != FB_MASK && !(nBatchFlags & m_batchFilter))
		return;

	SGraphicsPipelinePassContext passContext(pRenderView, this, m_technique, m_batchFilter, m_excludeFilter);

	passContext.rendItems.start = listStart < 0 ? 0 : listStart;
	passContext.rendItems.end = listEnd < 0 ? pRenderView->GetRenderItems(list).size() : listEnd;

	if (passContext.rendItems.IsEmpty())
		return;

	passContext.stageID = m_stageID;
	passContext.passID = m_passID;

	passContext.renderNearest = (list == EFSLIST_NEAREST_OBJECTS) && (m_passFlags & CSceneRenderPass::ePassFlags_RenderNearest);
	passContext.renderListId = list;
	passContext.renderItemGroup = m_numRenderItemGroups++;
	passContext.profilerSectionIndex = m_profilerSectionIndex;

#if !defined(_RELEASE)
	if (CRenderer::CV_r_stats == 6)
	{
		passContext.pDrawCallInfoPerMesh = gcpRendD3D->GetGraphicsPipeline().GetDrawCallInfoPerMesh();
		passContext.pDrawCallInfoPerNode = gcpRendD3D->GetGraphicsPipeline().GetDrawCallInfoPerNode();
	}
#endif

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
