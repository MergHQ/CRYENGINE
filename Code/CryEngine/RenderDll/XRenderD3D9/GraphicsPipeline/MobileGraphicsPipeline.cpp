// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MobileGraphicsPipeline.h"

#include "ShadowMap.h"
#include "SceneGBuffer.h"
#include "SceneForward.h"
#include "SceneCustom.h"
#include "SceneDepth.h"
#include "ComputeSkinning.h"
#include "Sky.h"
#include "GpuParticles.h"
#include "ClipVolumes.h"
#include "TiledShading.h"
#include "TiledLightVolumes.h"
#include "MobileComposition.h"

#include "Common/TypedConstantBuffer.h"
#include "Common/Textures/TextureHelpers.h"
#include "Common/Include_HLSL_CPP_Shared.h"
#include "../Common/RenderView.h"

#include "Common/RenderDisplayContext.h"

#include "../D3DPostProcess.h"
#include "Common/ReverseDepth.h"
#include "D3D_SVO.h"

CMobileGraphicsPipeline::CMobileGraphicsPipeline(const IRenderer::SGraphicsPipelineDescription& desc, const std::string& uniqueIdentifier, const SGraphicsPipelineKey key)
	: CGraphicsPipeline(desc, uniqueIdentifier, key)
	, m_changedCVars(gEnv->pConsole)
{}

void CMobileGraphicsPipeline::Init()
{
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
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat, CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetConstantBuffer(eConstantBufferShaderSlot_PerGroup, CDeviceBufferManager::GetNullConstantBuffer(), EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Hull);

		// Deliberately aliasing slots/use-cases here for visibility (e.g. EReservedTextureSlot_ComputeSkinVerts, EReservedTextureSlot_SkinExtraWeights and
		// EReservedTextureSlot_GpuParticleStream). The resource layout will just pick the first.
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_SkinExtraWeights, CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_ComputeSkinVerts, CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_GpuParticleStream, CDeviceBufferManager::GetNullBufferStructured(), EDefaultResourceViews::Default, EShaderStage_Vertex);
		m_defaultDrawExtraRL.SetBuffer(EReservedTextureSlot_AdjacencyInfo, CDeviceBufferManager::GetNullBufferTyped(), EDefaultResourceViews::Default, EShaderStage_Domain);

		m_pDefaultDrawExtraRS = GetDeviceObjectFactory().CreateResourceSet();
		m_pDefaultDrawExtraRS->Update(m_defaultDrawExtraRL);
	}

	// per view constant buffer
	m_mainViewConstantBuffer.CreateDeviceBuffer();

	// Register all other stages that don't need the global PSO cache
	RegisterStage<CSceneCustomStage>();
	RegisterStage<CTiledShadingStage>();
	RegisterStage<CMobileCompositionStage>();
	RegisterStage<CSceneDepthStage>();

	// Register and initialize all common stages
	CGraphicsPipeline::Init();

	// Out-of-pipeline passes for display
	m_HDRToFramePass.reset(new CStretchRectPass(this));
	m_PostToFramePass.reset(new CStretchRectPass(this));
	m_FrameToFramePass.reset(new CStretchRectPass(this));

	m_LZSubResPass[0].reset(new CDepthDownsamplePass(this));
	m_LZSubResPass[1].reset(new CDepthDownsamplePass(this));
	m_LZSubResPass[2].reset(new CDepthDownsamplePass(this));
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
void CMobileGraphicsPipeline::Resize(int renderWidth, int renderHeight)
{
	CGraphicsPipeline::Resize(renderWidth, renderHeight);
}

//////////////////////////////////////////////////////////////////////////
void CMobileGraphicsPipeline::ShutDown()
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

	m_LZSubResPass[0].reset();
	m_LZSubResPass[1].reset();
	m_LZSubResPass[2].reset();
	m_HQSubResPass[0].reset();
	m_HQSubResPass[1].reset();
	m_LQSubResPass[0].reset();
	m_LQSubResPass[1].reset();

	m_ResolvePass.reset();
	m_DownscalePass.reset();
	m_UpscalePass.reset();
}

//////////////////////////////////////////////////////////////////////////
void CMobileGraphicsPipeline::Update(EShaderRenderingFlags renderingFlags)
{
	FUNCTION_PROFILER_RENDERER();

	m_numInvalidDrawcalls = 0;
	GenerateMainViewConstantBuffer();

	m_renderingFlags = renderingFlags;
	CGraphicsPipeline::Update(renderingFlags);

	if (!m_changedCVars.GetCVars().empty())
	{
		CGraphicsPipeline::OnCVarsChanged(m_changedCVars);

		m_changedCVars.Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CMobileGraphicsPipeline::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, SGraphicsPipelineStateDescription stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
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

bool CMobileGraphicsPipeline::FillCommonScenePassStates(const SGraphicsPipelineStateDescription& inputDesc, CDeviceGraphicsPSODesc& psoDesc)
{
	CShader* pShader = static_cast<CShader*>(inputDesc.shaderItem.m_pShader);
	SShaderTechnique* pTechnique = pShader->GetTechnique(inputDesc.shaderItem.m_nTechnique, inputDesc.technique, true);
	if (!pTechnique)
		return false;

	CShaderResources* pRes = static_cast<CShaderResources*>(inputDesc.shaderItem.m_pShaderResources);
	const uint64 objectFlags = inputDesc.objectFlags;
	SShaderPass* pShaderPass = &pTechnique->m_Passes[0];

	// Handle quality flags
	CMobileGraphicsPipeline::ApplyShaderQuality(psoDesc, gcpRendD3D->GetShaderProfile(pShader->m_eShaderType));

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

CDeviceResourceLayoutPtr CMobileGraphicsPipeline::CreateScenePassLayout(const CDeviceResourceSetDesc& perPassResources)
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

void CMobileGraphicsPipeline::Execute()
{
	FUNCTION_PROFILER_RENDERER();

	CD3D9Renderer* pRenderer = gcpRendD3D;
	CRenderView* pRenderView = GetCurrentRenderView();
	CTexture* pZTexture = pRenderView->GetDepthTarget();

	if (CRendererCVars::CV_r_GraphicsPipelineMobile == 2)
		GetStage<CSceneGBufferStage>()->Execute();
	else
		GetStage<CSceneGBufferStage>()->ExecuteMicroGBuffer();

	pRenderView->GetDrawer().WaitForDrawSubmission();

	// Deferred shading
	{
		PROFILE_LABEL_SCOPE("DEFERRED_LIGHTING");

		{
			GetStage<CClipVolumesStage>()->GenerateClipVolumeInfo();
			GetStage<CTiledLightVolumesStage>()->Execute();
			GetStage<CTiledShadingStage>()->Execute();
			GetStage<CMobileCompositionStage>()->ExecuteDeferredLighting();
		}
	}

	// Opaque and transparent forward passes
	if (GetStage<CSkyStage>())
	{
		GetStage<CSkyStage>()->Execute(CRendererResources::s_ptexHDRTarget, pZTexture);
	}
	GetStage<CSceneForwardStage>()->ExecuteMobile();

	// Insert fence which is used on consoles to prevent overwriting video memory
	pRenderer->InsertParticleVideoDataFence(pRenderer->GetRenderFrameID());

	GetStage<CMobileCompositionStage>()->ExecutePostProcessing();

	pRenderer->m_pPostProcessMgr->End(pRenderView);
}
