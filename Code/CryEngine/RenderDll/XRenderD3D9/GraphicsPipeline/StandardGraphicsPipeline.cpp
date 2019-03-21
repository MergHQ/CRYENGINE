// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "StandardGraphicsPipeline.h"

#include "ShadowMap.h"
#include "SceneGBuffer.h"
#include "SceneDepth.h"
#include "SceneForward.h"
#include "Sky.h"
#include "SceneCustom.h"
#include "AutoExposure.h"
#include "Bloom.h"
#include "HeightMapAO.h"
#include "ScreenSpaceObscurance.h"
#include "ScreenSpaceReflections.h"
#include "ScreenSpaceSSS.h"
#include "VolumetricFog.h"
#include "Fog.h"
#include "VolumetricClouds.h"
#include "Water.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "SunShafts.h"
#include "ToneMapping.h"
#include "PostAA.h"
#include "ComputeSkinning.h"
#include "GpuParticles.h"
#include "ClipVolumes.h"
#include "DeferredDecals.h"
#include "ShadowMask.h"
#include "TiledShading.h"
#include "ColorGrading.h"
#include "WaterRipples.h"
#include "LensOptics.h"
#include "PostEffects.h"
#include "Rain.h"
#include "Snow.h"
#include "MobileComposition.h"
#include "OmniCamera.h"
#include "TiledLightVolumes.h"
#include "DebugRenderTargets.h"

#include "Common/TypedConstantBuffer.h"
#include "Common/Textures/TextureHelpers.h"
#include "Common/Include_HLSL_CPP_Shared.h"
#include "../Common/RenderView.h"

#include "Common/RenderDisplayContext.h"

#include "../D3DPostProcess.h"
#include "Common/ReverseDepth.h"
#include "D3D_SVO.h"

CStandardGraphicsPipeline::CStandardGraphicsPipeline(const IRenderer::SGraphicsPipelineDescription& desc, const std::string& uniqueIdentifier, const SGraphicsPipelineKey key)
	: CGraphicsPipeline(desc, uniqueIdentifier, key)
	, m_changedCVars(gEnv->pConsole)
{}

