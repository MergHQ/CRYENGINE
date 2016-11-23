// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "StandardGraphicsPipeline.h"

#include "ShadowMap.h"
#include "SceneGBuffer.h"
#include "SceneForward.h"
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
#include "ShadowMask.h"
#include "TiledShading.h"
#include "ColorGrading.h"
#include "WaterRipples.h"
#include "LensOptics.h"
#include "Common/TypedConstantBuffer.h"
#include "Common/Textures/TextureHelpers.h"
#include "Common/Include_HLSL_CPP_Shared.h"
#include "../Common/RenderView.h"

#include "DriverD3D.h" //for gcpRendD3D

#include "../D3DPostProcess.h"
#include "Common/ReverseDepth.h"
#include "D3D_SVO.h"

//////////////////////////////////////////////////////////////////////////
CStandardGraphicsPipeline::SViewInfo::SViewInfo()
	: pRenderCamera(nullptr)
	, pCamera(nullptr)
	, pFrustumPlanes(nullptr)
	, cameraProjZeroMatrix(IDENTITY)
	, cameraProjMatrix(IDENTITY)
	, cameraProjNearestMatrix(IDENTITY)
	, projMatrix(IDENTITY)
	, viewMatrix(IDENTITY)
	, invCameraProjMatrix(IDENTITY)
	, invViewMatrix(IDENTITY)
	, prevCameraMatrix(IDENTITY)
	, prevCameraProjMatrix(IDENTITY)
	, prevCameraProjNearestMatrix(IDENTITY)
	, downscaleFactor(1)
	, flags(eFlags_None)
{
}

void CStandardGraphicsPipeline::SViewInfo::SetCamera(const CRenderCamera& renderCam, const CCamera& cam, const CCamera& previousCam, Vec2 subpixelShift, float drawNearestFov, float drawNearestFarPlane, Plane obliqueClippingPlane)
{
	pCamera = &cam;
	pRenderCamera = &renderCam;

	Matrix44A proj, nearestProj;
	{
		const CRenderCamera& cam = renderCam;

		if (flags & eFlags_ReverseDepth) mathMatrixPerspectiveOffCenterReverseDepth((Matrix44A*)&proj, cam.fWL, cam.fWR, cam.fWB, cam.fWT, cam.fNear, cam.fFar);
		else                             mathMatrixPerspectiveOffCenter((Matrix44A*)&proj, cam.fWL, cam.fWR, cam.fWB, cam.fWT, cam.fNear, cam.fFar);

		if (flags & eFlags_ObliqueFrustumClipping)
		{
			Matrix44A mObliqueProjMatrix(IDENTITY);
			mObliqueProjMatrix.m02 = obliqueClippingPlane.n[0];
			mObliqueProjMatrix.m12 = obliqueClippingPlane.n[1];
			mObliqueProjMatrix.m22 = obliqueClippingPlane.n[2];
			mObliqueProjMatrix.m32 = obliqueClippingPlane.d;

			proj = proj * mObliqueProjMatrix;
		}

		if (flags & eFlags_SubpixelShift)
		{
			proj.m20 += subpixelShift.x;
			proj.m21 += subpixelShift.y;
		}

		nearestProj = GetNearestProjection(drawNearestFov, drawNearestFarPlane, subpixelShift);
	}

	Matrix44 view, viewZero, invView;
	ExtractViewMatrices(cam, view, viewZero, invView);

	Matrix44 prevView, prevViewZero, prevInvView;
	ExtractViewMatrices(previousCam, prevView, prevViewZero, prevInvView);

	cameraProjMatrix = view * proj;
	cameraProjZeroMatrix = viewZero * proj;
	projMatrix = proj;
	viewMatrix = view;
	invViewMatrix = invView;
	cameraProjNearestMatrix = viewZero * nearestProj;

	prevCameraMatrix = prevView;
	prevCameraProjMatrix = prevView * proj;
	prevCameraProjNearestMatrix = prevView;
	prevCameraProjNearestMatrix.SetRow(3, Vec3(ZERO));
	prevCameraProjNearestMatrix = prevCameraProjNearestMatrix * nearestProj;

	Matrix44_tpl<f64> mProjInv;
	if (mathMatrixPerspectiveFovInverse(&mProjInv, &proj))
	{
		Matrix44_tpl<f64> mViewInv;
		mathMatrixLookAtInverse(&mViewInv, &view);
		invCameraProjMatrix = mProjInv * mViewInv;
	}
	else
	{
		invCameraProjMatrix = cameraProjMatrix.GetInverted();
	}
}

void CStandardGraphicsPipeline::SViewInfo::SetLegacyCamera(CD3D9Renderer* pRenderer, const Matrix44& previousFrameCameraMatrix)
{
	SRenderPipeline& RESTRICT_REFERENCE rp = pRenderer->m_RP;

	pCamera = &pRenderer->GetCamera();
	pRenderCamera = &pRenderer->GetRCamera();

	Matrix44 nearestProj = GetNearestProjection(pRenderer->GetDrawNearestFOV(), pRenderer->CV_r_DrawNearFarPlane, pRenderer->m_vProjMatrixSubPixoffset);

	cameraProjZeroMatrix = pRenderer->m_CameraProjZeroMatrix;
	cameraProjMatrix = pRenderer->m_CameraProjMatrix;
	projMatrix = pRenderer->m_ProjMatrix;
	viewMatrix = pRenderer->m_ViewMatrix;
	Matrix44_tpl<f64> invView64;
	mathMatrixLookAtInverse(&invView64, &viewMatrix);
	invViewMatrix = invView64;
	invCameraProjMatrix = pRenderer->m_InvCameraProjMatrix;
	cameraProjNearestMatrix = pRenderer->m_CameraZeroMatrix * nearestProj;

	prevCameraMatrix = previousFrameCameraMatrix;
	prevCameraProjMatrix = previousFrameCameraMatrix * pRenderer->m_ProjMatrix;
	prevCameraProjNearestMatrix = previousFrameCameraMatrix;
	prevCameraProjNearestMatrix.SetRow(3, Vec3(ZERO));
	prevCameraProjNearestMatrix = prevCameraProjNearestMatrix * nearestProj;
}

//////////////////////////////////////////////////////////////////////////

