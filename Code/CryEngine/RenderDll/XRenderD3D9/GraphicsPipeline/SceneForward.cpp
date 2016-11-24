// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneForward.h"

#include "DriverD3D.h"
#include "Common/ReverseDepth.h"


void CSceneForwardStage::Init()
{
	// Create per-pass resources
	m_pOpaquePassResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	m_pTransparentPassResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	
	bool bSuccess = PreparePerPassResources(true);
	assert(bSuccess);
	
	// Create resource layout
	SDeviceResourceLayoutDesc layoutDesc;
	layoutDesc.SetConstantBuffer(EResourceLayoutSlot_PerInstanceCB, eConstantBufferShaderSlot_PerInstance, EShaderStage_Vertex | EShaderStage_Pixel);
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerMaterialRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialResources());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerInstanceExtraRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultInstanceExtraResources());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerPassRS, m_pOpaquePassResources);
	m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(layoutDesc);
	assert(m_pResourceLayout != nullptr);
	
	// Opaque forward scene pass
	m_forwardOpaquePass.SetLabel("TILED_FORWARD_OPAQUE");
	m_forwardOpaquePass.SetupPassContext(m_stageID, ePass_Forward, TTYPE_GENERAL, FB_TILED_FORWARD);
	m_forwardOpaquePass.SetPassResources(m_pResourceLayout, m_pOpaquePassResources);
	m_forwardOpaquePass.SetRenderTargets(
		// Depth
		&gcpRendD3D->m_DepthBufferOrigMSAA,
		// Color 0
		CTexture::s_ptexHDRTarget
	);
}

bool CSceneForwardStage::CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	outPSO = nullptr;

	if (!(desc.shaderItem.m_nPreprocessFlags & FB_TILED_FORWARD))
		return true;
	
	CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), desc);
	if (!pRenderer->GetGraphicsPipeline().FillCommonScenePassStates(desc, psoDesc))
		return true;

	psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

	CSceneRenderPass* pSceneRenderPass = &m_forwardOpaquePass;

	if (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
	{
		psoDesc.m_RenderState = ReverseDepthHelper::ConvertDepthFunc(psoDesc.m_RenderState);
	}

	psoDesc.m_RenderState &= ~(GS_DEPTHWRITE | GS_DEPTHFUNC_MASK | GS_ALPHATEST);
	psoDesc.m_RenderState |= GS_DEPTHFUNC_EQUAL;
	psoDesc.m_ShaderFlags_RT &= ~g_HWSR_MaskBit[HWSR_ALPHATEST];

	pSceneRenderPass->ExtractRenderTargetFormats(psoDesc);

	outPSO = CCryDeviceWrapper::GetObjectFactory().CreateGraphicsPSO(psoDesc);
	return outPSO != nullptr;
}

bool CSceneForwardStage::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	DevicePipelineStatesArray& stageStates = pStateArray[m_stageID];

	if (pStateCache->Find(stateDesc, stageStates))
		return true;

	bool bFullyCompiled = true;

	SGraphicsPipelineStateDescription _stateDesc = stateDesc;

	_stateDesc.technique = TTYPE_GENERAL;
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_Forward, stageStates[ePass_Forward]);

	if (bFullyCompiled)
	{
		pStateCache->Put(stateDesc, stageStates);
	}

	return bFullyCompiled;
}

