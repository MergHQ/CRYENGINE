// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SceneGBuffer.h"
#include "D3DPostProcess.h"

#include "Common/Include_HLSL_CPP_Shared.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/TypedConstantBuffer.h"
#include "Common/ReverseDepth.h"

#include "Common/RenderView.h"
#include "CompiledRenderObject.h"
#include "Common/SceneRenderPass.h"

#include "GraphicsPipeline/TiledShading.h"
#include "GraphicsPipeline/SceneDepth.h"

CSceneGBufferStage::CSceneGBufferStage()
	: m_perPassResources()
{}

void CSceneGBufferStage::Init()
{
	m_pPerPassResourceSet = GetDeviceObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	CRY_VERIFY(SetAndBuildPerPassResources(true));

	// Create resource layout
	m_pResourceLayout = gcpRendD3D->GetGraphicsPipeline().CreateScenePassLayout(m_perPassResources);

	// Freeze resource-set layout (assert will fire when violating the constraint)
	m_perPassResources.AcceptChangedBindPoints();

	// Depth Pre-pass
	m_depthPrepass.SetLabel("ZPREPASS");
	m_depthPrepass.SetFlags(CSceneRenderPass::ePassFlags_ReverseDepth | CSceneRenderPass::ePassFlags_RenderNearest | CSceneRenderPass::ePassFlags_VrProjectionPass);
	m_depthPrepass.SetPassResources(m_pResourceLayout, m_pPerPassResourceSet);

	// Opaque Pass
	m_opaquePass.SetLabel("OPAQUE");
	m_opaquePass.SetFlags(CSceneRenderPass::ePassFlags_ReverseDepth | CSceneRenderPass::ePassFlags_RenderNearest | CSceneRenderPass::ePassFlags_VrProjectionPass);
	m_opaquePass.SetPassResources(m_pResourceLayout, m_pPerPassResourceSet);

	// Opaque with Velocity Pass
	m_opaqueVelocityPass.SetLabel("OPAQUE_VELOCITY");
	m_opaqueVelocityPass.SetFlags(CSceneRenderPass::ePassFlags_ReverseDepth | CSceneRenderPass::ePassFlags_RenderNearest | CSceneRenderPass::ePassFlags_VrProjectionPass);
	m_opaqueVelocityPass.SetPassResources(m_pResourceLayout, m_pPerPassResourceSet);

	// Overlay Pass
	m_overlayPass.SetLabel("OVERLAYS");
	m_overlayPass.SetFlags(CSceneRenderPass::ePassFlags_ReverseDepth);
	m_overlayPass.SetPassResources(m_pResourceLayout, m_pPerPassResourceSet);

	// Micro GBuffer Pass
	m_microGBufferPass.SetLabel("MICRO");
	m_microGBufferPass.SetFlags(CSceneRenderPass::ePassFlags_ReverseDepth);
	m_microGBufferPass.SetPassResources(m_pResourceLayout, m_pPerPassResourceSet);
}