void CStandardGraphicsPipeline::SViewInfo::ExtractViewMatrices(const CCamera& cam, Matrix44& view, Matrix44& viewZero, Matrix44& invView) const
{
	Matrix34_tpl<f64> mCam34 = cam.GetMatrix();
	mCam34.OrthonormalizeFast();

	Matrix44_tpl<f64> mCam44T = mCam34.GetTransposed();

	// Rotate around x-axis by -PI/2
	Vec4_tpl<f64> row1 = mCam44T.GetRow4(1);
	mCam44T.SetRow4(1, mCam44T.GetRow4(2));
	mCam44T.SetRow4(2, Vec4_tpl<f64>(-row1.x, -row1.y, -row1.z, -row1.w));

	if (flags & eFlags_MirrorCamera)
	{
		Vec4_tpl<f64> _row1 = mCam44T.GetRow4(1);
		mCam44T.SetRow4(1, Vec4_tpl<f64>(-_row1.x, -_row1.y, -_row1.z, -_row1.w));
	}
	invView = (Matrix44_tpl<f32> )mCam44T;

	Matrix44_tpl<f64> mView64;
	mathMatrixLookAtInverse(&mView64, &mCam44T);
	view = (Matrix44_tpl<f32> )mView64;

	viewZero = view;
	viewZero.SetRow(3, Vec3(ZERO));
}

//////////////////////////////////////////////////////////////////////////

