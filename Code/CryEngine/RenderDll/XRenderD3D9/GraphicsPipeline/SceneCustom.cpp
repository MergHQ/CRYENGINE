// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneCustom.h"

#include "DriverD3D.h"
#include "Common/ReverseDepth.h"


void CSceneCustomStage::Init()
{
	// Create per-pass resources
	m_pPerPassResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	
	bool bSuccess = PreparePerPassResources(true);
	assert(bSuccess);
	
	// Create resource layout
	SDeviceResourceLayoutDesc layoutDesc;
	layoutDesc.SetConstantBuffer(EResourceLayoutSlot_PerInstanceCB, eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerMaterialRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialResources());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerInstanceExtraRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultInstanceExtraResources());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerPassRS, m_pPerPassResources);
	m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(layoutDesc);
	assert(m_pResourceLayout != nullptr);
	
	// Wireframe Pass
	m_wireframePass.SetLabel("CUSTOM_WIREFRAME");
	m_wireframePass.SetupPassContext(m_stageID, ePass_Wireframe, TTYPE_DEBUG, FB_GENERAL);
	m_wireframePass.SetPassResources(m_pResourceLayout, m_pPerPassResources);
	m_wireframePass.SetRenderTargets(
	  // Depth
	  &gcpRendD3D->m_DepthBufferOrigMSAA,
	  // Color 0
	  gcpRendD3D->GetBackBufferTexture()
	  );

	// Highlighted ID Pass
	m_selectionIDPass.SetLabel("CUSTOM_HIGHLIGHTED_PASS");
	m_selectionIDPass.SetupPassContext(m_stageID, ePass_SelectionIDs, TTYPE_DEBUG, FB_GENERAL, EFSLIST_CUSTOM);
	m_selectionIDPass.SetPassResources(m_pResourceLayout, m_pPerPassResources);
	m_selectionIDPass.SetRenderTargets(
		// Depth. Initialize with null since depth texture will not be properly initialized at this point
		0,
		// Color 0
		CTexture::s_ptexSceneSelectionIDs
	);

	m_samplerPoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
	m_samplerLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
}

bool CSceneCustomStage::CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	outPSO = NULL;

	CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), desc);
	if (!pRenderer->GetGraphicsPipeline().FillCommonScenePassStates(desc, psoDesc))
		return true;

	CSceneRenderPass* pSceneRenderPass;
	
	if (passID == ePass_Wireframe)
	{
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
		psoDesc.m_RenderState = GS_DEPTHFUNC_LEQUAL | GS_WIREFRAME | GS_DEPTHWRITE;
		psoDesc.m_CullMode = eCULL_None;
		pSceneRenderPass = &m_wireframePass;
	}
	else if (passID == ePass_SelectionIDs)
	{
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		psoDesc.m_RenderState = GS_DEPTHFUNC_LEQUAL | GS_DEPTHWRITE;
		pSceneRenderPass = &m_selectionIDPass;
		// extract depth format here rather than in ExtractRenderTargetFormats since the depth texture may not exist yet.
		psoDesc.m_DepthStencilFormat = eTF_D32F;
	}

	if (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
	{
		psoDesc.m_RenderState = ReverseDepthHelper::ConvertDepthFunc(psoDesc.m_RenderState);
	}

	pSceneRenderPass->ExtractRenderTargetFormats(psoDesc);

	outPSO = CCryDeviceWrapper::GetObjectFactory().CreateGraphicsPSO(psoDesc);
	return outPSO != nullptr;
}

bool CSceneCustomStage::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	DevicePipelineStatesArray& stageStates = pStateArray[m_stageID];

	if (pStateCache->Find(stateDesc, stageStates))
		return true;

	bool bFullyCompiled = true;

	SGraphicsPipelineStateDescription _stateDesc = stateDesc;

	_stateDesc.technique = TTYPE_DEBUG;
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_Wireframe, stageStates[ePass_Wireframe]);
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_SelectionIDs, stageStates[ePass_SelectionIDs]);

	if (bFullyCompiled)
	{
		pStateCache->Put(stateDesc, stageStates);
	}

	return bFullyCompiled;
}