bool CSceneGBufferStage::CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO)
{
	outPSO = NULL;

	CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout, desc);
	if (!GetStdGraphicsPipeline().FillCommonScenePassStates(desc, psoDesc))
		return true;

	CShader* pShader = static_cast<CShader*>(desc.shaderItem.m_pShader);
	CShaderResources* pRes = static_cast<CShaderResources*>(desc.shaderItem.m_pShaderResources);
	if (pRes->IsTransparent() && !(pShader->m_Flags2 & EF2_HAIR))
		return true;
	
	const uint64 objectFlags = desc.objectFlags;
	CSceneRenderPass* pSceneRenderPass = (passID == ePass_DepthBufferFill) ? &m_depthPrepass : &m_opaquePass;

	// Enable stencil for vis area and dynamic object markup
	psoDesc.m_RenderState |= GS_STENCIL;
	psoDesc.m_StencilState =
		STENC_FUNC(FSS_STENCFUNC_ALWAYS)
		| STENCOP_FAIL(FSS_STENCOP_KEEP)
		| STENCOP_ZFAIL(FSS_STENCOP_KEEP)
		| STENCOP_PASS(FSS_STENCOP_REPLACE);
	psoDesc.m_StencilReadMask = 0xFF & (~BIT_STENCIL_RESERVED);
	psoDesc.m_StencilWriteMask = 0xFF;

	if (passID == ePass_DepthBufferFill)
	{
		// No blending or depth-write omission allowed in ZPrepass (depth test omission is okay)
		CRY_ASSERT(!(psoDesc.m_RenderState & GS_BLEND_MASK) || !(psoDesc.m_RenderState & GS_DEPTHWRITE));
	}
	else
	{
		if (objectFlags & FOB_HAS_PREVMATRIX)
		{
			psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
			if ((objectFlags & FOB_MOTION_BLUR) && !(objectFlags & FOB_SKINNED))
			{
				psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_VERTEX_VELOCITY];
			}

			pSceneRenderPass = &m_opaqueVelocityPass;
		}

		if ((objectFlags & FOB_DECAL) || (objectFlags & FOB_TERRAIN_LAYER) || (((CShader*)desc.shaderItem.m_pShader)->GetFlags() & EF_DECAL))
		{
			// Shouldn't this be EQUAL for DECAL and LEQUAL for TERRAIN?
			psoDesc.m_RenderState = (psoDesc.m_RenderState & ~(GS_BLEND_MASK | GS_DEPTHWRITE | GS_DEPTHFUNC_MASK));
			psoDesc.m_RenderState |= GS_DEPTHFUNC_LEQUAL | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NOCOLMASK_GBUFFER_OVERLAY;

			psoDesc.m_StencilReadMask = BIT_STENCIL_ALLOW_TERRAINLAYERBLEND;
			psoDesc.m_StencilWriteMask = 0;
			psoDesc.m_StencilState =
				STENC_FUNC(FSS_STENCFUNC_EQUAL)
				| STENCOP_FAIL(FSS_STENCOP_KEEP)
				| STENCOP_ZFAIL(FSS_STENCOP_KEEP)
				| STENCOP_PASS(FSS_STENCOP_KEEP);		
				
			psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND];

			if (objectFlags & FOB_DECAL_TEXGEN_2D)
				psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];

			pSceneRenderPass = &m_overlayPass;
		}

		// Disable alpha testing/depth writes if object renders using a z-prepass and requested technique is not z prepass
		else if (objectFlags & FOB_ZPREPASS)
		{
			if (passID == ePass_AttrGBufferFill)
			{
				psoDesc.m_RenderState &= ~(GS_STENCIL | GS_DEPTHWRITE | GS_DEPTHFUNC_MASK | GS_ALPHATEST);
				psoDesc.m_RenderState |= GS_DEPTHFUNC_EQUAL;
				psoDesc.m_ShaderFlags_RT &= ~(g_HWSR_MaskBit[HWSR_ALPHATEST] | g_HWSR_MaskBit[HWSR_DISSOLVE]);
			}
		}
	}

	if (passID == ePass_MicroGBufferFill)
	{
		pSceneRenderPass = &m_microGBufferPass;
		psoDesc.m_ShaderFlags_RT &= ~(g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1]);
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
		psoDesc.m_ShaderFlags_RT &= ~g_HWSR_MaskBit[HWSR_MOTION_BLUR];
	}

	psoDesc.m_RenderState = ReverseDepthHelper::ConvertDepthFunc(psoDesc.m_RenderState);
	psoDesc.m_pRenderPass = pSceneRenderPass->GetRenderPass();

	if (!psoDesc.m_pRenderPass)
		return false;

	outPSO = GetDeviceObjectFactory().CreateGraphicsPSO(psoDesc);
	return outPSO != nullptr;
}

bool CSceneGBufferStage::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	DevicePipelineStatesArray& stageStates = pStateArray[m_stageID];

	if (pStateCache->Find(stateDesc, stageStates))
		return true;

	bool bFullyCompiled = true;

	SGraphicsPipelineStateDescription _stateDesc = stateDesc;

	_stateDesc.technique = TTYPE_Z;
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_FullGBufferFill , stageStates[ePass_FullGBufferFill ]);
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_MicroGBufferFill, stageStates[ePass_MicroGBufferFill]);
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_AttrGBufferFill , stageStates[ePass_AttrGBufferFill ]);

	_stateDesc.technique = TTYPE_ZPREPASS;
	bFullyCompiled &= CreatePipelineState(_stateDesc, ePass_DepthBufferFill , stageStates[ePass_DepthBufferFill ]);

	if (bFullyCompiled)
	{
		pStateCache->Put(stateDesc, stageStates);
	}

	return bFullyCompiled;
}