Matrix44 CStandardGraphicsPipeline::SViewInfo::GetNearestProjection(float nearestFOV, float farPlane, Vec2 subpixelShift)
{
	CRY_ASSERT(pCamera);

	Matrix44A result;

	float fFov = pCamera->GetFov();
	if (nearestFOV > 1.0f && nearestFOV < 179.0f)
		fFov = DEG2RAD(nearestFOV);

	float fNearRatio = DRAW_NEAREST_MIN / pCamera->GetNearPlane();
	float wT = tanf(fFov * 0.5f) * DRAW_NEAREST_MIN, wB = -wT;
	float wR = wT * pCamera->GetProjRatio(), wL = -wR;

	wL += pCamera->GetAsymL() * fNearRatio;
	wR += pCamera->GetAsymR() * fNearRatio;
	wB += pCamera->GetAsymB() * fNearRatio, wT += pCamera->GetAsymT() * fNearRatio;

	if (flags & eFlags_ReverseDepth) mathMatrixPerspectiveOffCenterReverseDepth(&result, wL, wR, wB, wT, DRAW_NEAREST_MIN, farPlane);
	else                             mathMatrixPerspectiveOffCenter(&result, wL, wR, wB, wT, DRAW_NEAREST_MIN, farPlane);

	if (flags & eFlags_SubpixelShift)
	{
		result.m20 += subpixelShift.x;
		result.m21 += subpixelShift.y;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
CRenderView* CGraphicsPipelineStage::RenderView()
{
	return gcpRendD3D->GetGraphicsPipeline().GetCurrentRenderView();
}

CStandardGraphicsPipeline::CStandardGraphicsPipeline()
	: m_changedCVars(gEnv->pConsole)
{}

void CStandardGraphicsPipeline::Init()
{
	// default material resources
	{
		m_pDefaultMaterialResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet();
		m_pDefaultMaterialResources->SetConstantBuffer(eConstantBufferShaderSlot_PerMaterial, CDeviceBufferManager::CreateNullConstantBuffer(), EShaderStage_AllWithoutCompute);

		for (EEfResTextures texType = EFTT_DIFFUSE; texType < EFTT_MAX; texType = EEfResTextures(texType + 1))
		{
			m_pDefaultMaterialResources->SetTexture(texType, CTexture::s_pTexNULL, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
		}
	}

	// default extra per instance
	{
		EShaderStage shaderStages = EShaderStage_Vertex | EShaderStage_Hull | EShaderStage_Domain;
		CGpuBuffer nullBuffer;
		nullBuffer.Create(0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DX11BUF_NULL_RESOURCE | DX11BUF_BIND_SRV, nullptr);
		m_pDefaultInstanceExtraResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet();
		m_pDefaultInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat, CDeviceBufferManager::CreateNullConstantBuffer(), shaderStages);
		m_pDefaultInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, CDeviceBufferManager::CreateNullConstantBuffer(), shaderStages);
		m_pDefaultInstanceExtraResources->SetBuffer(EReservedTextureSlot_SkinExtraWeights, nullBuffer, false, shaderStages);
		m_pDefaultInstanceExtraResources->SetBuffer(EReservedTextureSlot_AdjacencyInfo, nullBuffer, false, shaderStages);    // shares shader slot with EReservedTextureSlot_PatchID
		m_pDefaultInstanceExtraResources->SetBuffer(EReservedTextureSlot_ComputeSkinVerts, nullBuffer, false, shaderStages); // shares shader slot with EReservedTextureSlot_PatchID
		m_pDefaultInstanceExtraResources->Build();                                                                           // This needs to be a valid resource-set since it's shared by all CompiledRenderObject that don't need a unique instance
	}

	// per view constant buffer
	m_pPerViewConstantBuffer = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(sizeof(HLSL_PerViewGlobalConstantBuffer));

	// Register scene stages that make use of the global PSO cache
	RegisterSceneStage<CShadowMapStage, eStage_ShadowMap>(m_pShadowMapStage);
	RegisterSceneStage<CSceneGBufferStage, eStage_SceneGBuffer>(m_pSceneGBufferStage);
	RegisterSceneStage<CSceneForwardStage, eStage_SceneForward>(m_pSceneForwardStage);
	RegisterSceneStage<CSceneCustomStage, eStage_SceneCustom>(m_pSceneCustomStage);

	// Register all other stages that don't need the global PSO cache
	RegisterStage<CHeightMapAOStage>(m_pHeightMapAOStage, eStage_HeightMapAO);
	RegisterStage<CScreenSpaceObscuranceStage>(m_pScreenSpaceObscuranceStage, eStage_ScreenSpaceObscurance);
	RegisterStage<CScreenSpaceReflectionsStage>(m_pScreenSpaceReflectionsStage, eStage_ScreenSpaceReflections);
	RegisterStage<CScreenSpaceSSSStage>(m_pScreenSpaceSSSStage, eStage_ScreenSpaceSSS);
	RegisterStage<CVolumetricFogStage>(m_pVolumetricFogStage, eStage_VolumetricFog);
	RegisterStage<CFogStage>(m_pFogStage, eStage_Fog);
	RegisterStage<CVolumetricCloudsStage>(m_pVolumetricCloudsStage, eStage_VolumetricClouds);
	RegisterStage<CWaterStage>(m_pWaterStage, eStage_Water);
	RegisterStage<CWaterRipplesStage>(m_pWaterRipplesStage, eStage_WaterRipples);
	RegisterStage<CMotionBlurStage>(m_pMotionBlurStage, eStage_MotionBlur);
	RegisterStage<CDepthOfFieldStage>(m_pDepthOfFieldStage, eStage_DepthOfField);
	RegisterStage<CAutoExposureStage>(m_pAutoExposureStage, eStage_AutoExposure);
	RegisterStage<CBloomStage>(m_pBloomStage, eStage_Bloom);
	RegisterStage<CToneMappingStage>(m_pToneMappingStage, eStage_ToneMapping);
	RegisterStage<CSunShaftsStage>(m_pSunShaftsStage, eStage_Sunshafts);
	RegisterStage<CPostAAStage>(m_pPostAAStage, eStage_PostAA);
	RegisterStage<CComputeSkinningStage>(m_pComputeSkinningStage, eStage_ComputeSkinning);
	RegisterStage<CGpuParticlesStage>(m_pGpuParticlesStage, eStage_GpuParticles);
	RegisterStage<CClipVolumesStage>(m_pClipVolumesStage, eStage_ClipVolumes);
	RegisterStage<CShadowMaskStage>(m_pShadowMaskStage, eStage_ShadowMask);
	RegisterStage<CTiledShadingStage>(m_pTiledShadingStage, eStage_TiledShading);
	RegisterStage<CColorGradingStage>(m_pColorGradingStage, eStage_ColorGrading);
	RegisterStage<CLensOpticsStage>(m_pLensOpticsStage, eStage_LensOptics);
}

void CStandardGraphicsPipeline::Prepare(CRenderView* pRenderView, EShaderRenderingFlags renderingFlags)
{
	m_pCurrentRenderView = pRenderView;
	m_renderingFlags = renderingFlags;
	m_numInvalidDrawcalls = 0;

	if (!m_changedCVars.GetCVars().empty())
	{
		for (int i = 0; i < eStage_Count; ++i)
			m_pipelineStages[i]->OnCVarsChanged(m_changedCVars);

		m_changedCVars.Reset();
	}
}

bool CStandardGraphicsPipeline::CreatePipelineStates(DevicePipelineStatesArray* pStateArray, SGraphicsPipelineStateDescription stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	// NOTE: Please update SDeviceObjectHelpers::CheckTessellationSupport when adding new techniques types here.

	bool bFullyCompiled = true;

	// GBuffer
	{
		stateDesc.technique = TTYPE_Z;
		bFullyCompiled &= m_pSceneGBufferStage->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	// ShadowMap
	{
		stateDesc.technique = TTYPE_SHADOWGEN;
		bFullyCompiled &= m_pShadowMapStage->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	// Forward
	{
		stateDesc.technique = TTYPE_GENERAL;
		bFullyCompiled &= m_pSceneForwardStage->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	// Custom
	{
		stateDesc.technique = TTYPE_DEBUG;
		bFullyCompiled &= m_pSceneCustomStage->CreatePipelineStates(pStateArray, stateDesc, pStateCache);
	}

	return bFullyCompiled;
}

void CStandardGraphicsPipeline::ApplyShaderQuality(CDeviceGraphicsPSODesc& psoDesc, const SShaderProfile& shaderProfile)
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

int CStandardGraphicsPipeline::GetViewInfoCount() const
{
	int viewInfoCount = 0;

	if (m_pCurrentRenderView) // called from within CStandardGraphicsPipeline::Execute() scope => use renderview cameras
	{
		for (CCamera::EEye eye = CCamera::eEye_Left; eye != CCamera::eEye_eCount; eye = CCamera::EEye(eye + 1))
		{
			uint32 renderingFlags = m_renderingFlags;

			if ((renderingFlags & (SHDF_STEREO_LEFT_EYE | SHDF_STEREO_RIGHT_EYE)) == 0) // non-stereo case
				renderingFlags |= SHDF_STEREO_LEFT_EYE;

			uint32 currentEyeFlag = eye == CCamera::eEye_Left ? SHDF_STEREO_LEFT_EYE : SHDF_STEREO_RIGHT_EYE;

			if (renderingFlags & currentEyeFlag)
			{
				++viewInfoCount;
			}
		}
	}
	else
	{
		++viewInfoCount;
	}

	return viewInfoCount;
}

int CStandardGraphicsPipeline::GetViewInfo(CStandardGraphicsPipeline::SViewInfo viewInfo[2], const D3DViewPort* pCustomViewport)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;

	SViewInfo::eFlags viewFlags = SViewInfo::eFlags_None;
	if (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) viewFlags |= SViewInfo::eFlags_ReverseDepth;
	if (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL)    viewFlags |= SViewInfo::eFlags_MirrorCull;
	if (!(rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE) && !(rp.m_PersFlags2 & RBPF2_NOPOSTAA))
		viewFlags |= SViewInfo::eFlags_SubpixelShift;

	SViewport viewport = pRenderer->GetViewport();

	if (pCustomViewport)
	{
		viewport.nX = int(pCustomViewport->TopLeftX);
		viewport.nY = int(pCustomViewport->TopLeftY);
		viewport.nWidth = int(pCustomViewport->Width);
		viewport.nHeight = int(pCustomViewport->Height);
	}

	int viewInfoCount = 0;

	if (m_pCurrentRenderView) // called from within CStandardGraphicsPipeline::Execute() scope => use renderview cameras
	{
		for (CCamera::EEye eye = CCamera::eEye_Left; eye != CCamera::eEye_eCount; eye = CCamera::EEye(eye + 1))
		{
			uint32 renderingFlags = m_renderingFlags;

			if ((renderingFlags & (SHDF_STEREO_LEFT_EYE | SHDF_STEREO_RIGHT_EYE)) == 0) // non-stereo case
				renderingFlags |= SHDF_STEREO_LEFT_EYE;

			uint32 currentEyeFlag = eye == CCamera::eEye_Left ? SHDF_STEREO_LEFT_EYE : SHDF_STEREO_RIGHT_EYE;

			if (renderingFlags & currentEyeFlag)
			{
				auto& cam = m_pCurrentRenderView->GetCamera(eye);
				auto& previousCam = m_pCurrentRenderView->GetPreviousCamera(eye);
				auto& rcam = m_pCurrentRenderView->GetRenderCamera(eye);

				viewInfo[viewInfoCount].flags = viewFlags;
				viewInfo[viewInfoCount].SetCamera(rcam, cam, previousCam, pRenderer->m_vProjMatrixSubPixoffset,
				                                  pRenderer->GetDrawNearestFOV(), pRenderer->CV_r_DrawNearFarPlane, rp.m_TI[rp.m_nProcessThreadID].m_pObliqueClipPlane);
				viewInfo[viewInfoCount].viewport = viewport;
				viewInfo[viewInfoCount].pFrustumPlanes = cam.GetFrustumPlane(0);
				viewInfo[viewInfoCount].downscaleFactor = Vec4(rp.m_CurDownscaleFactor.x, rp.m_CurDownscaleFactor.y, pRenderer->m_PrevViewportScale.x, pRenderer->m_PrevViewportScale.y);

				++viewInfoCount;
			}
		}
	}
	else
	{
		viewInfo[viewInfoCount].flags = viewFlags;
		viewInfo[viewInfoCount].SetLegacyCamera(pRenderer, pRenderer->GetPreviousFrameCameraMatrix());
		viewInfo[viewInfoCount].viewport = viewport;
		viewInfo[viewInfoCount].downscaleFactor = Vec4(rp.m_CurDownscaleFactor.x, rp.m_CurDownscaleFactor.y, pRenderer->m_PrevViewportScale.x, pRenderer->m_PrevViewportScale.y);
		viewInfo[viewInfoCount].pFrustumPlanes = pRenderer->GetCamera().GetFrustumPlane(0);

		++viewInfoCount;
	}

	return viewInfoCount;
}

void CStandardGraphicsPipeline::UpdatePerViewConstantBuffer(const D3DViewPort* pCustomViewport)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;

	SViewInfo viewInfo[2];
	int viewInfoCount = GetViewInfo(viewInfo, pCustomViewport);
	UpdatePerViewConstantBuffer(viewInfo, viewInfoCount, m_pPerViewConstantBuffer);
}

void CStandardGraphicsPipeline::UpdatePerViewConstantBuffer(const SViewInfo* pViewInfo, int viewInfoCount, CConstantBufferPtr& pPerViewBuffer)
{
	if (!gEnv->p3DEngine || !pPerViewBuffer)
		return;

	CD3D9Renderer* pRenderer = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;
	const SCGParamsPF& perFrameConstants = pRenderer->m_cEF.m_PF[rp.m_nProcessThreadID];
	CryStackAllocWithSizeVectorCleared(HLSL_PerViewGlobalConstantBuffer, viewInfoCount, bufferData, CDeviceBufferManager::AlignBufferSizeForStreaming);

	for (int i = 0; i < viewInfoCount; ++i)
	{
		CRY_ASSERT(pViewInfo[i].pCamera && pViewInfo[i].pRenderCamera);

		const SViewInfo& viewInfo = pViewInfo[i];
		HLSL_PerViewGlobalConstantBuffer& cb = bufferData[i];

		const float time = rp.m_TI[rp.m_nProcessThreadID].m_RealTime;
		const bool bReverseDepth = (viewInfo.flags & CStandardGraphicsPipeline::SViewInfo::eFlags_ReverseDepth) != 0;

		cb.CV_HPosScale = viewInfo.downscaleFactor;
		cb.CV_ScreenSize = Vec4(float(viewInfo.viewport.nWidth),
		                        float(viewInfo.viewport.nHeight),
		                        0.5f / (viewInfo.viewport.nWidth / viewInfo.downscaleFactor.x),
		                        0.5f / (viewInfo.viewport.nHeight / viewInfo.downscaleFactor.y));

		cb.CV_ViewProjZeroMatr = viewInfo.cameraProjZeroMatrix.GetTransposed();
		cb.CV_ViewProjMatr = viewInfo.cameraProjMatrix.GetTransposed();
		cb.CV_ViewProjNearestMatr = viewInfo.cameraProjNearestMatrix.GetTransposed();
		cb.CV_InvViewProj = viewInfo.invCameraProjMatrix.GetTransposed();
		cb.CV_PrevViewProjMatr = viewInfo.prevCameraProjMatrix.GetTransposed();
		cb.CV_PrevViewProjNearestMatr = viewInfo.prevCameraProjNearestMatrix.GetTransposed();
		cb.CV_ViewMatr = viewInfo.viewMatrix.GetTransposed();
		cb.CV_InvViewMatr = viewInfo.invViewMatrix.GetTransposed();

		Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
		CShadowUtils::ProjectScreenToWorldExpansionBasis(Matrix44(IDENTITY), *viewInfo.pCamera, pRenderer->m_vProjMatrixSubPixoffset,
		                                                 float(viewInfo.viewport.nWidth), float(viewInfo.viewport.nHeight), vWBasisX, vWBasisY, vWBasisZ, vCamPos, true, NULL);

		cb.CV_ScreenToWorldBasis.SetColumn(0, Vec3r(vWBasisX));
		cb.CV_ScreenToWorldBasis.SetColumn(1, Vec3r(vWBasisY));
		cb.CV_ScreenToWorldBasis.SetColumn(2, Vec3r(vWBasisZ));
		cb.CV_ScreenToWorldBasis.SetColumn(3, viewInfo.pRenderCamera->vOrigin);

		cb.CV_SunLightDir = Vec4(perFrameConstants.pSunDirection, 1.0f);
		cb.CV_SunColor = Vec4(perFrameConstants.pSunColor, perFrameConstants.sunSpecularMultiplier);
		cb.CV_SkyColor = Vec4(perFrameConstants.pSkyColor, 1.0f);
		cb.CV_FogColor = Vec4(rp.m_TI[rp.m_nProcessThreadID].m_FS.m_CurColor.toVec3(), perFrameConstants.pVolumetricFogParams.z);
		cb.CV_TerrainInfo = Vec4(gEnv->p3DEngine->GetTerrainTextureMultiplier(), 0, 0, 0);

		cb.CV_AnimGenParams = Vec4(time * 2.0f, time * 0.25f, time * 1.0f, time * 0.125f);

		cb.CV_DecalZFightingRemedy = Vec4(perFrameConstants.pDecalZFightingRemedy, 0);

		cb.CV_CamFrontVector = Vec4(viewInfo.pRenderCamera->vZ.GetNormalized(), 0);
		cb.CV_CamUpVector = Vec4(viewInfo.pRenderCamera->vY.GetNormalized(), 0);

		// CV_NearFarClipDist
		{
			// Note: CV_NearFarClipDist.z is used to put the weapon's depth range into correct relation to the whole scene
			// when generating the depth texture in the z pass (_RT_NEAREST)
			cb.CV_NearFarClipDist = Vec4(viewInfo.pRenderCamera->fNear,
			                             viewInfo.pRenderCamera->fFar,
			                             viewInfo.pRenderCamera->fFar / gEnv->p3DEngine->GetMaxViewDistance(),
			                             1.0f / viewInfo.pRenderCamera->fFar);
		}

		// CV_ProjRatio
		{
			float zn = viewInfo.pRenderCamera->fNear;
			float zf = viewInfo.pRenderCamera->fFar;
			float hfov = viewInfo.pCamera->GetHorizontalFov();
			cb.CV_ProjRatio.x = bReverseDepth ? zn / (zn - zf) : zf / (zf - zn);
			cb.CV_ProjRatio.y = bReverseDepth ? zn / (zf - zn) : zn / (zn - zf);
			cb.CV_ProjRatio.z = 1.0f / hfov;
			cb.CV_ProjRatio.w = 1.0f;
		}

		// CV_NearestScaled
		{
			float zn = viewInfo.pRenderCamera->fNear;
			float zf = viewInfo.pRenderCamera->fFar;
			float nearZRange = pRenderer->CV_r_DrawNearZRange;
			float camScale = pRenderer->CV_r_DrawNearFarPlane / gEnv->p3DEngine->GetMaxViewDistance();
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
			cb.CV_TessInfo.x = sqrtf(float(viewInfo.viewport.nWidth * viewInfo.viewport.nHeight)) / (hfov * pRenderer->CV_r_tessellationtrianglesize);
			cb.CV_TessInfo.y = pRenderer->CV_r_displacementfactor;
			cb.CV_TessInfo.z = pCV_e_TessellationMaxDistance->GetFVal();
			cb.CV_TessInfo.w = (float)CRenderer::CV_r_ParticlesTessellationTriSize;
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

bool CStandardGraphicsPipeline::FillCommonScenePassStates(const SGraphicsPipelineStateDescription& inputDesc, CDeviceGraphicsPSODesc& psoDesc)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	CShader* pShader = static_cast<CShader*>(inputDesc.shaderItem.m_pShader);
	SShaderTechnique* pTechnique = pShader->GetTechnique(inputDesc.shaderItem.m_nTechnique, inputDesc.technique, true);
	if (!pTechnique)
		return false;

	CShaderResources* pRes = static_cast<CShaderResources*>(inputDesc.shaderItem.m_pShaderResources);
	const uint64 objectFlags = inputDesc.objectFlags;
	SShaderPass* pShaderPass = &pTechnique->m_Passes[0];

	// Handle quality flags
	CStandardGraphicsPipeline::ApplyShaderQuality(psoDesc, pRenderer->GetShaderProfile(pShader->m_eShaderType));

	SThreadInfo* const pShaderThreadInfo = &(pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID]);
	if (pShaderThreadInfo->m_PersFlags & RBPF_REVERSE_DEPTH)
		psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];

	// Set resource states
	bool bTwoSided = false;
	{
		if (pRes->m_ResFlags & MTL_FLAG_2SIDED)
		{
			bTwoSided = true;
		}

		if (pRes->IsAlphaTested())
		{
			psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
		}

		if (pRes->m_Textures[EFTT_DIFFUSE] && pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_pTexModifier)
		{
			psoDesc.m_ShaderFlags_MD |= pRes->m_Textures[EFTT_DIFFUSE]->m_Ext.m_nUpdateFlags;
		}

		if (pRes->m_pDeformInfo)
			psoDesc.m_ShaderFlags_MDV |= pRes->m_pDeformInfo->m_eType;
	}

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

	psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_PER_INSTANCE_CB_TEMP];

	psoDesc.m_CullMode = bTwoSided ? eCULL_None : (pShaderPass->m_eCull != -1 ? (ECull)pShaderPass->m_eCull : eCULL_Back);
	psoDesc.m_PrimitiveType = ERenderPrimitiveType(inputDesc.primitiveType);

	if (psoDesc.m_bAllowTesselation)
	{
		psoDesc.m_PrimitiveType = ept3ControlPointPatchList;
		psoDesc.m_ObjectStreamMask |= VSM_NORMALS;
	}

	psoDesc.m_ShaderFlags_RT |= CVrProjectionManager::Instance()->GetRTFlags();

	return true;
}

void CStandardGraphicsPipeline::SwitchToLegacyPipeline()
{
	CD3D9Renderer* rd = gcpRendD3D;

	rd->m_nCurStateRS = (uint32) - 1;
	rd->m_nCurStateBL = (uint32) - 1;
	rd->m_nCurStateDP = (uint32) - 1;
	rd->ResetToDefault();
	rd->RT_UnbindResources();
	rd->FX_SetState(0, 0, 0xFFFFFFFF);
	rd->D3DSetCull(eCULL_Back);

	rd->m_CurTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	rd->GetDeviceContext().ResetCachedState();

	CTexture::ResetTMUs();
	CHWShader::s_pCurPS = nullptr;
	CHWShader::s_pCurVS = nullptr;
	CHWShader::s_pCurGS = nullptr;
	CHWShader::s_pCurDS = nullptr;
	CHWShader::s_pCurHS = nullptr;
	CHWShader::s_pCurCS = nullptr;
	CHWShader_D3D::s_nActivationFailMask = 0;

	CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->Reset();

	// backup viewport we were about to set
	SViewport newViewport = rd->m_NewViewport;

	// Restore engine's render target stack by pushing dummy texture
	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTargetScaled[0], NULL);
	rd->FX_PopRenderTarget(0);
	rd->FX_SetActiveRenderTargets();

	// now its safe to finally set viewport
	rd->m_bViewportDirty = true;
	rd->m_CurViewport = SViewport();
	rd->m_NewViewport = newViewport;
	rd->FX_SetViewport();
}