void CStandardGraphicsPipeline::Init()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "CStandardGraphicsPipeline::Init");

	// default material bind points
	{
		m_defaultMaterialBindPoints.SetConstantBuffer(eConstantBufferShaderSlot_PerMaterial, CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_AllWithoutCompute);

		for (EEfResTextures texType = EFTT_DIFFUSE; texType < EFTT_MAX; texType = EEfResTextures(texType + 1))
		{
			if (TextureHelpers::IsSlotAvailable(texType))
			{
				EShaderStage shaderStages = TextureHelpers::GetShaderStagesForTexSlot(texType);
				m_defaultMaterialBindPoints.SetTexture(texType, CRendererResources::s_pTexNULL, EDefaultResourceViews::Default, shaderStages);
			}
		}
	}

	// default extra per instance
	{
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat    , CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_PerGroup    , CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Hull);

		// Deliberately aliasing slots/use-cases here for visibility (e.g. EReservedTextureSlot_ComputeSkinVerts, EReservedTextureSlot_SkinExtraWeights and
		// EReservedTextureSlot_GpuParticleStream). The resource layout will just pick the first.
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_SkinExtraWeights , CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_ComputeSkinVerts , CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_GpuParticleStream, CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_AdjacencyInfo    , CDeviceBufferManager::GetNullBufferTyped()     , EDefaultResourceViews::Default, EShaderStage_Domain);

		m_pDefaultDrawExtraRS = GetDeviceObjectFactory().CreateResourceSet();
		m_pDefaultDrawExtraRS->Update(m_defaultDrawExtraRL);
	}

	// per view constant buffer
	m_mainViewConstantBuffer.CreateDeviceBuffer();

	// Register all common stages
	CGraphicsPipeline::Init();

	// Register all other stages that don't need the global PSO cache
	RegisterStage<CSceneCustomStage>();
	RegisterStage<CHeightMapAOStage>();
	RegisterStage<CScreenSpaceObscuranceStage>();
	RegisterStage<CScreenSpaceReflectionsStage>();
	RegisterStage<CScreenSpaceSSSStage>();
	RegisterStage<CVolumetricCloudsStage>();
	RegisterStage<CWaterRipplesStage>();
	RegisterStage<CMotionBlurStage>();
	RegisterStage<CDepthOfFieldStage>();
	RegisterStage<CAutoExposureStage>();
	RegisterStage<CBloomStage>();
	RegisterStage<CToneMappingStage>();
	RegisterStage<CSunShaftsStage>();
	RegisterStage<CPostAAStage>();
	RegisterStage<CComputeSkinningStage>();
	RegisterStage<CComputeParticlesStage>();
	RegisterStage<CDeferredDecalsStage>();
	RegisterStage<CShadowMaskStage>();
	RegisterStage<CTiledShadingStage>();
	RegisterStage<CWaterStage>(); // Has a custom PSO cache like Forward
	RegisterStage<CLensOpticsStage>();
	RegisterStage<CColorGradingStage>();
	RegisterStage<CPostEffectStage>();
	RegisterStage<CRainStage>();
	RegisterStage<CSnowStage>();
	RegisterStage<CSceneDepthStage>();
	RegisterStage<COmniCameraStage>();
	RegisterStage<CVolumetricFogStage>();

	// Now init stages
	InitStages();

	// Out-of-pipeline passes for display
	m_HDRToFramePass.reset(new CStretchRectPass(this));
	m_PostToFramePass.reset(new CStretchRectPass(this));
	m_FrameToFramePass.reset(new CStretchRectPass(this));

	m_HQSubResPass[0].reset(new CStableDownsamplePass(this));
	m_HQSubResPass[1].reset(new CStableDownsamplePass(this));
	m_LQSubResPass[0].reset(new CStretchRectPass(this));
	m_LQSubResPass[1].reset(new CStretchRectPass(this));

	m_ResolvePass.reset(new CStretchRectPass(this));
	m_DownscalePass.reset(new CDownsamplePass(this));
	m_UpscalePass.reset(new CSharpeningUpsamplePass(this));
	m_AnisoVBlurPass.reset(new CAnisotropicVerticalBlurPass(this));

	// preallocate light volume buffer
	GetLightVolumeBuffer().Create();

	m_bInitialized = true;
}

//////////////////////////////////////////////////////////////////////////
void CStandardGraphicsPipeline::Resize(int renderWidth, int renderHeight)
{
	CGraphicsPipeline::Resize(renderWidth, renderHeight);
}

//////////////////////////////////////////////////////////////////////////
void CStandardGraphicsPipeline::ShutDown()
{
	m_bInitialized = false;

	CGraphicsPipeline::ShutDown();
	CGraphicsPipeline::SetCurrentRenderView(nullptr);

	m_mainViewConstantBuffer.Clear();
	m_defaultDrawExtraRL.ClearResources();
	m_pDefaultDrawExtraRS.reset();

	m_HDRToFramePass.reset();
	m_PostToFramePass.reset();
	m_FrameToFramePass.reset();

	m_HQSubResPass[0].reset();
	m_HQSubResPass[1].reset();
	m_LQSubResPass[0].reset();
	m_LQSubResPass[1].reset();

	m_ResolvePass.reset();
	m_DownscalePass.reset();
	m_UpscalePass.reset();
}

//////////////////////////////////////////////////////////////////////////
void CStandardGraphicsPipeline::Update(EShaderRenderingFlags renderingFlags)
{
	FUNCTION_PROFILER_RENDERER();

	m_numInvalidDrawcalls = 0;
	GenerateMainViewConstantBuffer();

	if (!m_changedCVars.GetCVars().empty())
	{
		CGraphicsPipeline::OnCVarsChanged(m_changedCVars);
		m_changedCVars.Reset();
	}

	m_renderingFlags = renderingFlags;
	CGraphicsPipeline::Update(renderingFlags);
}