bool CSceneForwardStage::PreparePerPassResources(bool bOnInit)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	for (uint32 i = 0; i < 2; i++)
	{
		CDeviceResourceSetPtr pResourceSet = (i == 0) ? m_pOpaquePassResources : m_pTransparentPassResources;
		pResourceSet->Clear();

		// Samplers
		{
			auto materialSamplers = gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialSamplers();
			for (int i = 0; i < materialSamplers.size(); ++i)
			{
				pResourceSet->SetSampler(EEfResSamplers(i), materialSamplers[i], EShaderStage_AllWithoutCompute);
			}
			// Hardcoded point samplers
			pResourceSet->SetSampler(8, gcpRendD3D->m_nPointWrapSampler, EShaderStage_AllWithoutCompute);
			pResourceSet->SetSampler(9, gcpRendD3D->m_nPointClampSampler, EShaderStage_AllWithoutCompute);
		}

		// Textures
		{
			int nTerrainTex0 = 0, nTerrainTex1 = 0, nTerrainTex2 = 0;
			if (gEnv->p3DEngine && gEnv->p3DEngine->GetITerrain())
				gEnv->p3DEngine->GetITerrain()->GetAtlasTexId(nTerrainTex0, nTerrainTex1, nTerrainTex2);

			pResourceSet->SetTexture(ePerPassTexture_WindGrid, CTexture::s_ptexWindGrid, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(ePerPassTexture_TerrainElevMap, CTexture::GetByID(nTerrainTex2), SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(ePerPassTexture_TerrainNormMap, CTexture::GetByID(nTerrainTex1), SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(ePerPassTexture_TerrainBaseMap, CTexture::GetByID(nTerrainTex0), SResourceView::DefaultViewSRGB, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(ePerPassTexture_NormalsFitting, CTexture::s_ptexNormalsFitting, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(ePerPassTexture_DissolveNoise, CTexture::s_ptexDissolveNoiseMap, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		}

		// Tiled shading resources
		{
			CTiledShading& tiledShading = pRenderer->GetTiledShading();
			pResourceSet->SetBuffer(17, tiledShading.m_tileOpaqueLightMaskBuf, false, EShaderStage_AllWithoutCompute);
			pResourceSet->SetBuffer(18, tiledShading.m_LightShadeInfoBuf, false, EShaderStage_AllWithoutCompute);
			pResourceSet->SetBuffer(19, tiledShading.m_clipVolumeInfoBuf, false, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(20, tiledShading.m_specularProbeAtlas.texArray, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(21, tiledShading.m_diffuseProbeAtlas.texArray, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(22, tiledShading.m_spotTexAtlas.texArray, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(23, CTexture::s_ptexShadowMask, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			pResourceSet->SetTexture(24, CTexture::s_ptexSceneNormalsBent, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);

			if (CRenderer::CV_r_DeferredShadingTiled < 3)
			{
				pResourceSet->SetBuffer(17, tiledShading.m_tileTranspLightMaskBuf, false, EShaderStage_AllWithoutCompute);
			}
			
			// Overwrite resources for transparent pass (need to be careful that the layout is still the same)
			if (i == 1)
			{
				pResourceSet->SetBuffer(17, tiledShading.m_tileTranspLightMaskBuf, false, EShaderStage_AllWithoutCompute);
				pResourceSet->SetTexture(23, CTexture::s_ptexRT_ShadowPool, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
				pResourceSet->SetTexture(24, CTexture::s_ptexShadowJitterMap, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
			}
		}
	
		// Constant buffers
		{
			pRenderer->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
			CConstantBufferPtr pPerViewCB;
			if (bOnInit)  // Handle case when no view is available in the initialization of the stage
				pPerViewCB = CDeviceBufferManager::CreateNullConstantBuffer();
			else
				pPerViewCB = pRenderer->GetGraphicsPipeline().GetPerViewConstantBuffer();
		
			pResourceSet->SetConstantBuffer(eConstantBufferShaderSlot_PerView, pPerViewCB, EShaderStage_AllWithoutCompute);
		
			if (bOnInit)
				continue;
		}

		pResourceSet->Build();
	}

	return bOnInit || (m_pOpaquePassResources->IsValid() && m_pTransparentPassResources->IsValid());
}

void CSceneForwardStage::Execute_Opaque()
{
	PROFILE_LABEL_SCOPE("FORWARD_OPAQUE");

	CD3D9Renderer* pRenderer = gcpRendD3D;
	
	if (CRenderer::CV_r_DeferredShadingTiled > 0)
	{
		PROFILE_LABEL_SCOPE("TILED_FORWARD_OPAQUE");
		
		D3DViewPort viewport = { 0.f, 0.f, float(pRenderer->m_MainViewport.nWidth), float(pRenderer->m_MainViewport.nHeight), 0.0f, 1.0f };
		pRenderer->RT_SetViewport(0, 0, int(viewport.Width), int(viewport.Height));
	
		PreparePerPassResources(false);

		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
		m_forwardOpaquePass.PrepareRenderPassForUse(commandList);
	
		m_forwardOpaquePass.SetFlags(CSceneRenderPass::ePassFlags_VrProjectionPass);
		m_forwardOpaquePass.SetViewport(viewport);

		CRenderView* pRenderView = gcpRendD3D->GetGraphicsPipeline().GetCurrentRenderView();
		RenderView()->GetDrawer().InitDrawSubmission();

		m_forwardOpaquePass.BeginExecution();
		m_forwardOpaquePass.DrawRenderItems(pRenderView, EFSLIST_FORWARD_OPAQUE);
		m_forwardOpaquePass.EndExecution();

		RenderView()->GetDrawer().JobifyDrawSubmission();
		RenderView()->GetDrawer().WaitForDrawSubmission();
	}
	
	// Legacy pipeline
	{
		void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;
		pRenderer->m_RP.m_PersFlags2 |= RBPF2_FORWARD_SHADING_PASS;

		// Note: Eye overlay writes to diffuse color buffer for eye shader reading
		pRenderer->FX_ProcessEyeOverlayRenderLists(EFSLIST_EYE_OVERLAY, pRenderFunc, true);

		{
			PROFILE_LABEL_SCOPE("FORWARD_OPAQUE");
			pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
			pRenderer->FX_ProcessRenderList(EFSLIST_FORWARD_OPAQUE, pRenderFunc, true, FB_GENERAL, FB_TILED_FORWARD);
			pRenderer->GetTiledShading().UnbindForwardShadingResources();
		}

		{
			PROFILE_LABEL_SCOPE("TERRAINLAYERS");
			pRenderer->FX_ProcessRenderList(EFSLIST_TERRAINLAYER, pRenderFunc, true);
		}

		{
			PROFILE_LABEL_SCOPE("FORWARD_DECALS");
			pRenderer->FX_ProcessRenderList(EFSLIST_DECAL, pRenderFunc, true);
		}

		pRenderer->FX_ProcessSkinRenderLists(EFSLIST_SKIN, pRenderFunc, true);

		pRenderer->m_RP.m_PersFlags2 &= ~RBPF2_FORWARD_SHADING_PASS;
	}
}

void CSceneForwardStage::Execute_TransparentBelowWater()
{
	PROFILE_LABEL_SCOPE("TRANSPARENT_BW");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;

	if (!SRendItem::IsListEmpty(EFSLIST_TRANSP) && (SRendItem::BatchFlags(EFSLIST_TRANSP) & FB_BELOW_WATER))
	{
		pRenderer->FX_ScreenStretchRect(CTexture::s_ptexCurrSceneTarget);
	}

	pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
	pRenderer->FX_ProcessRenderList(EFSLIST_TRANSP, pRenderFunc, true, FB_BELOW_WATER, 0);
	pRenderer->GetTiledShading().UnbindForwardShadingResources();
}

void CSceneForwardStage::Execute_TransparentAboveWater()
{
	PROFILE_LABEL_SCOPE("TRANSPARENT_AW");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;

	if (!SRendItem::IsListEmpty(EFSLIST_TRANSP))
	{
		pRenderer->FX_ScreenStretchRect(CTexture::s_ptexCurrSceneTarget);
	}

	pRenderer->GetTiledShading().BindForwardShadingResources(NULL);
	pRenderer->FX_ProcessRenderList(EFSLIST_TRANSP, pRenderFunc, true, FB_GENERAL, FB_BELOW_WATER);
	pRenderer->GetTiledShading().UnbindForwardShadingResources();
}
