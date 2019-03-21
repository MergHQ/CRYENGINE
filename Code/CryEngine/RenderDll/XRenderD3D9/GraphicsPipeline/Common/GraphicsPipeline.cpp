// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GraphicsPipeline.h"
#include "Common/RendererResources.h"
#include "../PostAA.h"
#include "../ShadowMap.h"
#include "../SceneGBuffer.h"
#include "../SceneForward.h"
#include "../TiledLightVolumes.h"
#include "../ClipVolumes.h"
#include "../Fog.h"
#include "../Sky.h"
#include "../VolumetricFog.h"
#include "../DebugRenderTargets.h"

//////////////////////////////////////////////////////////////////////////
CGraphicsPipeline::CGraphicsPipeline(const IRenderer::SGraphicsPipelineDescription& desc, const std::string& uniqueIdentifier, const SGraphicsPipelineKey key)
	: m_defaultMaterialBindPoints()
	, m_defaultDrawExtraRL()
	, m_uniquePipelineIdentifierName(uniqueIdentifier)
	, m_pipelineDesc(desc)
	, m_key(key)
{
	m_renderingFlags = (EShaderRenderingFlags)desc.shaderFlags;
	m_pipelineStages.fill(nullptr);
	m_pVRProjectionManager = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CGraphicsPipeline::~CGraphicsPipeline()
{
	ShutDown();
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::ClearState()
{
	GetDeviceObjectFactory().GetCoreCommandList().Reset();
}

void CGraphicsPipeline::ClearDeviceState()
{
	GetDeviceObjectFactory().GetCoreCommandList().GetGraphicsInterface()->ClearState(false);
}

//////////////////////////////////////////////////////////////////////////

void CGraphicsPipeline::Init()
{
	// Register scene stages that make use of the global PSO cache
	RegisterStage<CShadowMapStage>();
	RegisterStage<CSceneGBufferStage>();
	RegisterStage<CSceneForwardStage>();

	// Register all other stages that don't need the global PSO cache
	RegisterStage<CTiledLightVolumesStage>();
	RegisterStage<CClipVolumesStage>();
	RegisterStage<CFogStage>();
	RegisterStage<CDebugRenderTargetsStage>();

	if ((m_pipelineDesc.shaderFlags & SHDF_ALLOW_SKY) != 0)
	{
		RegisterStage<CSkyStage>();
	}

	m_pVRProjectionManager = gcpRendD3D->m_pVRProjectionManager;
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::ShutDown()
{
	m_lightVolumeBuffer.Release();

	// destroy stages in reverse order to satisfy data dependencies
	for (auto it = m_pipelineStages.rbegin(); it != m_pipelineStages.rend(); ++it)
	{
		if (*it)
			delete *it;
	}

	m_pipelineStages.fill(nullptr);
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::Resize(int renderWidth, int renderHeight)
{
	// Sets the current render resolution on all the pipeline stages.
	for (auto it = m_pipelineStages.begin(); it != m_pipelineStages.end(); ++it)
	{
		if (*it)
			(*it)->Resize(renderWidth, renderHeight);
	}

	m_renderWidth  = renderWidth;
	m_renderHeight = renderHeight;
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::SetCurrentRenderView(CRenderView* pRenderView)
{
	m_pCurrentRenderView = pRenderView;

	// Sets the current render view on all the pipeline stages.
	for (auto it = m_pipelineStages.begin(); it != m_pipelineStages.end(); ++it)
	{
		if (*it)
			(*it)->SetRenderView(pRenderView);
	}
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::Update(EShaderRenderingFlags renderingFlags)
{
	CRendererResources::Update(renderingFlags);

	for (auto it = m_pipelineStages.begin(); it != m_pipelineStages.end(); ++it)
	{
		if (*it && (*it)->IsStageActive(renderingFlags))
			(*it)->Update();
	}
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::OnCVarsChanged(const CCVarUpdateRecorder& rCVarRecs)
{
	CRendererResources::OnCVarsChanged(rCVarRecs);

	for (auto it = m_pipelineStages.begin(); it != m_pipelineStages.end(); ++it)
	{
		if (*it)
			(*it)->OnCVarsChanged(rCVarRecs);
	}
}

//////////////////////////////////////////////////////////////////////////
std::array<SamplerStateHandle, EFSS_MAX> CGraphicsPipeline::GetDefaultMaterialSamplers() const
{
	std::array<SamplerStateHandle, EFSS_MAX> result =
	{
		{
			gcpRendD3D->m_nMaterialAnisoHighSampler,                                                                                                                                         // EFSS_ANISO_HIGH
			gcpRendD3D->m_nMaterialAnisoLowSampler,                                                                                                                                          // EFSS_ANISO_LOW
			CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Wrap, eSamplerAddressMode_Wrap, eSamplerAddressMode_Wrap, 0x0)),         // EFSS_TRILINEAR
			CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR, eSamplerAddressMode_Wrap, eSamplerAddressMode_Wrap, eSamplerAddressMode_Wrap, 0x0)),          // EFSS_BILINEAR
			CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Clamp, eSamplerAddressMode_Clamp, eSamplerAddressMode_Clamp, 0x0)),      // EFSS_TRILINEAR_CLAMP
			CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_BILINEAR, eSamplerAddressMode_Clamp, eSamplerAddressMode_Clamp, eSamplerAddressMode_Clamp, 0x0)),       // EFSS_BILINEAR_CLAMP
			gcpRendD3D->m_nMaterialAnisoSamplerBorder,                                                                                                                                       // EFSS_ANISO_HIGH_BORDER
			CDeviceObjectFactory::GetOrCreateSamplerStateHandle(SSamplerState(FILTER_TRILINEAR, eSamplerAddressMode_Border, eSamplerAddressMode_Border, eSamplerAddressMode_Border, 0x0)),   // EFSS_TRILINEAR_BORDER
		} };

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::ExecuteAnisotropicVerticalBlur(CTexture* pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly)
{
	m_AnisoVBlurPass->Execute(pTex, nAmount, fScale, fDistribution, bAlphaOnly);
}