//////////////////////////////////////////////////////////////////////////
bool CStandardGraphicsPipeline::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, SGraphicsPipelineStateDescription stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	// NOTE: Please update SDeviceObjectHelpers::CheckTessellationSupport when adding new techniques types here.

	bool bFullyCompiled = true;

	// GBuffer
	{
		stateDesc.technique = TTYPE_Z;
		bFullyCompiled &= GetStage<CSceneGBufferStage>()->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	// ShadowMap
	{
		stateDesc.technique = TTYPE_SHADOWGEN;
		bFullyCompiled &= GetStage<CShadowMapStage>()->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	// Forward
	{
		stateDesc.technique = TTYPE_GENERAL;
		bFullyCompiled &= GetStage<CSceneForwardStage>()->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

#if RENDERER_ENABLE_FULL_PIPELINE
	// Custom
	{
		stateDesc.technique = TTYPE_DEBUG;
		bFullyCompiled &= GetStage<CSceneCustomStage>()->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}
#endif

	return bFullyCompiled;
}

bool CStandardGraphicsPipeline::FillCommonScenePassStates(const SGraphicsPipelineStateDescription& inputDesc, CDeviceGraphicsPSODesc& psoDesc)
{
	CShader* pShader = static_cast<CShader*>(inputDesc.shaderItem.m_pShader);
	SShaderTechnique* pTechnique = pShader->GetTechnique(inputDesc.shaderItem.m_nTechnique, inputDesc.technique, true);
	if (!pTechnique)
		return false;

	CShaderResources* pRes = static_cast<CShaderResources*>(inputDesc.shaderItem.m_pShaderResources);
	const uint64 objectFlags = inputDesc.objectFlags;
	SShaderPass* pShaderPass = &pTechnique->m_Passes[0];

	// Handle quality flags
	CStandardGraphicsPipeline::ApplyShaderQuality(psoDesc, gcpRendD3D->GetShaderProfile(pShader->m_eShaderType));

	psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];

	// Set resource states
	bool bTwoSided = false;

	if (pRes->m_ResFlags & MTL_FLAG_2SIDED)
		bTwoSided = true;

	if (pRes->IsAlphaTested())
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];

	if (pRes->m_Textures[EFTT_DIFFUSE] && pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_pTexModifier)
		psoDesc.m_ShaderFlags_MD |= pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_nUpdateFlags;

	// Merge EDeformType into EVertexModifier to save space/parameters
	if (pRes->m_pDeformInfo)
		psoDesc.m_ShaderFlags_MDV |= EVertexModifier(pRes->m_pDeformInfo->m_eType);

	psoDesc.m_ShaderFlags_MDV |= psoDesc.m_pShader->m_nMDV;

	if (objectFlags & FOB_OWNER_GEOMETRY)
		psoDesc.m_ShaderFlags_MDV &= ~MDV_DEPTH_OFFSET;

	if (objectFlags & FOB_BENDED)
		psoDesc.m_ShaderFlags_MDV |= MDV_BENDING;

	if (!(objectFlags & FOB_TRANS_MASK))
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_OBJ_IDENTITY];

	if (objectFlags & FOB_BLEND_WITH_TERRAIN_COLOR)
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];

	psoDesc.m_bAllowTesselation = false;
	psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];

	if (objectFlags & FOB_NEAREST)
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_NEAREST];

	if (objectFlags & FOB_DISSOLVE)
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_DISSOLVE];

	if (psoDesc.m_RenderState & GS_ALPHATEST)
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];

#ifdef TESSELLATION_RENDERER
	const bool bHasTesselationShaders = pShaderPass && pShaderPass->m_HShader && pShaderPass->m_DShader;
	if (bHasTesselationShaders && (!(objectFlags & FOB_NEAREST) && (objectFlags & FOB_ALLOW_TESSELLATION)))
	{
		psoDesc.m_ShaderFlags_RT &= ~g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
		psoDesc.m_bAllowTesselation = true;
	}
#endif

	psoDesc.m_CullMode = bTwoSided ? eCULL_None : ((pShaderPass && pShaderPass->m_eCull != -1) ? (ECull)pShaderPass->m_eCull : eCULL_Back);
	psoDesc.m_PrimitiveType = inputDesc.primitiveType;

	if (psoDesc.m_bAllowTesselation)
	{
		psoDesc.m_PrimitiveType = ept3ControlPointPatchList;
		psoDesc.m_ObjectStreamMask |= VSM_NORMALS;
	}

	psoDesc.m_ShaderFlags_RT |= m_pVRProjectionManager->GetRTFlags();

	return true;
}