void CStandardGraphicsPipeline::SwitchFromLegacyPipeline()
{
	CHWShader_D3D::s_nActivationFailMask = 0;

	CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->Reset();
}

std::array<int, EFSS_MAX> CStandardGraphicsPipeline::GetDefaultMaterialSamplers() const
{
	std::array<int, EFSS_MAX> result =
	{
		{
			gcpRendD3D->m_nMaterialAnisoHighSampler,   // EFSS_ANISO_HIGH
			gcpRendD3D->m_nMaterialAnisoLowSampler,    // EFSS_ANISO_LOW
			gcpRendD3D->m_nTrilinearWrapSampler,       // EFSS_TRILINEAR
			gcpRendD3D->m_nBilinearWrapSampler,        // EFSS_BILINEAR
			gcpRendD3D->m_nTrilinearClampSampler,      // EFSS_TRILINEAR_CLAMP
			gcpRendD3D->m_nBilinearClampSampler,       // EFSS_BILINEAR_CLAMP
			gcpRendD3D->m_nMaterialAnisoSamplerBorder, // EFSS_ANISO_HIGH_BORDER
			gcpRendD3D->m_nTrilinearBorderSampler,     // EFSS_TRILINEAR_BORDER
		}
	};

	return result;
}

void CStandardGraphicsPipeline::RenderTiledShading()
{
	SwitchFromLegacyPipeline();
	m_pTiledShadingStage->Execute();
	SwitchToLegacyPipeline();
}

