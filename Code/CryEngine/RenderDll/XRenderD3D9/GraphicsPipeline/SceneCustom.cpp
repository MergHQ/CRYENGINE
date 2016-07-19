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
}

bool CSceneCustomStage::CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	outPSO = NULL;

	CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), desc);
	if (!pRenderer->GetGraphicsPipeline().FillCommonScenePassStates(desc, psoDesc))
		return true;

	CSceneRenderPass* pSceneRenderPass = &m_wireframePass;
	
	if (passID == ePass_Wireframe)
	{
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
		psoDesc.m_RenderState = GS_DEPTHFUNC_LEQUAL | GS_WIREFRAME | GS_DEPTHWRITE;
		psoDesc.m_CullMode = eCULL_None;
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

void CSceneCustomStage::Execute()
{	
	CD3D9Renderer* pRenderer = gcpRendD3D;
	bool bWireframePass = pRenderer->GetWireframeMode() != R_SOLID_MODE;

	if (!bWireframePass)
		return;
	
	PROFILE_LABEL_SCOPE("CUSTOM_SCENE_PASSES");
	
	D3DViewPort viewport = { 0.f, 0.f, float(pRenderer->m_MainViewport.nWidth), float(pRenderer->m_MainViewport.nHeight), 0.0f, 1.0f };
	pRenderer->RT_SetViewport(0, 0, int(viewport.Width), int(viewport.Height));
	
	PreparePerPassResources(false);
	
	if (bWireframePass)
	{
		const bool bReverseDepth = (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
		pRenderer->FX_ClearTarget(&pRenderer->m_DepthBufferOrigMSAA, CLEAR_ZBUFFER | CLEAR_STENCIL, bReverseDepth ? 0.0f : 1.0f, 1);
		pRenderer->FX_ClearTarget(pRenderer->GetBackBufferTexture(), ColorF(0.2, 0.2, 0.2, 1));

		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
		m_wireframePass.PrepareRenderPassForUse(commandList);
	
		CRenderView* pRenderView = pRenderer->GetGraphicsPipeline().GetCurrentRenderView();
		m_wireframePass.SetFlags(CSceneRenderPass::ePassFlags_None);
		m_wireframePass.SetViewport(viewport);
	
		RenderView()->GetDrawer().InitDrawSubmission();

		m_wireframePass.DrawRenderItems(pRenderView, EFSLIST_GENERAL);

		RenderView()->GetDrawer().JobifyDrawSubmission();
		RenderView()->GetDrawer().WaitForDrawSubmission();
	}
}