CDeviceResourceLayoutPtr CStandardGraphicsPipeline::CreateScenePassLayout(const CDeviceResourceSetDesc& perPassResources)
{
	SDeviceResourceLayoutDesc layoutDesc;

	layoutDesc.SetConstantBuffer(EResourceLayoutSlot_PerDrawCB, eConstantBufferShaderSlot_PerDraw, EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Domain);

	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerDrawExtraRS, GetDefaultDrawExtraResourceLayout());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerMaterialRS, GetDefaultMaterialBindPoints());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerPassRS, perPassResources);

	CDeviceResourceLayoutPtr pResourceLayout = GetDeviceObjectFactory().CreateResourceLayout(layoutDesc);
	assert(pResourceLayout != nullptr);
	return pResourceLayout;
}

void CStandardGraphicsPipeline::ExecuteHDRPostProcessing()
{
	FUNCTION_PROFILER_RENDERER();
	PROFILE_LABEL_SCOPE("POST_EFFECTS_HDR");

	const auto& viewInfo = GetCurrentViewInfo(CCamera::eEye_Left);
	PostProcessUtils().m_pView = viewInfo.viewMatrix;
	PostProcessUtils().m_pProj = viewInfo.projMatrix;

	PostProcessUtils().m_pViewProj = PostProcessUtils().m_pView * PostProcessUtils().m_pProj;
	PostProcessUtils().m_pViewProj = ReverseDepthHelper::Convert(PostProcessUtils().m_pViewProj);
	PostProcessUtils().m_pViewProj.Transpose();

	if (GetStage<CRainStage>()->IsStageActive(m_renderingFlags))
		GetStage<CRainStage>()->Execute();

	// Note: MB uses s_ptexHDRTargetPrev to avoid doing another copy, so this should be right before the MB pass
	{
		m_FrameToFramePass->Execute(CRendererResources::s_ptexHDRTarget, CRendererResources::s_ptexHDRTargetPrev);
	}

	if (GetStage<CDepthOfFieldStage>()->IsStageActive(m_renderingFlags))
		GetStage<CDepthOfFieldStage>()->Execute();

	if (GetStage<CMotionBlurStage>()->IsStageActive(m_renderingFlags))
		GetStage<CMotionBlurStage>()->Execute();

	if (GetStage<CSnowStage>()->IsStageActive(m_renderingFlags))
		GetStage<CSnowStage>()->Execute();

	// Half resolution downsampling
	if (GetStage<CAutoExposureStage>()->IsStageActive(m_renderingFlags) ||
	    GetStage<CBloomStage>()->IsStageActive(m_renderingFlags) ||
	    GetStage<CSunShaftsStage>()->IsStageActive(m_renderingFlags))
	{
		PROFILE_LABEL_SCOPE("HALFRES_DOWNSAMPLE_HDRTARGET");

		if (CRendererCVars::CV_r_HDRBloomQuality > 1)
			m_HQSubResPass[0]->Execute(CRendererResources::s_ptexHDRTarget, CRendererResources::s_ptexHDRTargetScaled[0][0], true);
		else
			m_LQSubResPass[0]->Execute(CRendererResources::s_ptexHDRTarget, CRendererResources::s_ptexHDRTargetScaled[0][0]);
	}

	// Quarter resolution downsampling
	if (GetStage<CAutoExposureStage>()->IsStageActive(m_renderingFlags) ||
	    GetStage<CBloomStage>()->IsStageActive(m_renderingFlags))
	{
		PROFILE_LABEL_SCOPE("QUARTER_RES_DOWNSAMPLE_HDRTARGET");

		if (CRendererCVars::CV_r_HDRBloomQuality > 0)
			m_HQSubResPass[1]->Execute(CRendererResources::s_ptexHDRTargetScaled[0][0], CRendererResources::s_ptexHDRTargetScaled[1][0], CRendererCVars::CV_r_HDRBloomQuality >= 1);
		else
			m_LQSubResPass[1]->Execute(CRendererResources::s_ptexHDRTargetScaled[0][0], CRendererResources::s_ptexHDRTargetScaled[1][0]);
	}

	// reads CRendererResources::s_ptexHDRTargetScaled[1][0]
	if (GetStage<CAutoExposureStage>()->IsStageActive(m_renderingFlags))
		GetStage<CAutoExposureStage>()->Execute();

	// reads CRendererResources::s_ptexHDRTargetScaled[1][0] and then kills it
	if (GetStage<CBloomStage>()->IsStageActive(m_renderingFlags))
		GetStage<CBloomStage>()->Execute();

	// writes CRendererResources::s_ptexSceneTargetR11G11B10F[0]
	if (GetStage<CLensOpticsStage>()->IsStageActive(m_renderingFlags))
		GetStage<CLensOpticsStage>()->Execute();

	// reads CRendererResources::s_ptexHDRTargetScaled[0][0]
	if (GetStage<CSunShaftsStage>()->IsStageActive(m_renderingFlags))
		GetStage<CSunShaftsStage>()->Execute();

	if (GetStage<CColorGradingStage>()->IsStageActive(m_renderingFlags))
		GetStage<CColorGradingStage>()->Execute();

	// 0 is used for disable debugging and 1 is used to just show the average and estimated luminance, and exposure values.
	if (GetStage<CToneMappingStage>()->IsDebugDrawEnabled())
		GetStage<CToneMappingStage>()->ExecuteDebug();
	else
		GetStage<CToneMappingStage>()->Execute();
}