void CStandardGraphicsPipeline::RenderScreenSpaceSSS(CTexture* pIrradianceTex)
{
	SwitchFromLegacyPipeline();
	m_pScreenSpaceSSSStage->Execute(pIrradianceTex);
	SwitchToLegacyPipeline();
}

void CStandardGraphicsPipeline::RenderPostAA()
{
	SwitchFromLegacyPipeline();
	m_pPostAAStage->Execute();
	SwitchToLegacyPipeline();
}

void CStandardGraphicsPipeline::ExecuteHDRPostProcessing()
{
	PROFILE_LABEL_SCOPE("HDR_POSTPROCESS");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	pRenderer->GetModelViewMatrix(PostProcessUtils().m_pView.GetData());
	pRenderer->GetProjectionMatrix(PostProcessUtils().m_pProj.GetData());

	PostProcessUtils().m_pViewProj = PostProcessUtils().m_pView * PostProcessUtils().m_pProj;
	if (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
		PostProcessUtils().m_pViewProj = ReverseDepthHelper::Convert(PostProcessUtils().m_pViewProj);
	PostProcessUtils().m_pViewProj.Transpose();

	// Use specular RT as temporary backbuffer when native resolution rendering is enabled; the RT gets popped before upscaling
	if (pRenderer->IsNativeScalingEnabled())
		pRenderer->FX_PushRenderTarget(0, CTexture::s_ptexSceneSpecular, NULL);

	const uint32 nAAMode = pRenderer->FX_GetAntialiasingType();
	CTexture* pDstRT = CTexture::s_ptexSceneDiffuse;
	if ((nAAMode & (eAT_SMAA_MASK | eAT_FXAA_MASK)) && gRenDev->CV_r_PostProcess && pDstRT)
	{
		assert(pDstRT);
		// Render to intermediate target, to avoid redundant imagecopy/stretchrect for PostAA
		pRenderer->FX_PushRenderTarget(0, pDstRT, &pRenderer->m_DepthBufferOrigMSAA);
		pRenderer->RT_SetViewport(0, 0, pDstRT->GetWidth(), pDstRT->GetHeight());
	}

	// Rain
	{
		CSceneRain* pSceneRain = (CSceneRain*)PostEffectMgr()->GetEffect(ePFX_SceneRain);
		const SRainParams& rainInfo = gcpRendD3D->m_p3DEngineCommon.m_RainInfo;
		if (pSceneRain && pSceneRain->IsActive() && rainInfo.fRainDropsAmount > 0.01f)
		{
			SwitchToLegacyPipeline();
			pSceneRain->Render();
		}
	}

	SwitchFromLegacyPipeline();

	// Note: MB uses s_ptexHDRTargetPrev to avoid doing another copy, so this should be right before the MB pass
	{
		static CStretchRectPass* s_passCopyRT = nullptr;
		if (!m_bUtilityPassesInitialized) s_passCopyRT = CreateStaticUtilityPass<CStretchRectPass>();

		s_passCopyRT->Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetPrev);
	}

	m_pDepthOfFieldStage->Execute();

	m_pMotionBlurStage->Execute();

	// Snow
	{
		CSceneSnow* pSceneSnow = (CSceneSnow*)PostEffectMgr()->GetEffect(ePFX_SceneSnow);
		if (pSceneSnow->IsActiveSnow())
		{
			SwitchToLegacyPipeline();
			pSceneSnow->Render();
			SwitchFromLegacyPipeline();
		}
	}

	// Half resolution downsampling
	{
		PROFILE_LABEL_SCOPE("HALFRES_DOWNSAMPLE_HDRTARGET");
		static CStableDownsamplePass* s_passStableDownsample = nullptr;
		static CStretchRectPass* s_passSimpleDownsample = nullptr;
		if (!m_bUtilityPassesInitialized) s_passStableDownsample = CreateStaticUtilityPass<CStableDownsamplePass>();
		if (!m_bUtilityPassesInitialized) s_passSimpleDownsample = CreateStaticUtilityPass<CStretchRectPass>();

		if (CRenderer::CV_r_HDRBloomQuality > 1)
			s_passStableDownsample->Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0], true);
		else
			s_passSimpleDownsample->Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0]);
	}

	// Quarter resolution downsampling
	{
		PROFILE_LABEL_SCOPE("QUARTER_RES_DOWNSAMPLE_HDRTARGET");
		static CStableDownsamplePass* s_passStableDownsample = nullptr;
		static CStretchRectPass* s_passSimpleDownsample = nullptr;
		if (!m_bUtilityPassesInitialized) s_passStableDownsample = CreateStaticUtilityPass<CStableDownsamplePass>();
		if (!m_bUtilityPassesInitialized) s_passSimpleDownsample = CreateStaticUtilityPass<CStretchRectPass>();

		if (CRenderer::CV_r_HDRBloomQuality > 0)
			s_passStableDownsample->Execute(CTexture::s_ptexHDRTargetScaled[0], CTexture::s_ptexHDRTargetScaled[1], CRenderer::CV_r_HDRBloomQuality >= 1);
		else
			s_passSimpleDownsample->Execute(CTexture::s_ptexHDRTargetScaled[0], CTexture::s_ptexHDRTargetScaled[1]);
	}

	if (pRenderer->m_CurRenderEye != RIGHT_EYE)
	{
		m_pAutoExposureStage->Execute();
	}

	m_pBloomStage->Execute();

	// Lens optics
	if (CRenderer::CV_r_flares)
	{
		PROFILE_LABEL_SCOPE("LENS_OPTICS");
		m_pLensOpticsStage->Execute(m_pCurrentRenderView);
	}

	m_pSunShaftsStage->Execute();
	m_pColorGradingStage->Execute();
	m_pToneMappingStage->Execute();
}