bool CSceneGBufferStage::SetAndBuildPerPassResources(bool bOnInit)
{
	CD3D9Renderer* rd = gcpRendD3D;

	// samplers
	{
		auto materialSamplers = gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialSamplers();
		for (int i = 0; i < materialSamplers.size(); ++i)
		{
			m_perPassResources.SetSampler(EEfResSamplers(i), materialSamplers[i], EShaderStage_AllWithoutCompute);
		}
		// hard-coded point samplers
		m_perPassResources.SetSampler(8, EDefaultSamplerStates::PointWrap, EShaderStage_AllWithoutCompute);
		m_perPassResources.SetSampler(9, EDefaultSamplerStates::PointClamp, EShaderStage_AllWithoutCompute);
	}

	// textures
	{
		int nTerrainTex0 = 0, nTerrainTex1 = 0, nTerrainTex2 = 0;
		if (gEnv->p3DEngine && gEnv->p3DEngine->GetITerrain())
			gEnv->p3DEngine->GetITerrain()->GetAtlasTexId(nTerrainTex0, nTerrainTex1, nTerrainTex2);

		m_perPassResources.SetTexture(ePerPassTexture_PerlinNoiseMap, CRendererResources::s_ptexPerlinNoiseMap, EDefaultResourceViews::Default, EShaderStage_AllWithoutCompute);
		m_perPassResources.SetTexture(ePerPassTexture_WindGrid, CRendererResources::s_ptexWindGrid, EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_perPassResources.SetTexture(ePerPassTexture_TerrainElevMap, CTexture::GetByID(nTerrainTex2), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_perPassResources.SetTexture(ePerPassTexture_TerrainNormMap, CTexture::GetByID(nTerrainTex1), EDefaultResourceViews::Default, EShaderStage_Pixel);
		m_perPassResources.SetTexture(ePerPassTexture_TerrainBaseMap, CTexture::GetByID(nTerrainTex0), EDefaultResourceViews::sRGB, EShaderStage_Pixel);
		m_perPassResources.SetTexture(ePerPassTexture_NormalsFitting, CRendererResources::s_ptexNormalsFitting, EDefaultResourceViews::Default, EShaderStage_Pixel);
		m_perPassResources.SetTexture(ePerPassTexture_SceneLinearDepth, CRendererResources::s_ptexLinearDepth, EDefaultResourceViews::Default, EShaderStage_Pixel);
	}

	// constant buffers
	{
		CConstantBufferPtr pPerViewCB;

		// Handle case when no view is available in the initialization of the g-buffer-stage
		if (bOnInit)
			pPerViewCB = CDeviceBufferManager::GetNullConstantBuffer();
		else
			pPerViewCB = rd->GetGraphicsPipeline().GetMainViewConstantBuffer();

		m_perPassResources.SetConstantBuffer(eConstantBufferShaderSlot_PerView, pPerViewCB, EShaderStage_AllWithoutCompute);
	}

	if (bOnInit)
		return true;

	CRY_ASSERT(!m_perPassResources.HasChangedBindPoints()); // Cannot change resource layout after init. It is baked into the shaders
	return m_pPerPassResourceSet->Update(m_perPassResources);
}

void CSceneGBufferStage::Update()
{
	CRenderView* pRenderView = RenderView();
	const SRenderViewport& viewport = pRenderView->GetViewport();

	CTexture* pZTexture = pRenderView->GetDepthTarget();

	CStandardGraphicsPipeline* p = static_cast<CStandardGraphicsPipeline*>(&GetGraphicsPipeline());
	EShaderRenderingFlags flags = (EShaderRenderingFlags)p->GetRenderFlags();
	const bool isForwardMinimal = (flags & SHDF_FORWARD_MINIMAL) != 0;

	SetAndBuildPerPassResources(false);

	{
		// Depth pre-pass
		m_depthPrepass.SetViewport(viewport);
		m_depthPrepass.SetRenderTargets(
			// Depth
			pZTexture,
			// Color 0
			NULL
		);
	}

	if (!isForwardMinimal)
	{
		CTexture* pSceneSpecular = CRendererResources::s_ptexSceneSpecular;
#if defined(DURANGO_USE_ESRAM)
		pSceneSpecular = CRendererResources::s_ptexSceneSpecularESRAM;
#endif

		// Opaque Pass
		m_opaquePass.SetViewport(viewport);
		m_opaquePass.SetRenderTargets(
			// Depth
			pZTexture,
			// Color 0
			CRendererResources::s_ptexSceneNormalsMap,
			// Color 1
			CRendererResources::s_ptexSceneDiffuse,
			// Color 2
			pSceneSpecular
		);

		// Opaque with Velocity Pass
		m_opaqueVelocityPass.SetViewport(viewport);
		m_opaqueVelocityPass.SetRenderTargets(
			// Depth
			pZTexture,
			// Color 0
			CRendererResources::s_ptexSceneNormalsMap,
			// Color 1
			CRendererResources::s_ptexSceneDiffuse,
			// Color 2
			pSceneSpecular,
			// Color 3
			CRendererResources::s_ptexVelocityObjects[0]
		);

		// Overlay Pass
		m_overlayPass.SetViewport(viewport);
		m_overlayPass.SetRenderTargets(
			// Depth
			pZTexture,
			// Color 0
			CRendererResources::s_ptexSceneNormalsMap,
			// Color 1
			CRendererResources::s_ptexSceneDiffuse,
			// Color 2
			pSceneSpecular
		);

		// Micro GBuffer Pass
		m_microGBufferPass.SetViewport(viewport);
		m_microGBufferPass.SetRenderTargets(
			// Depth
			pZTexture,
			// Color 0
			CRendererResources::s_ptexSceneNormalsMap
		);
	}
}

void CSceneGBufferStage::ExecuteDepthPrepass()
{
	CRenderView* pRenderView = RenderView();
	
	m_depthPrepass.BeginExecution();
	m_depthPrepass.SetupDrawContext(m_stageID, ePass_DepthBufferFill, TTYPE_ZPREPASS, FB_ZPREPASS);
	m_depthPrepass.DrawRenderItems(pRenderView, EFSLIST_ZPREPASS_NEAREST);
	m_depthPrepass.DrawRenderItems(pRenderView, EFSLIST_ZPREPASS);
	m_depthPrepass.EndExecution();
}

void CSceneGBufferStage::ExecuteSceneOpaque()
{
	CRenderView* pRenderView = RenderView();

	/* The FOB-flag hierarchy for splitting and sorting is:
	 *
	 * FOB_HAS_PREVMATRIX not set          ...0
	 *   FOB_ZPREPASS not set              ...0
	 *     FOB_ALPHATEST not set           ...0
	 *     FOB_ALPHATEST set               ...
	 *   FOB_ZPREPASS set                  ...NoZPre
	 *     FOB_ALPHATEST not set           ...NoZPre
	 *     FOB_ALPHATEST set               ...
	 * FOB_HAS_PREVMATRIX set              ...Velocity
	 *   FOB_ZPREPASS not set              ...Velocity
	 *     FOB_ALPHATEST not set           ...Velocity
	 *     FOB_ALPHATEST set               ...
	 *   FOB_ZPREPASS set                  ...VelocityNoZPre
	 *     FOB_ALPHATEST not set           ...VelocityNoZPre
	 *     FOB_ALPHATEST set               ...
	 *                                     ...Num
	 */
	CRY_ASSERT_MESSAGE(CRenderer::CV_r_ZPassDepthSorting == 1, "RendItem sorting has been overwritten and are not sorted by ObjFlags, this function can't be used!");;

	static_assert(FOB_SORT_MASK & FOB_HAS_PREVMATRIX, "FOB's HAS_PREVMATRIX must be a sort criteria");
	static_assert(FOB_SORT_MASK & FOB_ZPREPASS, "FOB's ZPREPASS must be a sort criteria");
	static_assert((FOB_ZPREPASS << 1) == FOB_HAS_PREVMATRIX, "FOB's ZPREPASS must be the next lower bit of HAS_PREVMATRIX");
	static_assert((((~0ULL) & FOB_SORT_MASK) & ~FOB_HAS_PREVMATRIX) < FOB_HAS_PREVMATRIX, "FOB's HAS_PREVMATRIX must be the most significant FOB-flag");

	CMotionBlurPredicate mbPredicate;
	CZPrePassPredicate zpPredicate;

	int nearestNum            = pRenderView->GetRenderItems(EFSLIST_NEAREST_OBJECTS).size();
 	int nearestVelocity       = pRenderView->FindRenderListSplit(mbPredicate, EFSLIST_NEAREST_OBJECTS, 0, nearestNum);
	int nearestNoZPre         = pRenderView->FindRenderListSplit(zpPredicate, EFSLIST_NEAREST_OBJECTS, 0, nearestVelocity);
	int nearestVelocityNoZPre = pRenderView->FindRenderListSplit(zpPredicate, EFSLIST_NEAREST_OBJECTS, nearestVelocity, nearestNum);

	int generalNum            = pRenderView->GetRenderItems(EFSLIST_GENERAL).size();
	int generalVelocity       = pRenderView->FindRenderListSplit(mbPredicate, EFSLIST_GENERAL, 0, generalNum);
	int generalNoZPre         = pRenderView->FindRenderListSplit(zpPredicate, EFSLIST_GENERAL, 0, generalVelocity);
	int generalVelocityNoZPre = pRenderView->FindRenderListSplit(zpPredicate, EFSLIST_GENERAL, generalVelocity, generalNum);

	/* NOTE:
	 *
	 * Instead of:
	 * - depth write m_opaquePass
	 * - depth test  m_opaquePass
	 * - depth write m_opaqueVelocityPass
	 * - depth test  m_opaqueVelocityPass
	 *
	 * We actually want:
	 * - depth write m_opaquePass
	 * - depth write m_opaqueVelocityPass
	 * - depth test  m_opaquePass
	 * - depth test  m_opaqueVelocityPass
	 *
	 * Because we can then start downloading and trans-coding the depth-buffer before the draw-loop finishes.
	 */

	{
		// Opaque
		m_opaquePass.BeginExecution();

		// [~FOB_HAS_PREVMATRIX & ~FOB_ZPREPASS]
		m_opaquePass.SetupDrawContext(m_stageID, ePass_FullGBufferFill, TTYPE_Z, FB_Z, FB_ZPREPASS);
		m_opaquePass.DrawRenderItems(pRenderView, EFSLIST_NEAREST_OBJECTS, 0, nearestNoZPre);
		m_opaquePass.DrawRenderItems(pRenderView, EFSLIST_GENERAL, 0, generalNoZPre);

		// [~FOB_HAS_PREVMATRIX & FOB_ZPREPASS]
		m_opaquePass.SetupDrawContext(m_stageID, ePass_AttrGBufferFill, TTYPE_Z, FB_Z | FB_ZPREPASS);
		m_opaquePass.DrawRenderItems(pRenderView, EFSLIST_NEAREST_OBJECTS, nearestNoZPre, nearestVelocity);
		m_opaquePass.DrawRenderItems(pRenderView, EFSLIST_GENERAL, generalNoZPre, generalVelocity);

		m_opaquePass.EndExecution();
	}

	{
		// OpaqueVelocity
		m_opaqueVelocityPass.BeginExecution();

		// [FOB_HAS_PREVMATRIX & ~FOB_ZPREPASS]
		m_opaqueVelocityPass.SetupDrawContext(m_stageID, ePass_FullGBufferFill, TTYPE_Z, FB_Z, FB_ZPREPASS);
		m_opaqueVelocityPass.DrawRenderItems(pRenderView, EFSLIST_NEAREST_OBJECTS, nearestVelocity, nearestVelocityNoZPre);
		m_opaqueVelocityPass.DrawRenderItems(pRenderView, EFSLIST_GENERAL, generalVelocity, generalVelocityNoZPre);

		// [FOB_HAS_PREVMATRIX & FOB_ZPREPASS]
		m_opaqueVelocityPass.SetupDrawContext(m_stageID, ePass_AttrGBufferFill, TTYPE_Z, FB_Z | FB_ZPREPASS);
		m_opaqueVelocityPass.DrawRenderItems(pRenderView, EFSLIST_NEAREST_OBJECTS, nearestVelocityNoZPre, nearestNum);
		m_opaqueVelocityPass.DrawRenderItems(pRenderView, EFSLIST_GENERAL, generalVelocityNoZPre, generalNum);

		m_opaqueVelocityPass.EndExecution();
	}
}

void CSceneGBufferStage::ExecuteSceneOverlays()
{
	CRenderView* pRenderView = RenderView();

	{
		m_overlayPass.BeginExecution();
		m_overlayPass.SetupDrawContext(m_stageID, ePass_AttrGBufferFill, TTYPE_Z, FB_Z);
		m_overlayPass.DrawRenderItems(pRenderView, EFSLIST_TERRAINLAYER);
		m_overlayPass.DrawRenderItems(pRenderView, EFSLIST_DECAL);
		m_overlayPass.EndExecution();
	}
}

void CSceneGBufferStage::ExecuteGBufferVisualization()
{
	FUNCTION_PROFILER_RENDERER();
	PROFILE_LABEL_SCOPE("GBUFFER_VISUALIZATION");

	static CCryNameTSCRC tech("DebugGBuffer");

	m_passBufferVisualization.SetTechnique(CShaderMan::s_shDeferredShading, tech, 0);
	m_passBufferVisualization.SetRenderTarget(0, CRendererResources::s_ptexSceneDiffuseTmp);
	m_passBufferVisualization.SetPrimitiveType(CRenderPrimitive::ePrim_ProceduralTriangle);
	m_passBufferVisualization.SetState(GS_NODEPTHTEST);

	m_passBufferVisualization.SetTexture(0, CRendererResources::s_ptexLinearDepth);
	m_passBufferVisualization.SetTexture(1, CRendererResources::s_ptexSceneNormalsMap);
	m_passBufferVisualization.SetTexture(2, CRendererResources::s_ptexSceneDiffuse);
	m_passBufferVisualization.SetTexture(3, CRendererResources::s_ptexSceneSpecular);
	m_passBufferVisualization.SetSampler(0, EDefaultSamplerStates::PointClamp);

	m_passBufferVisualization.BeginConstantUpdate();
	static CCryNameR paramName("DebugViewMode");
	m_passBufferVisualization.SetConstant(paramName, Vec4((float)CRenderer::CV_r_DeferredShadingDebugGBuffer, 0, 0, 0), eHWSC_Pixel);
			
	m_passBufferVisualization.Execute();
}

void CSceneGBufferStage::ExecuteMicroGBuffer()
{
	FUNCTION_PROFILER_RENDERER();
	PROFILE_LABEL_SCOPE("GBUFFER_MICRO");

	auto& RESTRICT_REFERENCE commandList = GetDeviceObjectFactory().GetCoreCommandList();

	CRenderView* pRenderView = RenderView();
	auto& rendItemDrawer = pRenderView->GetDrawer();

	CClearSurfacePass::Execute(RenderView()->GetDepthTarget(), CLEAR_ZBUFFER | CLEAR_STENCIL, Clr_FarPlane_Rev.r, STENCIL_VALUE_OUTDOORS);

	m_microGBufferPass.PrepareRenderPassForUse(commandList);
	
	rendItemDrawer.InitDrawSubmission();

	m_microGBufferPass.BeginExecution();
	m_microGBufferPass.SetupDrawContext(m_stageID, ePass_MicroGBufferFill, TTYPE_Z, FB_Z);
	m_microGBufferPass.DrawRenderItems(pRenderView, EFSLIST_NEAREST_OBJECTS);
	m_microGBufferPass.DrawRenderItems(pRenderView, EFSLIST_GENERAL);
	m_microGBufferPass.DrawRenderItems(pRenderView, EFSLIST_TERRAINLAYER);
	m_microGBufferPass.DrawRenderItems(pRenderView, EFSLIST_DECAL);
	m_microGBufferPass.EndExecution();

	rendItemDrawer.JobifyDrawSubmission();
}

void CSceneGBufferStage::Execute()
{
	FUNCTION_PROFILER_RENDERER();
	PROFILE_LABEL_SCOPE("GBUFFER");

	// NOTE: no more external state changes in here, everything should have been setup
	CRenderView* pRenderView = RenderView();
	auto& rendItemDrawer = pRenderView->GetDrawer();

	if (CRenderer::CV_r_DeferredShadingTiled < CTiledShadingStage::eDeferredMode_Disabled)
	{
		bool bClearAll = CRenderer::CV_r_wireframe != 0 || CRendererCVars::CV_r_DeferredShadingDebugGBuffer > 0;

		if (CVrProjectionManager::IsMultiResEnabledStatic())
			bClearAll = true;

		if (bClearAll)
		{
			CClearSurfacePass::Execute(CRendererResources::s_ptexSceneNormalsMap, Clr_Transparent);
			CClearSurfacePass::Execute(CRendererResources::s_ptexSceneDiffuse, Clr_Empty);
			CClearSurfacePass::Execute(CRendererResources::s_ptexSceneSpecular, Clr_Empty);
		}

		// Clear velocity target
		CClearSurfacePass::Execute(GetUtils().GetVelocityObjectRT(pRenderView), Clr_Transparent);
	}

	auto& RESTRICT_REFERENCE commandList = GetDeviceObjectFactory().GetCoreCommandList();

	// Prepare ========================================================================
	if (CRendererCVars::CV_r_UseZPass >= eZPassMode_PartialZPrePass)
	{
		m_depthPrepass.PrepareRenderPassForUse(commandList);
	}

	if (CRenderer::CV_r_DeferredShadingTiled < CTiledShadingStage::eDeferredMode_Disabled)
	{
		// Stereo has separate velocity targets for left and right eye
		m_opaqueVelocityPass.ExchangeRenderTarget(3, GetUtils().GetVelocityObjectRT(pRenderView));

		m_opaquePass.PrepareRenderPassForUse(commandList);
		m_opaqueVelocityPass.PrepareRenderPassForUse(commandList);
		m_overlayPass.PrepareRenderPassForUse(commandList);
	}

	// Execute ========================================================================
	rendItemDrawer.InitDrawSubmission();

	if (CRendererCVars::CV_r_UseZPass >= eZPassMode_PartialZPrePass)
	{
		ExecuteDepthPrepass();

		if (CRendererCVars::CV_r_UseZPass == eZPassMode_FullZPrePass)
		{
			rendItemDrawer.JobifyDrawSubmission();
			rendItemDrawer.WaitForDrawSubmission();

			// Function call is guaranteed to be side-effect-free
			GetStdGraphicsPipeline().GetSceneDepthStage()->Execute();

			rendItemDrawer.InitDrawSubmission();
		}
	}

	if (CRenderer::CV_r_DeferredShadingTiled < CTiledShadingStage::eDeferredMode_Disabled)
	{
		ExecuteSceneOpaque();

		if (CRendererCVars::CV_r_UseZPass != eZPassMode_FullZPrePass)
		{
			rendItemDrawer.JobifyDrawSubmission();
			rendItemDrawer.WaitForDrawSubmission();
			
			// Function call is guaranteed to be side-effect-free
			GetStdGraphicsPipeline().GetSceneDepthStage()->Execute();

			rendItemDrawer.InitDrawSubmission();
		}

		// Needs depth for soft depth test
		ExecuteSceneOverlays();
	}

	rendItemDrawer.JobifyDrawSubmission();
}

void CSceneGBufferStage::ExecuteMinimumZpass()
{
	FUNCTION_PROFILER_RENDERER();
	PROFILE_LABEL_SCOPE("MINIMUM_ZPASS");

	// NOTE: no more external state changes in here, everything should have been setup

	CRenderView* pRenderView = RenderView();
	auto& rendItemDrawer = pRenderView->GetDrawer();

	if (CRendererCVars::CV_r_UseZPass >= eZPassMode_PartialZPrePass)
	{
		auto& RESTRICT_REFERENCE commandList = GetDeviceObjectFactory().GetCoreCommandList();

		m_depthPrepass.PrepareRenderPassForUse(commandList);

		rendItemDrawer.InitDrawSubmission();

		ExecuteDepthPrepass();

		rendItemDrawer.JobifyDrawSubmission();
		rendItemDrawer.WaitForDrawSubmission();
	}
}