void CStandardGraphicsPipeline::Execute()
{
	FUNCTION_PROFILER_RENDERER();

	// TODO: Make this into it's own stage or pipeline
	if (CSceneCustomStage::DoDebugRendering() && m_renderingFlags & EShaderRenderingFlags::SHDF_ALLOW_RENDER_DEBUG)
	{
		GetStage<CSceneCustomStage>()->ExecuteDebugger();

		if (GetStage<CSceneCustomStage>()->IsDebugOverlayEnabled())
			GetStage<CSceneCustomStage>()->ExecuteDebugOverlay();

		return;
	}

	CD3D9Renderer* pRenderer = gcpRendD3D;
	CRenderView* pRenderView = GetCurrentRenderView();
	auto& renderItemDrawer = pRenderView->GetDrawer();
	CTexture* pZTexture = pRenderView->GetDepthTarget();

	m_renderPassScheduler.SetEnabled(true);

	PROFILE_LABEL_PUSH("GRAPHICS_PIPELINE");

	// Generate cloud volume textures for shadow mapping. Only needs view, and needs to run before ShadowMaskgen.
	GetStage<CVolumetricCloudsStage>()->ExecuteShadowGen();

	if (pRenderView->GetCurrentEye() != CCamera::eEye_Right)
	{
		// Compute algorithms
		GetStage<CComputeSkinningStage>()->Execute();

		// Revert resource states to graphics pipeline
		GetStage<CComputeParticlesStage>()->PreDraw();
		GetStage<CComputeSkinningStage>()->PreDraw();

		if (GetStage<CRainStage>()->IsRainOcclusionEnabled())
			GetStage<CRainStage>()->ExecuteRainOcclusion();
	}

	// GBuffer
	GetStage<CSceneGBufferStage>()->Execute();

	// Wait for GBuffer draw jobs to finish
	renderItemDrawer.WaitForDrawSubmission();

	// Issue split barriers for GBuffer
	CTexture* pTextures[] = {
		CRendererResources::s_ptexSceneNormalsMap,
		CRendererResources::s_ptexSceneDiffuse,
		CRendererResources::s_ptexSceneSpecular,
		pZTexture
	};

	CDeviceGraphicsCommandInterface* pCmdList = GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface();
	pCmdList->BeginResourceTransitions(CRY_ARRAY_COUNT(pTextures), pTextures, eResTransition_TextureRead);

	// Shadow maps
	if (GetStage<CShadowMapStage>()->IsStageActive(m_renderingFlags))
		GetStage<CShadowMapStage>()->Execute();

	// Wait for Shadow Map draw jobs to finish (also required for HeightMap AO and SVOGI)
	renderItemDrawer.WaitForDrawSubmission();

	if (GetStage<CDeferredDecalsStage>()->IsStageActive(m_renderingFlags))
		GetStage<CDeferredDecalsStage>()->Execute();

	if (GetStage<CSceneGBufferStage>()->IsGBufferVisualizationEnabled())
		GetStage<CSceneGBufferStage>()->ExecuteGBufferVisualization();

	// GBuffer modifiers
	if (GetStage<CRainStage>()->IsDeferredRainEnabled())
		GetStage<CRainStage>()->ExecuteDeferredRainGBuffer();

	if (GetStage<CSnowStage>()->IsDeferredSnowEnabled())
		GetStage<CSnowStage>()->ExecuteDeferredSnowGBuffer();

	// Generate cloud volume textures for shadow mapping.
	if (GetStage<CVolumetricCloudsStage>()->IsStageActive(m_renderingFlags))
		GetStage<CVolumetricCloudsStage>()->ExecuteShadowGen();

	// SVOGI
	{
#if defined(FEATURE_SVO_GI)
		if (CSvoRenderer::GetInstance())
		{
			PROFILE_LABEL_SCOPE("SVOGI");

			CSvoRenderer::GetInstance()->UpdateCompute(pRenderView);
			CSvoRenderer::GetInstance()->UpdateRender(pRenderView);
		}
#endif
	}

	// Screen Space Reflections
	if (GetStage<CScreenSpaceReflectionsStage>()->IsStageActive(m_renderingFlags))
		GetStage<CScreenSpaceReflectionsStage>()->Execute();

	// Height Map AO
	if (GetStage<CHeightMapAOStage>()->IsStageActive(m_renderingFlags))
		GetStage<CHeightMapAOStage>()->Execute();

	// Screen Space Obscurance
	if (GetStage<CScreenSpaceObscuranceStage>()->IsStageActive(m_renderingFlags))
		GetStage<CScreenSpaceObscuranceStage>()->Execute();

	if (GetStage<CTiledShadingStage>()->IsStageActive(m_renderingFlags))
		GetStage<CTiledLightVolumesStage>()->Execute();

	// Water volume caustics (before m_pTiledShadingStage->Execute())
	GetStage<CWaterStage>()->ExecuteWaterVolumeCaustics();

	SetPipelineFlags(GetPipelineFlags() | EPipelineFlags::NO_SHADER_FOG);

	// Deferred shading
	{
		PROFILE_LABEL_SCOPE("DEFERRED_LIGHTING");

		GetStage<CClipVolumesStage>()->GenerateClipVolumeInfo();
		GetStage<CClipVolumesStage>()->Prepare();
		GetStage<CClipVolumesStage>()->Execute();

		if (GetStage<CTiledShadingStage>()->IsStageActive(m_renderingFlags))
		{
			GetStage<CShadowMaskStage>()->Prepare();
			GetStage<CShadowMaskStage>()->Execute();

			GetStage<CTiledShadingStage>()->Execute();

			if (GetStage<CScreenSpaceSSSStage>()->IsStageActive(m_renderingFlags))
				GetStage<CScreenSpaceSSSStage>()->Execute(CRendererResources::s_ptexSceneTargetR11G11B10F[0]);
		}
	}

	{
		PROFILE_LABEL_SCOPE("FORWARD Z");

		// Opaque forward passes
		if (GetStage<CSkyStage>())
		{
			GetStage<CSkyStage>()->Execute(CRendererResources::s_ptexHDRTarget, pZTexture);
		}
		GetStage<CSceneForwardStage>()->ExecuteOpaque();
	}

	// Deferred ocean caustics
	if (GetStage<CWaterStage>()->IsDeferredOceanCausticsEnabled())
		GetStage<CWaterStage>()->ExecuteDeferredOceanCaustics();
	{
		// Fog
		if (GetStage<CVolumetricFogStage>()->IsStageActive(m_renderingFlags))
			GetStage<CVolumetricFogStage>()->Execute();

		if (GetStage<CFogStage>()->IsStageActive(m_renderingFlags))
			GetStage<CFogStage>()->Execute();

		SetPipelineFlags(GetPipelineFlags() & ~EPipelineFlags::NO_SHADER_FOG);

		// Clouds
		if (GetStage<CVolumetricCloudsStage>()->IsStageActive(m_renderingFlags))
			GetStage<CVolumetricCloudsStage>()->Execute();

		// Water fog volumes
		GetStage<CWaterStage>()->ExecuteWaterFogVolumeBeforeTransparent();
	}

	{
		PROFILE_LABEL_SCOPE("FORWARD T");

		// Transparent (below water)
		GetStage<CSceneForwardStage>()->ExecuteTransparentBelowWater();
		// Ocean and water volumes
		GetStage<CWaterStage>()->Execute();
		// Transparent (above water)
		GetStage<CSceneForwardStage>()->ExecuteTransparentAboveWater();

		if (GetStage<CSceneForwardStage>()->IsTransparentDepthFixupEnabled())
			GetStage<CSceneForwardStage>()->ExecuteTransparentDepthFixup();

		// Half-res particles
		if (GetStage<CSceneForwardStage>()->IsTransparentLoResEnabled())
			GetStage<CSceneForwardStage>()->ExecuteTransparentLoRes(1 + crymath::clamp<int>(CRendererCVars::CV_r_ParticlesHalfResAmount, 0, 1));
	}

	// Insert fence which is used on consoles to prevent overwriting video memory
	pRenderer->InsertParticleVideoDataFence(pRenderer->GetRenderFrameID());

	if (GetStage<CSnowStage>()->IsDeferredSnowDisplacementEnabled())
		GetStage<CSnowStage>()->ExecuteDeferredSnowDisplacement();

	if (pRenderView->GetCurrentEye() == CCamera::eEye_Right ||
	    !pRenderer->GetS3DRend().IsStereoEnabled() ||
	    !pRenderer->GetS3DRend().RequiresSequentialSubmission())
	{
		GetStage<CComputeParticlesStage>()->PostDraw();
	}

	if (!(m_renderingFlags & SHDF_CUBEMAPGEN))
	{
		// HDR and LDR post-processing
		{
			// CRendererResources::s_ptexHDRTarget -> CRendererResources::s_ptexDisplayTarget (Tonemapping)
			ExecuteHDRPostProcessing();

			// CRendererResources::s_ptexDisplayTarget
			GetStage<CSceneForwardStage>()->ExecuteAfterPostProcessHDR();

			// CRendererResources::s_ptexDisplayTarget -> CRenderOutput->m_pColorTarget (PostAA)
			// Post effects disabled, copy diffuse to color target
			if (!GetStage<CPostEffectStage>()->Execute())
				m_PostToFramePass->Execute(CRendererResources::s_ptexDisplayTargetDst, pRenderView->GetRenderOutput()->GetColorTarget());

			// CRenderOutput->m_pColorTarget
			GetStage<CSceneForwardStage>()->ExecuteAfterPostProcessLDR();
		}

		if (GetStage<CSceneCustomStage>()->IsSelectionHighlightEnabled())
		{
			GetStage<CSceneCustomStage>()->ExecuteHelpers();
			GetStage<CSceneCustomStage>()->ExecuteSelectionHighlight();
		}

		if (GetStage<CSceneCustomStage>()->IsDebugOverlayEnabled())
			GetStage<CSceneCustomStage>()->ExecuteDebugOverlay();

		// Display tone mapping debugging information on the screen
		if (GetStage<CToneMappingStage>()->IsDebugInfoEnabled())
			GetStage<CToneMappingStage>()->DisplayDebugInfo();
	}
	else
	{
		// Raw HDR copy
		m_HDRToFramePass->Execute(CRendererResources::s_ptexHDRTarget, pRenderView->GetRenderOutput()->GetColorTarget());
	}

	if (GetStage<COmniCameraStage>()->IsStageActive(m_renderingFlags))
		GetStage<COmniCameraStage>()->Execute();

	if (GetStage<CVolumetricFogStage>()->IsStageActive(m_renderingFlags))
		GetStage<CVolumetricFogStage>()->ResetFrame();

	PROFILE_LABEL_POP("GRAPHICS_PIPELINE");

	m_renderPassScheduler.SetEnabled(false);
	m_renderPassScheduler.Execute(this);
}