void CStandardGraphicsPipeline::Execute()
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	void (* pRenderFunc)() = &pRenderer->FX_FlushShader_General;
	pRenderer->m_RP.m_pRenderFunc = pRenderFunc;

	pRenderer->RT_SetCameraInfo();

	if (pRenderer->m_CurRenderEye != RIGHT_EYE)
	{
		m_pGpuParticlesStage->Execute(m_pCurrentRenderView);
		SwitchToLegacyPipeline();
		pRenderer->FX_DeferredRainPreprocess();
		m_pComputeSkinningStage->Execute(m_pCurrentRenderView);
		SwitchFromLegacyPipeline();
	}

	gcpRendD3D->GetS3DRend().TryInjectHmdCameraAsync(m_pCurrentRenderView);

	// Prepare tiled shading resources early to give DMA operations enough time to finish
	m_pTiledShadingStage->PrepareResources();

	// GBuffer
	m_pSceneGBufferStage->Execute();

	if (pRenderer->m_CurRenderEye != RIGHT_EYE)
	{
		m_pShadowMapStage->Prepare(m_pCurrentRenderView);
	}

	if (pRenderer->m_nGraphicsPipeline >= 2)
	{
		// Wait for GBuffer draw jobs to finish
		GetCurrentRenderView()->GetDrawer().WaitForDrawSubmission();
	}

	// Issue split barriers for GBuffer
	CTexture* pTextures[] = {
		CTexture::s_ptexSceneNormalsMap,
		CTexture::s_ptexSceneDiffuse,
		CTexture::s_ptexSceneSpecular,
		pRenderer->m_DepthBufferOrigMSAA.pTexture
	};
	CDeviceGraphicsCommandInterface* pCmdList = CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetGraphicsInterface();
	pCmdList->BeginResourceTransitions(CRY_ARRAY_COUNT(pTextures), pTextures, eResTransition_TextureRead);

	// Shadow maps
	if (pRenderer->m_CurRenderEye != RIGHT_EYE)
	{
		m_pShadowMapStage->Execute();
	}

	m_pSceneGBufferStage->ExecuteLinearizeDepth();

	if (CVrProjectionManager::IsMultiResEnabledStatic())
		CVrProjectionManager::Instance()->ExecuteFlattenDepth(CTexture::s_ptexZTarget, CVrProjectionManager::Instance()->GetZTargetFlattened());

	// Depth downsampling
	{
		CTexture* pZTexture = gcpRendD3D->m_DepthBufferOrigMSAA.pTexture;

		static CDepthDownsamplePass* s_passDepthDownsample2 = nullptr;
		if (!m_bUtilityPassesInitialized) s_passDepthDownsample2 = CreateStaticUtilityPass<CDepthDownsamplePass>();

		static CDepthDownsamplePass* s_passDepthDownsample4 = nullptr;
		if (!m_bUtilityPassesInitialized) s_passDepthDownsample4 = CreateStaticUtilityPass<CDepthDownsamplePass>();

		static CDepthDownsamplePass* s_passDepthDownsample8 = nullptr;
		if (!m_bUtilityPassesInitialized) s_passDepthDownsample8 = CreateStaticUtilityPass<CDepthDownsamplePass>();

		CTexture* pSourceDepth = CTexture::s_ptexZTarget;
#if CRY_PLATFORM_DURANGO
		pSourceDepth = pZTexture;  // On Durango reading device depth is faster since it is in ESRAM
#endif

		s_passDepthDownsample2->Execute(pSourceDepth, CTexture::s_ptexZTargetScaled, (pSourceDepth == pZTexture), true);
		s_passDepthDownsample4->Execute(CTexture::s_ptexZTargetScaled, CTexture::s_ptexZTargetScaled2, false, false);
		s_passDepthDownsample8->Execute(CTexture::s_ptexZTargetScaled2, CTexture::s_ptexZTargetScaled3, false, false);
	}

	SwitchToLegacyPipeline();

	// Deferred decals
	{
		pRenderer->FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &pRenderer->m_DepthBufferOrigMSAA);
		pRenderer->FX_PushRenderTarget(1, CTexture::s_ptexSceneDiffuse, NULL);
		pRenderer->FX_PushRenderTarget(2, CTexture::s_ptexSceneSpecular, NULL);

		pRenderer->FX_DeferredDecals();

		pRenderer->FX_PopRenderTarget(2);
		pRenderer->FX_PopRenderTarget(1);
		pRenderer->FX_PopRenderTarget(0);
	}

	// Depth readback (for occlusion culling)
	{
		pRenderer->FX_ZTargetReadBack();
	}

	// GBuffer modifiers
	{
		pRenderer->FX_DeferredRainGBuffer();
		pRenderer->FX_DeferredSnowLayer();
	}

	SwitchFromLegacyPipeline();

	// Generate cloud volume textures for shadow mapping.
	m_pVolumetricCloudsStage->ExecuteShadowGen();

	// SVOGI
	{
#if defined(FEATURE_SVO_GI)
		if (CSvoRenderer::GetInstance())
		{
			PROFILE_LABEL_SCOPE("SVOGI");
			SwitchToLegacyPipeline();
			CSvoRenderer::GetInstance()->UpdateCompute();
			CSvoRenderer::GetInstance()->UpdateRender();
			SwitchFromLegacyPipeline();
		}
#endif
	}

	// Screen Space Reflections
	m_pScreenSpaceReflectionsStage->Execute();

	if (pRenderer->m_nGraphicsPipeline >= 2)
	{
		// Wait for Shadow Map draw jobs to finish (also required for HeightMap AO)
		GetCurrentRenderView()->GetDrawer().WaitForDrawSubmission();
	}

	// Height Map AO
	ShadowMapFrustum* pHeightMapFrustum = nullptr;
	CTexture* pHeightMapAOScreenDepthTex = nullptr;
	CTexture* pHeightMapAOTex = nullptr;
	m_pHeightMapAOStage->Execute(pHeightMapFrustum, pHeightMapAOScreenDepthTex, pHeightMapAOTex);

	// Screen Space Obscurance
	m_pScreenSpaceObscuranceStage->Execute(pHeightMapFrustum, pHeightMapAOScreenDepthTex, pHeightMapAOTex);

	// Water volume caustics
	m_pWaterStage->ExecuteWaterVolumeCaustics();

	pRenderer->m_RP.m_PersFlags2 |= RBPF2_NOSHADERFOG;

	// Deferred shading
	{
		PROFILE_LABEL_SCOPE("DEFERRED_LIGHTING");
		m_pClipVolumesStage->Prepare(m_pCurrentRenderView);
		m_pClipVolumesStage->Execute();

		SwitchToLegacyPipeline();
		pRenderer->FX_ProcessRenderList(EFSLIST_DEFERRED_PREPROCESS, pRenderFunc, false);
		SwitchToLegacyPipeline();
	}

	{
		PROFILE_FRAME(WaitForParticleRendItems);
		pRenderer->SyncComputeVerticesJobs();
		pRenderer->UnLockParticleVideoMemory();
	}

	// Opaque forward passes
	m_pSceneForwardStage->Execute_Opaque();

	SwitchFromLegacyPipeline();

	// Deferred ocean caustics
	m_pWaterStage->ExecuteDeferredOceanCaustics();

	// Fog
	{
		m_pVolumetricFogStage->Execute();

		m_pFogStage->Execute();
	}

	pRenderer->m_RP.m_PersFlags2 &= ~(RBPF2_NOSHADERFOG);

	// Clouds
	{
		m_pVolumetricCloudsStage->Execute();
	}

	// Water fog volumes
	{
		m_pWaterStage->ExecuteWaterFogVolumeBeforeTransparent();
	}

	pRenderer->UpdatePrevMatrix(true);

	SwitchToLegacyPipeline();

	// Transparent (below water)
	m_pSceneForwardStage->Execute_TransparentBelowWater();

	SwitchFromLegacyPipeline();

	// Ocean and water volumes
	{
		m_pWaterStage->Execute();
	}

	SwitchToLegacyPipeline();

	// Transparent (above water)
	m_pSceneForwardStage->Execute_TransparentAboveWater();

	if (CRenderer::CV_r_TranspDepthFixup)
		pRenderer->FX_DepthFixupMerge();

	// Half-res particles
	{
		pRenderer->FX_ProcessHalfResParticlesRenderList(m_pCurrentRenderView, EFSLIST_HALFRES_PARTICLES, pRenderFunc, true);
	}

	pRenderer->m_CameraProjMatrixPrev = pRenderer->m_CameraProjMatrix;

	// Insert fence which is used on consoles to prevent overwriting video memory
	pRenderer->InsertParticleVideoDataFence();

	pRenderer->FX_DeferredSnowDisplacement();

	// Pop HDR target from RT stack
	{
		assert(pRenderer->m_RTStack[0][pRenderer->m_nRTStackLevel[0]].m_pTex == CTexture::s_ptexHDRTarget);
		pRenderer->FX_SetActiveRenderTargets();
		pRenderer->RT_UnbindTMUs();
		pRenderer->FX_PopRenderTarget(0);
		//pRenderer->EF_ClearTargetsLater(0);
	}

	if (pRenderer->m_CurRenderEye == RIGHT_EYE || !pRenderer->GetS3DRend().IsStereoEnabled() || !pRenderer->GetS3DRend().RequiresSequentialSubmission())
	{
		m_pGpuParticlesStage->PostDraw(m_pCurrentRenderView);
	}

	if (!(pRenderer->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN))
	{
		ExecuteHDRPostProcessing();

		// HDR and LDR post-processing
		{
			pRenderer->m_RP.m_PersFlags1 &= ~RBPF1_SKIP_AFTER_POST_PROCESS;
			SwitchToLegacyPipeline();

			pRenderer->FX_ProcessRenderList(EFSLIST_AFTER_HDRPOSTPROCESS, pRenderFunc, false);
			pRenderer->FX_ProcessRenderList(EFSLIST_POSTPROCESS, pRenderFunc, false);

			pRenderer->RT_SetViewport(0, 0, pRenderer->GetWidth(), pRenderer->GetHeight());

			pRenderer->FX_ProcessRenderList(EFSLIST_AFTER_POSTPROCESS, pRenderFunc, false);
		}

		SwitchFromLegacyPipeline();
		m_pSceneCustomStage->Execute();
		SwitchToLegacyPipeline();

		if (CRenderer::CV_r_DeferredShadingDebug)
			pRenderer->FX_DeferredRendering(m_pCurrentRenderView, true);
	}

	m_bUtilityPassesInitialized = true;
	m_pCurrentRenderView = nullptr;
}