//////////////////////////////////////////////////////////////////////////
const SRenderViewInfo& CGraphicsPipeline::GetCurrentViewInfo(CCamera::EEye eye) const
{
	const CRenderView* pRenderView = GetCurrentRenderView();
	CRY_ASSERT(pRenderView);
	if (pRenderView)
	{
		return pRenderView->GetViewInfo(eye);
	}

	static SRenderViewInfo viewInfo;
	return viewInfo;
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::ExecutePostAA()
{
	auto* pStage = GetStage<CPostAAStage>();
	CRY_ASSERT(pStage);
	pStage->Execute();
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::GeneratePerViewConstantBuffer(const SRenderViewInfo* pViewInfo, int viewInfoCount, CConstantBufferPtr pPerViewBuffer, const SRenderViewport* pCustomViewport)
{
	CRY_ASSERT(m_pCurrentRenderView);
	if (!gEnv->p3DEngine || !pPerViewBuffer)
		return;

	const SRenderViewShaderConstants& perFrameConstants = m_pCurrentRenderView->GetShaderConstants();
	CryStackAllocWithSizeVectorCleared(HLSL_PerViewGlobalConstantBuffer, viewInfoCount, bufferData, CDeviceBufferManager::AlignBufferSizeForStreaming);

	const SRenderGlobalFogDescription& globalFog = m_pCurrentRenderView->GetGlobalFog();

	for (int i = 0; i < viewInfoCount; ++i)
	{
		CRY_ASSERT(pViewInfo[i].pCamera);

		const SRenderViewInfo& viewInfo = pViewInfo[i];

		HLSL_PerViewGlobalConstantBuffer& cb = bufferData[i];

		const float animTime = GetAnimationTime().GetSeconds();
		const bool bReverseDepth = (viewInfo.flags & SRenderViewInfo::eFlags_ReverseDepth) != 0;

		cb.CV_HPosScale = viewInfo.downscaleFactor;

		SRenderViewport viewport = pCustomViewport ? *pCustomViewport : viewInfo.viewport;
		cb.CV_ScreenSize = Vec4(float(viewport.width),
		                        float(viewport.height),
		                        0.5f / (viewport.width / viewInfo.downscaleFactor.x),
		                        0.5f / (viewport.height / viewInfo.downscaleFactor.y));

		cb.CV_ViewProjZeroMatr = viewInfo.cameraProjZeroMatrix.GetTransposed();
		cb.CV_ViewProjMatr = viewInfo.cameraProjMatrix.GetTransposed();
		cb.CV_ViewProjNearestMatr = viewInfo.cameraProjNearestMatrix.GetTransposed();
		cb.CV_InvViewProj = viewInfo.invCameraProjMatrix.GetTransposed();
		cb.CV_PrevViewProjMatr = viewInfo.prevCameraProjMatrix.GetTransposed();
		cb.CV_PrevViewProjNearestMatr = viewInfo.prevCameraProjNearestMatrix.GetTransposed();
		cb.CV_ViewMatr = viewInfo.viewMatrix.GetTransposed();
		cb.CV_InvViewMatr = viewInfo.invViewMatrix.GetTransposed();

		Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
		CShadowUtils::ProjectScreenToWorldExpansionBasis(Matrix44(IDENTITY), *viewInfo.pCamera, m_pCurrentRenderView->m_vProjMatrixSubPixoffset,
		                                                 float(viewport.width), float(viewport.height), vWBasisX, vWBasisY, vWBasisZ, vCamPos, true);

		cb.CV_ScreenToWorldBasis.SetColumn(0, Vec3r(vWBasisX));
		cb.CV_ScreenToWorldBasis.SetColumn(1, Vec3r(vWBasisY));
		cb.CV_ScreenToWorldBasis.SetColumn(2, Vec3r(vWBasisZ));
		cb.CV_ScreenToWorldBasis.SetColumn(3, viewInfo.cameraOrigin);

		cb.CV_SunLightDir = Vec4(perFrameConstants.pSunDirection, 1.0f);
		cb.CV_SunColor = Vec4(perFrameConstants.pSunColor, perFrameConstants.sunSpecularMultiplier);
		cb.CV_SkyColor = Vec4(perFrameConstants.pSkyColor, 1.0f);
		cb.CV_FogColor = Vec4(globalFog.bEnable ? globalFog.color.toVec3() : Vec3(0.f, 0.f, 0.f), perFrameConstants.pVolumetricFogParams.z);
		cb.CV_TerrainInfo = Vec4(gEnv->p3DEngine->GetTerrainTextureMultiplier(), 0, 0, 0);

		cb.CV_AnimGenParams = Vec4(animTime * 2.0f, animTime * 0.25f, animTime * 1.0f, animTime * 0.125f);

		Vec3 pDecalZFightingRemedy;
		{
			const float* mProj = viewInfo.projMatrix.GetData();
			const float s = clamp_tpl(CRendererCVars::CV_r_ZFightingDepthScale, 0.1f, 1.0f);

			pDecalZFightingRemedy.x = s;                                      // scaling factor to pull decal in front
			pDecalZFightingRemedy.y = (float)((1.0f - s) * mProj[4 * 3 + 2]); // correction factor for homogeneous z after scaling is applied to xyzw { = ( 1 - v[0] ) * zMappingRageBias }
			pDecalZFightingRemedy.z = clamp_tpl(CRendererCVars::CV_r_ZFightingExtrude, 0.0f, 1.0f);

			// alternative way the might save a bit precision
			//PF.pDecalZFightingRemedy.x = s; // scaling factor to pull decal in front
			//PF.pDecalZFightingRemedy.y = (float)((1.0f - s) * mProj[4*2+2]);
			//PF.pDecalZFightingRemedy.z = clamp_tpl(CRendererCVars::CV_r_ZFightingExtrude, 0.0f, 1.0f);
		}
		cb.CV_DecalZFightingRemedy = Vec4(pDecalZFightingRemedy, 0);

		cb.CV_CamRightVector = Vec4(viewInfo.cameraVX.GetNormalized(), 0);
		cb.CV_CamFrontVector = Vec4(viewInfo.cameraVZ.GetNormalized(), 0);
		cb.CV_CamUpVector = Vec4(viewInfo.cameraVY.GetNormalized(), 0);
		cb.CV_WorldViewPosition = Vec4(viewInfo.cameraOrigin, 0);

		// CV_NearFarClipDist
		{
			// Note: CV_NearFarClipDist.z is used to put the weapon's depth range into correct relation to the whole scene
			// when generating the depth texture in the z pass (_RT_NEAREST)
			cb.CV_NearFarClipDist = Vec4(
				viewInfo.nearClipPlane,
				viewInfo.farClipPlane,
				viewInfo.farClipPlane / gEnv->p3DEngine->GetMaxViewDistance(),
				1.0f / viewInfo.farClipPlane);
		}

		// CV_ProjRatio
		{
			float zn = viewInfo.nearClipPlane;
			float zf = viewInfo.farClipPlane;
			float hfov = viewInfo.pCamera->GetHorizontalFov();
			cb.CV_ProjRatio.x = bReverseDepth ? zn / (zn - zf) : zf / (zf - zn);
			cb.CV_ProjRatio.y = bReverseDepth ? zn / (zf - zn) : zn / (zn - zf);
			cb.CV_ProjRatio.z = 1.0f / hfov;
			cb.CV_ProjRatio.w = 1.0f;
		}

		// CV_NearestScaled
		{
			float zn = viewInfo.nearClipPlane;
			float zf = viewInfo.farClipPlane;
			float nearZRange = CRendererCVars::CV_r_DrawNearZRange;
			cb.CV_NearestScaled.x = bReverseDepth ? 1.0f - zf / (zf - zn) * nearZRange : zf / (zf - zn) * nearZRange;
			cb.CV_NearestScaled.y = bReverseDepth ? zn / (zf - zn) * nearZRange * nearZRange : zn / (zn - zf) * nearZRange * nearZRange;
			cb.CV_NearestScaled.z = bReverseDepth ? 1.0f - (nearZRange - 0.001f) : nearZRange - 0.001f;
			cb.CV_NearestScaled.w = 1.0f;
		}

		// CV_TessInfo
		{
			// We want to obtain the edge length in pixels specified by CV_r_tessellationtrianglesize
			// Therefore the tess factor would depend on the viewport size and CV_r_tessellationtrianglesize
			static const ICVar* pCV_e_TessellationMaxDistance(gEnv->pConsole->GetCVar("e_TessellationMaxDistance"));
			assert(pCV_e_TessellationMaxDistance);

			const float hfov = viewInfo.pCamera->GetHorizontalFov();
			cb.CV_TessInfo.x = sqrtf(float(viewport.width * viewport.height)) / (hfov * CRendererCVars::CV_r_tessellationtrianglesize);
			cb.CV_TessInfo.y = CRendererCVars::CV_r_displacementfactor;
			cb.CV_TessInfo.z = pCV_e_TessellationMaxDistance->GetFVal();
			cb.CV_TessInfo.w = (float)CRendererCVars::CV_r_ParticlesTessellationTriSize;
		}

		cb.CV_FrustumPlaneEquation.SetRow4(0, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_RIGHT]);
		cb.CV_FrustumPlaneEquation.SetRow4(1, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_LEFT]);
		cb.CV_FrustumPlaneEquation.SetRow4(2, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_TOP]);
		cb.CV_FrustumPlaneEquation.SetRow4(3, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_BOTTOM]);

		if (gRenDev->m_pCurWindGrid)
		{
			float fSizeWH = (float)gRenDev->m_pCurWindGrid->m_nWidth * gRenDev->m_pCurWindGrid->m_fCellSize * 0.5f;
			float fSizeHH = (float)gRenDev->m_pCurWindGrid->m_nHeight * gRenDev->m_pCurWindGrid->m_fCellSize * 0.5f;
			cb.CV_WindGridOffset = Vec4(gRenDev->m_pCurWindGrid->m_vCentr.x - fSizeWH, gRenDev->m_pCurWindGrid->m_vCentr.y - fSizeHH, 1.0f / (float)gRenDev->m_pCurWindGrid->m_nWidth, 1.0f / (float)gRenDev->m_pCurWindGrid->m_nHeight);
		}
	}

	pPerViewBuffer->UpdateBuffer(&bufferData[0], sizeof(HLSL_PerViewGlobalConstantBuffer), 0, viewInfoCount);
}

//////////////////////////////////////////////////////////////////////////
void CGraphicsPipeline::ApplyShaderQuality(CDeviceGraphicsPSODesc& psoDesc, const SShaderProfile& shaderProfile)
{
	const uint64 quality = g_HWSR_MaskBit[HWSR_QUALITY];
	const uint64 quality1 = g_HWSR_MaskBit[HWSR_QUALITY1];

	psoDesc.m_ShaderFlags_RT &= ~(quality | quality1);
	switch (psoDesc.m_ShaderQuality = shaderProfile.GetShaderQuality())
	{
	case eSQ_Medium:
		psoDesc.m_ShaderFlags_RT |= quality;
		break;
	case eSQ_High:
		psoDesc.m_ShaderFlags_RT |= quality1;
		break;
	case eSQ_VeryHigh:
		psoDesc.m_ShaderFlags_RT |= (quality | quality1);
		break;
	}

}