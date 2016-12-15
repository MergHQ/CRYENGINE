// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   PostProcessWater : water related post processing

   Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

   =============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <Cry3DEngine/I3DEngine.h>
#include "D3DPostProcess.h"

#pragma warning(disable: 4244)

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CUnderwaterGodRays::Render()
{
	PROFILE_LABEL_SCOPE("GODRAYS");

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	float fAmount = m_pAmount->GetParam();
	float fWatLevel = SPostEffectsUtils::m_fWaterLevel;

	static CCryNameTSCRC pTechName("UnderwaterGodRays");
	static CCryNameR pParam0Name("PI_GodRaysParamsVS");
	static CCryNameR pParam1Name("PI_GodRaysParamsPS");

	uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
	gRenDev->m_RP.m_FlagsShader_RT &= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];

	if (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Render god-rays into low-res render target for less fillrate hit

	gcpRendD3D->FX_ClearTarget(CTexture::s_ptexBackBufferScaled[1], Clr_Transparent);
	gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexBackBufferScaled[1], 0);
	gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[1]->GetWidth(), CTexture::s_ptexBackBufferScaled[1]->GetHeight());

	//  PostProcessUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES);
	uint32 nPasses;
	CShaderMan::s_shPostEffects->FXSetTechnique(pTechName);
	CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETSTATES);

	int nSlicesCount = 10;
	bool bInVisarea = (gRenDev->m_p3DEngineCommon.m_pCamVisAreaInfo.nFlags & S3DEngineCommon::VAF_EXISTS_FOR_POSITION);
	Vec3 vSunDir = bInVisarea ? Vec3(0.5f, 0.5f, 1.0f) : -gEnv->p3DEngine->GetSunDirNormalized();

	for (int r(0); r < nSlicesCount; ++r)
	{
		// !force updating constants per-pass!
		CShaderMan::s_shPostEffects->FXBeginPass(0);

		// Set per instance params
		Vec4 pParams = Vec4(fWatLevel, fAmount, r, 1.0f / (float) nSlicesCount);
		CShaderMan::s_shPostEffects->FXSetVSFloat(pParam0Name, &pParams, 1);

		CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams, 1);

		gcpRendD3D->SetCullMode(R_CULL_NONE);
		gcpRendD3D->FX_SetState(GS_BLSRC_ONE | GS_BLDST_ONE | GS_NODEPTHTEST);

		PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0);

		CShaderMan::s_shPostEffects->FXEndPass();
	}

	CShaderMan::s_shPostEffects->FXEnd();
	gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

	// Restore previous viewport
	gcpRendD3D->FX_PopRenderTarget(0);
	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Display god-rays

	CCryNameTSCRC pTechName0("UnderwaterGodRaysFinal");

	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName0, FEF_DONTSETSTATES);
	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
	Vec4 pParams = Vec4(CRenderer::CV_r_water_godrays_distortion, 0, 0, 0);
	CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams, 1);
	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
	PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterDroplets::Render()
{
	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	static CCryNameTSCRC pTechName("WaterDroplets");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	const float fUserAmount = m_pAmount->GetParam();

	const float fAtten = 1.0f; //clamp_tpl<float>(fabs( gRenDev->GetRCamera().vOrigin.z - SPostEffectsUtils::m_fWaterLevel ), 0.0f, 1.0f);
	Vec4 vParams = Vec4(1, 1, 1, min(fUserAmount, 1.0f) * fAtten);
	static CCryNameR pParamName("waterDropletsParams");
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterFlow::Render()
{
	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	float fAmount = m_pAmount->GetParam();

	static CCryNameTSCRC pTechName("WaterFlow");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	Vec4 vParams = Vec4(1, 1, 1, fAmount);
	static CCryNameR pParamName("waterFlowParams");
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterVolume::Render()
{
	PROFILE_LABEL_SCOPE("WATERVOLUME_TEXGEN");

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	const int frameID = SPostEffectsUtils::m_iFrameCounter % 2;

	{
		static int nFrameID = 0;

		const int nGridSize = 64;

		int nCurFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameID;
		if (nFrameID != nCurFrameID)
		{
			static Vec4 pParams0(0, 0, 0, 0), pParams1(0, 0, 0, 0);
			Vec4 pCurrParams0, pCurrParams1;
			gEnv->p3DEngine->GetOceanAnimationParams(pCurrParams0, pCurrParams1);

			// Update sim settings
			if (WaterSimMgr()->NeedInit() || pCurrParams0.x != pParams0.x || pCurrParams0.y != pParams0.y ||
			    pCurrParams0.z != pParams0.z || pCurrParams0.w != pParams0.w || pCurrParams1.x != pParams1.x ||
			    pCurrParams1.y != pParams1.y || pCurrParams1.z != pParams1.z || pCurrParams1.w != pParams1.w)
			{
				pParams0 = pCurrParams0;
				pParams1 = pCurrParams1;
				WaterSimMgr()->Create(1.0, pParams0.x, pParams0.z, 1.0f, 1.0f);
			}

			// Create texture if required
			if (!CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeTemp[frameID]))
			{
				if (!CTexture::s_ptexWaterVolumeTemp[frameID]->Create2DTexture(64, 64, 1,
				                                                               FT_DONT_RELEASE | FT_NOMIPS | FT_STAGE_UPLOAD,
				                                                               0, eTF_R32G32B32A32F, eTF_R32G32B32A32F))
					return;
			}

			CTexture* pTexture = CTexture::s_ptexWaterVolumeTemp[frameID];

			// Copy data..
			if (CTexture::IsTextureExist(pTexture))
			{
				//const float fUpdateTime = 2.f*0.125f*gEnv->pTimer->GetCurrTime();
				const float fUpdateTime = 0.125 * gEnv->pTimer->GetCurrTime();// / clamp_tpl<float>(pParams1.x, 0.55f, 1.0f);

				void* pRawPtr = NULL;
				WaterSimMgr()->Update(nCurFrameID, fUpdateTime, true, pRawPtr);

				Vec4* pDispGrid = WaterSimMgr()->GetDisplaceGrid();

				const uint32 pitch = 4 * sizeof(f32) * nGridSize;
				const uint32 width = nGridSize;
				const uint32 height = nGridSize;

				STALL_PROFILER("update subresource")

				CDeviceTexture * pDevTex = pTexture->GetDevTexture();
				pDevTex->UploadFromStagingResource(0, [=](void* pData, uint32 rowPitch, uint32 slicePitch)
				{
					cryMemcpy(pData, pDispGrid, 4 * width * height * sizeof(f32));
					return true;
				});
			}
			nFrameID = nCurFrameID;
		}
	}

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	// make final normal map
	gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexWaterVolumeDDN, NULL);
	gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexWaterVolumeDDN->GetWidth(), CTexture::s_ptexWaterVolumeDDN->GetHeight());

	static CCryNameTSCRC pTechName("WaterVolumesNormalGen");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	static CCryNameR pParamName("waterVolumesParams");
	Vec4 vParams = Vec4(64.0f, 64.0f, 64.0f, 64.0f);
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

	int32 nFilter = FILTER_LINEAR;

	PostProcessUtils().SetTexture(CTexture::s_ptexWaterVolumeTemp[frameID], 0, nFilter, 0);
	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();

	gcpRendD3D->FX_PopRenderTarget(0);
	gcpRendD3D->RT_SetViewport(0, 0, iWidth, iHeight);

	// Generate mipmaps
	{
		if (!CTexture::s_pMipperWaterVolumeDDN)
			CTexture::s_pMipperWaterVolumeDDN = new CMipmapGenPass();

		gcpRendD3D->GetGraphicsPipeline().SwitchFromLegacyPipeline();
		CTexture::s_pMipperWaterVolumeDDN->Execute(CTexture::s_ptexWaterVolumeDDN);
		gcpRendD3D->GetGraphicsPipeline().SwitchToLegacyPipeline();
	}

	// HACK (re-set back-buffer): due to lazy RT updates/setting there's strong possibility we run into problems on x360 when we try to resolve from edram with no RT set
	gcpRendD3D->FX_SetActiveRenderTargets();
	gcpRendD3D->FX_ResetPipe();

	// disable processing
	m_pAmount->SetParam(0.0f);
}