bool CSceneCustomStage::PreparePerPassResources(bool bOnInit)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	m_pPerPassResources->Clear();

	// Samplers
	{
		auto materialSamplers = pRenderer->GetGraphicsPipeline().GetDefaultMaterialSamplers();
		for (int i = 0; i < materialSamplers.size(); ++i)
		{
			m_pPerPassResources->SetSampler(EEfResSamplers(i), materialSamplers[i], EShaderStage_AllWithoutCompute);
		}
		// Hardcoded point samplers
		m_pPerPassResources->SetSampler(8, pRenderer->m_nPointWrapSampler, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetSampler(9, pRenderer->m_nPointClampSampler, EShaderStage_AllWithoutCompute);
	}

	// Textures
	{
		int nTerrainTex0 = 0, nTerrainTex1 = 0, nTerrainTex2 = 0;
		if (gEnv->p3DEngine && gEnv->p3DEngine->GetITerrain())
			gEnv->p3DEngine->GetITerrain()->GetAtlasTexId(nTerrainTex0, nTerrainTex1, nTerrainTex2);

		// bind the scene depth buffer before the regular scene shader texture IDs.
		// TODO: This is fragile though and there should be a way to allocate unused IDs for this
		m_pPerPassResources->SetTexture(ePerPassTexture_SceneDepthBuffer, pRenderer->m_DepthBufferOrigMSAA.pTexture, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);

		m_pPerPassResources->SetTexture(ePerPassTexture_WindGrid, CTexture::s_ptexWindGrid, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetTexture(ePerPassTexture_TerrainElevMap, CTexture::GetByID(nTerrainTex2), SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetTexture(ePerPassTexture_TerrainNormMap, CTexture::GetByID(nTerrainTex1), SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetTexture(ePerPassTexture_TerrainBaseMap, CTexture::GetByID(nTerrainTex0), SResourceView::DefaultViewSRGB, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetTexture(ePerPassTexture_NormalsFitting, CTexture::s_ptexNormalsFitting, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		m_pPerPassResources->SetTexture(ePerPassTexture_DissolveNoise, CTexture::s_ptexDissolveNoiseMap, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
	}
	
	// Constant buffers
	{
		pRenderer->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
		CConstantBufferPtr pPerViewCB;
		if (bOnInit)  // Handle case when no view is available in the initialization of the stage
			pPerViewCB = CDeviceBufferManager::CreateNullConstantBuffer();
		else
			pPerViewCB = pRenderer->GetGraphicsPipeline().GetPerViewConstantBuffer();
		
		m_pPerPassResources->SetConstantBuffer(eConstantBufferShaderSlot_PerView, pPerViewCB, EShaderStage_AllWithoutCompute);
		
		if (bOnInit)
			return true;
	}

	m_pPerPassResources->Build();
	return m_pPerPassResources->IsValid();
}

struct CHighlightPredicate
{
	bool operator() (SRendItem& item)
	{
		return (item.pObj->m_editorSelectionID & 0x2) == 0;
	}
};

struct CSelectionPredicate
{
	bool operator() (SRendItem& item)
	{
		return (item.pObj->m_editorSelectionID & 0x1) == 0;
	}
};

void CSceneCustomStage::Execute()
{	
	CD3D9Renderer* pRenderer = gcpRendD3D;
	bool bWireframePass = pRenderer->GetWireframeMode() != R_SOLID_MODE;
	// should probably somehow allow some editor viewports to not use this pass
	bool bSelectionIDPass = pRenderer->IsEditorMode() && !gEnv->IsEditorGameMode();

	if (!bWireframePass && !bSelectionIDPass)
		return;
	
	PROFILE_LABEL_SCOPE("CUSTOM_SCENE_PASSES");
	
	if (bWireframePass)
	{
		D3DViewPort viewport = { 0.f, 0.f, float(pRenderer->m_MainViewport.nWidth), float(pRenderer->m_MainViewport.nHeight), 0.0f, 1.0f };
		pRenderer->RT_SetViewport(0, 0, int(viewport.Width), int(viewport.Height));

		PreparePerPassResources(false);

		const bool bReverseDepth = (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
		pRenderer->FX_ClearTarget(&pRenderer->m_DepthBufferOrigMSAA, CLEAR_ZBUFFER | CLEAR_STENCIL, bReverseDepth ? 0.0f : 1.0f, 1);
		pRenderer->FX_ClearTarget(pRenderer->GetBackBufferTexture(), ColorF(0.2, 0.2, 0.2, 1));

		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
		m_wireframePass.PrepareRenderPassForUse(commandList);
	
		CRenderView* pRenderView = pRenderer->GetGraphicsPipeline().GetCurrentRenderView();
		m_wireframePass.SetFlags(CSceneRenderPass::ePassFlags_VrProjectionPass);
		m_wireframePass.SetViewport(viewport);
	
		RenderView()->GetDrawer().InitDrawSubmission();

		m_wireframePass.BeginExecution();
		m_wireframePass.DrawRenderItems(pRenderView, EFSLIST_GENERAL);
		m_wireframePass.EndExecution();

		RenderView()->GetDrawer().JobifyDrawSubmission();
		RenderView()->GetDrawer().WaitForDrawSubmission();
	}
	else if (bSelectionIDPass)
	{
		// first check if we actually have anything worth drawing
		uint32 numItems = gcpRendD3D->m_RP.m_pCurrentRenderView->GetRenderItems(EFSLIST_CUSTOM).size();

		if (numItems == 0)
			return;

		// update our depth texture here
		CTexture* pDepthRT = CTexture::s_ptexSceneHalfDepthStencil;

		m_depthTarget.nWidth = pDepthRT->GetWidth();
		m_depthTarget.nHeight = pDepthRT->GetHeight();
		m_depthTarget.nFrameAccess = -1;
		m_depthTarget.bBusy = false;
		m_depthTarget.pTexture = pDepthRT;
		m_depthTarget.pTarget = pDepthRT->GetDevTexture()->Get2DTexture();
		m_depthTarget.pSurface = pDepthRT->GetDeviceDepthStencilView();

		D3DViewPort viewport = { 0.f, 0.f, float(CTexture::s_ptexSceneSelectionIDs->GetWidth()), float(CTexture::s_ptexSceneSelectionIDs->GetHeight()), 0.0f, 1.0f };
		pRenderer->RT_SetViewport(0, 0, int(viewport.Width), int(viewport.Height));

		PreparePerPassResources(false);

		const bool bReverseDepth = (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
		pRenderer->FX_ClearTarget(&m_depthTarget, CLEAR_ZBUFFER | CLEAR_STENCIL, bReverseDepth ? 0.0f : 1.0f, 1);
		pRenderer->FX_ClearTarget(CTexture::s_ptexSceneSelectionIDs, ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
		m_selectionIDPass.SetRenderTargets(
			// Depth
			&m_depthTarget,
			// Color 0
			CTexture::s_ptexSceneSelectionIDs
		);
		m_selectionIDPass.PrepareRenderPassForUse(commandList);

		CRenderView* pRenderView = pRenderer->GetGraphicsPipeline().GetCurrentRenderView();
		m_selectionIDPass.SetFlags(CSceneRenderPass::ePassFlags_None);
		m_selectionIDPass.SetViewport(viewport);

		pRenderView->GetDrawer().InitDrawSubmission();

		CHighlightPredicate highlightPredicate;
		CSelectionPredicate selectionPredicate;

		uint32 startHighlight = pRenderView->FindRenderListSplit(highlightPredicate, EFSLIST_CUSTOM, 0, numItems);
		uint32 startSelected = pRenderView->FindRenderListSplit(selectionPredicate, EFSLIST_CUSTOM, 0, startHighlight);

		// First pass, draw selected object IDs
		m_selectionIDPass.BeginExecution();
		m_selectionIDPass.DrawRenderItems(pRenderView, EFSLIST_CUSTOM, startSelected, numItems);
		m_selectionIDPass.EndExecution();
		pRenderView->GetDrawer().JobifyDrawSubmission();
		pRenderView->GetDrawer().WaitForDrawSubmission();

		// Now, we use the selection ID texture to alpha composite highlights and outlines on our scene
		viewport.Width = float(pRenderer->m_MainViewport.nWidth);
		viewport.Height = float(pRenderer->m_MainViewport.nHeight);

		pRenderer->RT_SetViewport(0, 0, int(viewport.Width), int(viewport.Height));

		static CCryNameTSCRC techSilhouette("SelectionSilhouetteHighlight");

		if (m_highlightPass.InputChanged())
		{
			m_highlightPass.SetTechnique(CShaderMan::s_shPostEffects, techSilhouette, 0);
			m_highlightPass.SetRenderTarget(0, pRenderer->GetBackBufferTexture());
			m_highlightPass.SetState(GS_NODEPTHTEST | GS_BLDST_ONEMINUSSRCALPHA | GS_BLSRC_SRCALPHA);
			m_highlightPass.SetTextureSamplerPair(0, CTexture::s_ptexSceneSelectionIDs, m_samplerPoint);
		}
		static CCryNameR highlightColorName("highlightColor");
		static CCryNameR selectionColorName("selectionColor");
		static CCryNameR ghostAlphaName("ghostAlpha");
		static CCryNameR outlineName("outlineWidth");

		m_highlightPass.BeginConstantUpdate();
		m_highlightPass.SetConstant(highlightColorName, pRenderer->GetHighlightColor().toVec4(), eHWSC_Pixel);
		m_highlightPass.SetConstant(selectionColorName, pRenderer->GetSelectionColor().toVec4(), eHWSC_Pixel);
		m_highlightPass.SetConstant(ghostAlphaName, Vec4(pRenderer->GetHighlightParams().y), eHWSC_Pixel);
		m_highlightPass.SetConstant(outlineName, Vec4(pRenderer->GetHighlightParams().x), eHWSC_Vertex);

		m_highlightPass.SetViewport(viewport);
		m_highlightPass.Execute();

		// reset the depth target
		m_selectionIDPass.SetRenderTargets(
			// Depth
			0,
			// Color 0
			CTexture::s_ptexSceneSelectionIDs
		);
	}
}