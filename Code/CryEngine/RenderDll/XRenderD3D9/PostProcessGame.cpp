// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   PostProcessGame : game related post processing

   Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

   =============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <Cry3DEngine/I3DEngine.h>
#include "D3DPostProcess.h"

#pragma warning(disable: 4244)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game/Hud specific post-processing
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_CustomRenderScene(bool bEnable)
{
	if (bEnable)
	{
		PostProcessUtils().Log(" +++ Begin custom render scene +++ \n");
		if ((CRenderer::CV_r_customvisions == 1) || (CRenderer::CV_r_customvisions == 3))
		{
#ifdef SUPPORTS_MSAA
			CTexture::s_ptexSceneNormalsMap->SetUseMultisampledRTV(false);
#endif

			// TODO: pick another texture, normal's clear value is 0.5/0.5/0.0/0.0
			FX_ClearTarget(CTexture::s_ptexSceneNormalsMap, Clr_Transparent);
			FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &gcpRendD3D->m_DepthBufferOrig);
			RT_SetViewport(0, 0, GetWidth(), GetHeight());
		}

		m_RP.m_PersFlags2 |= RBPF2_CUSTOM_RENDER_PASS;
	}
	else
	{
		if ((CRenderer::CV_r_customvisions == 1) || (CRenderer::CV_r_customvisions == 3))
		{
			FX_PopRenderTarget(0);

#ifdef SUPPORTS_MSAA
			CTexture::s_ptexSceneNormalsMap->SetUseMultisampledRTV(true);
			CTexture::s_ptexSceneNormalsMap->SetResolved(true);
#endif
		}

		FX_ResetPipe();

		PostProcessUtils().Log(" +++ End custom render scene +++ \n");

		RT_SetViewport(0, 0, GetWidth(), GetHeight());

		m_RP.m_PersFlags2 &= ~RBPF2_CUSTOM_RENDER_PASS;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CHudSilhouettes::Render()
{
	PROFILE_LABEL_SCOPE("HUD_SILHOUETTES");

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	float fBlendParam = clamp_tpl<float>(m_pAmount->GetParam(), 0.0f, 1.0f);
	float fType = m_pType->GetParam();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Render highlighted geometry
	{
		PROFILE_LABEL_SCOPE("HUD_SILHOUETTES_ENTITIES_PASS");

		uint32 nPrevPers2 = gRenDev->m_RP.m_PersFlags2;
		gRenDev->m_RP.m_PersFlags2 &= ~RBPF2_NOPOSTAA;

		// render to texture all masks
		gcpRendD3D->FX_ProcessPostRenderLists(FB_CUSTOM_RENDER);

		gRenDev->m_RP.m_PersFlags2 = nPrevPers2;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Render silhouettes
	switch (CRenderer::CV_r_customvisions)
	{
	case 1:
		{
			RenderDeferredSilhouettes(fBlendParam, fType);
			break;
		}
	case 2:
		{
			// These are forward rendered so do nothing
			break;
		}
	case 3:
		{
			RenderDeferredSilhouettesOptimised(fBlendParam, fType);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void CHudSilhouettes::RenderDeferredSilhouettes(float fBlendParam, float fType)
{
	gcpRendD3D->RT_SetViewport(PostProcessUtils().m_pScreenRect.left, PostProcessUtils().m_pScreenRect.top, PostProcessUtils().m_pScreenRect.right, PostProcessUtils().m_pScreenRect.bottom);
	PostProcessUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

	CTexture* pScreen = CTexture::s_ptexSceneNormalsMap;
	CTexture* pMask = CTexture::s_ptexBlack;
	CTexture* pMaskBlurred = CTexture::s_ptexBlack;

	// skip processing, nothing was added to mask
	if ((SRendItem::BatchFlags(EFSLIST_GENERAL) | SRendItem::BatchFlags(EFSLIST_TRANSP)) & FB_CUSTOM_RENDER)
	{
		// could render directly to frame buffer ? save 1 resolve - no glow though
		{
			// store silhouettes/signature temporary render target, so that we can post process this afterwards
			gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexBackBufferScaled[0], NULL);
			gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());

			static CCryNameTSCRC pTech1Name("BinocularView");
			PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

			gRenDev->FX_SetState(GS_NODEPTHTEST);

			// Set VS params
			const float uvOffset = 2.0f;
			Vec4 vsParams = Vec4(uvOffset, 0.0f, 0.0f, 0.0f);
			CShaderMan::s_shPostEffectsGame->FXSetVSFloat(m_vsParamName, &vsParams, 1);

			// Set PS default params
			Vec4 pParams = Vec4(0, 0, 0, (!fType) ? 1.0f : 0.0f);
			CShaderMan::s_shPostEffectsGame->FXSetPSFloat(m_psParamName, &pParams, 1);

			PostProcessUtils().SetTexture(CTexture::s_ptexSceneNormalsMap, 0, FILTER_POINT);
			PostProcessUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);
			PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

			PostProcessUtils().ShEndPass();
			gcpRendD3D->FX_PopRenderTarget(0);

			pMask = CTexture::s_ptexBackBufferScaled[0];
			pMaskBlurred = CTexture::s_ptexBackBufferScaled[1];
		}

		////////////////////////////////////////////////////////////////////////////////////////////////
		// compute glow

		{
			GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);

			// blur - for glow
			//			GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.5f, 2.0f, false);
			GetUtils().TexBlurIterative(CTexture::s_ptexBackBufferScaled[1], 1, false);

			gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());
		}

		////////////////////////////////////////////////////////////////////////////////////////////////
		// finally add silhouettes to screen

		{
			static CCryNameTSCRC pTechSilhouettesName("BinocularViewSilhouettes");

			GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechSilhouettesName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

			gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

			GetUtils().SetTexture(pMask, 0);
			GetUtils().SetTexture(pMaskBlurred, 1);

			// Set PS default params
			Vec4 pParams = Vec4(0, 0, 0, fBlendParam * 0.33f);
			static CCryNameR pParamName("psParams");
			CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &pParams, 1);

			GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0, &gcpRendD3D->m_FullResRect);

			GetUtils().ShEndPass();
		}
	}

	gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void CHudSilhouettes::RenderDeferredSilhouettesOptimised(float fBlendParam, float fType)
{
	const bool bHasSilhouettesToRender = ((SRendItem::BatchFlags(EFSLIST_GENERAL) |
	                                       SRendItem::BatchFlags(EFSLIST_TRANSP))
	                                      & FB_CUSTOM_RENDER) ? true : false;

	if (bHasSilhouettesToRender)
	{
		// Down Sample
		GetUtils().StretchRect(CTexture::s_ptexSceneNormalsMap, CTexture::s_ptexBackBufferScaled[0]);

		PROFILE_LABEL_SCOPE("HUD_SILHOUETTES_DEFERRED_PASS");

		// Draw silhouettes
		GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, m_deferredSilhouettesOptimisedTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE | GS_NOCOLMASK_A);

		GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0, FILTER_LINEAR);

		// Set vs params
		const float uvOffset = 1.5f;
		Vec4 vsParams = Vec4(uvOffset, 0.0f, 0.0f, 0.0f);
		CShaderMan::s_shPostEffectsGame->FXSetVSFloat(m_vsParamName, &vsParams, 1);

		// Set ps params
		const float fillStrength = m_pFillStr->GetParam();
		const float silhouetteBoost = 1.7f;
		const float silhouetteBrightness = 1.333f;
		const float focusReduction = 0.33f;
		const float silhouetteAlpha = 0.8f;
		const bool bBinocularsActive = (!fType);
		const float silhouetteStrength = (bBinocularsActive ? 1.0f : (fBlendParam * focusReduction)) * silhouetteAlpha;

		Vec4 psParams;
		psParams.x = silhouetteStrength;
		psParams.y = silhouetteBoost;
		psParams.z = silhouetteBrightness;
		psParams.w = fillStrength;

		CShaderMan::s_shPostEffectsGame->FXSetPSFloat(m_psParamName, &psParams, 1);

		GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0, &gcpRendD3D->m_FullResRect);

		GetUtils().ShEndPass();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CAlienInterference::Render()
{
	PROFILE_LABEL_SCOPE("ALIEN_INTERFERENCE");

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	float fAmount = m_pAmount->GetParam();

	static CCryNameTSCRC pTechName("AlienInterference");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	Vec4 vParams = Vec4(1, 1, (float)PostProcessUtils().m_iFrameCounter, fAmount);
	static CCryNameR pParamName("psParams");
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

	static CCryNameR pParamAlienInterferenceName("AlienInterferenceTint");
	vParams = m_pTintColor->GetParamVec4();
	vParams.x *= 2.0f;
	vParams.y *= 2.0f;
	vParams.z *= 2.0f;
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamAlienInterferenceName, &vParams, 1);

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenFrost::Render()
{
	float fAmount = m_pAmount->GetParam();

	if (fAmount <= 0.02f)
	{
		m_fRandOffset = cry_random(0.0f, 1.0f);
		return;
	}

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	float fCenterAmount = m_pCenterAmount->GetParam();

	PostProcessUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[1]);

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// display frost
	static CCryNameTSCRC pTechName("ScreenFrost");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	static CCryNameR pParam0Name("screenFrostParamsVS");
	static CCryNameR pParam1Name("screenFrostParamsPS");

	PostProcessUtils().ShSetParamVS(pParam0Name, Vec4(1, 1, 1, m_fRandOffset));
	PostProcessUtils().ShSetParamPS(pParam1Name, Vec4(1, 1, fCenterAmount, fAmount));

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFlashBang::Preprocess()
{

	float fActive = m_pActive->GetParam();
	if (fActive || m_fSpawnTime)
	{
		if (fActive)
			m_fSpawnTime = 0.0f;

		m_pActive->SetParam(0.0f);

		return true;
	}

	return false;
}

void CFlashBang::Render()
{
	float fTimeDuration = m_pTime->GetParam();
	float fDifractionAmount = m_pDifractionAmount->GetParam();
	float fBlindTime = m_pBlindAmount->GetParam();

	if (!m_fSpawnTime)
	{
		m_fSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();

		// Create temporary ghost image and capture screen
		SAFE_DELETE(m_pGhostImage);

		m_pGhostImage = new SDynTexture(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1, eTF_R8G8B8A8, eTT_2D, FT_STATE_CLAMP, "GhostImageTempRT");
		m_pGhostImage->Update(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1);

		if (m_pGhostImage && m_pGhostImage->m_pTexture)
		{
			PostProcessUtils().StretchRect(CTexture::s_ptexBackBuffer, m_pGhostImage->m_pTexture);
		}
	}

	// Update current time
	float fCurrTime = (PostProcessUtils().m_pTimer->GetCurrTime() - m_fSpawnTime) / fTimeDuration;

	// Effect finished
	if (fCurrTime > 1.0f)
	{
		m_fSpawnTime = 0.0f;
		m_pActive->SetParam(0.0f);

		SAFE_DELETE(m_pGhostImage);

		return;
	}

	// make sure to update dynamic texture if required
	if (m_pGhostImage && !m_pGhostImage->m_pTexture)
	{
		m_pGhostImage->Update(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1);
	}

	if (!m_pGhostImage || !m_pGhostImage->m_pTexture)
	{
		return;
	}

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	//////////////////////////////////////////////////////////////////////////////////////////////////
	static CCryNameTSCRC pTechName("FlashBang");
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	float fLuminance = 1.0f - fCurrTime;  //PostProcessUtils().InterpolateCubic(0.0f, 1.0f, 0.0f, 1.0f, fCurrTime);

	// opt: some pre-computed constants
	Vec4 vParams = Vec4(fLuminance, fLuminance * fDifractionAmount, 3.0f * fLuminance * fBlindTime, fLuminance);
	static CCryNameR pParamName("vFlashBangParams");
	CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

	PostProcessUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
	PostProcessUtils().SetTexture(m_pGhostImage->m_pTexture, 1, FILTER_LINEAR);

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterKillCamera::Render()
{
	PROFILE_LABEL_SCOPE("KILL_CAMERA");

	// Update time
	float frameTime = PostProcessUtils().m_pTimer->GetFrameTime();
	m_blindTimer += frameTime;

	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

	float grainStrength = m_pGrainStrength->GetParam();
	Vec4 chromaShift = m_pChromaShift->GetParamVec4(); // xyz = offset, w = strength
	Vec4 vignette = m_pVignette->GetParamVec4();       // xy = screen scale, z = radius, w = blind noise vignette scale
	Vec4 colorScale = m_pColorScale->GetParamVec4();

	// Scale vignette with overscan borders
	Vec2 overscanBorders = Vec2(0.0f, 0.0f);
	gRenDev->EF_Query(EFQ_OverscanBorders, overscanBorders);
	const float vignetteOverscanMaxValue = 4.0f;
	const Vec2 vignetteOverscanScalar = Vec2(1.0f, 1.0f) + (overscanBorders * vignetteOverscanMaxValue);
	vignette.x *= vignetteOverscanScalar.x;
	vignette.y *= vignetteOverscanScalar.y;

	float inverseVignetteRadius = 1.0f / clamp_tpl<float>(vignette.z * 2.0f, 0.001f, 2.0f);
	Vec2 vignetteScreenScale(max(vignette.x, 0.0f), max(vignette.y, 0.0f));

	// Blindness
	Vec4 blindness = m_pBlindness->GetParamVec4(); // x = blind duration, y = blind fade out duration, z = blindness grey scale, w = blind noise min scale
	float blindDuration = max(blindness.x, 0.0f);
	float blindFadeOutDuration = max(blindness.y, 0.0f);
	float blindGreyScale = clamp_tpl<float>(blindness.z, 0.0f, 1.0f);
	float blindNoiseMinScale = clamp_tpl<float>(blindness.w, 0.0f, 10.0f);
	float blindNoiseVignetteScale = clamp_tpl<float>(vignette.w, 0.0f, 10.0f);

	float blindAmount = 0.0f;
	if (m_blindTimer < blindDuration)
	{
		blindAmount = 1.0f;
	}
	else
	{
		float blindFadeOutTimer = m_blindTimer - blindDuration;
		if (blindFadeOutTimer < blindFadeOutDuration)
		{
			blindAmount = 1.0f - (blindFadeOutTimer / blindFadeOutDuration);
		}
	}

	// Rendering
	PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, m_techName, FEF_DONTSETSTATES);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	gcpRendD3D->GetViewport(&x, &y, &width, &height);

	// Set PS default params
	const int PARAM_COUNT = 4;
	Vec4 pParams[PARAM_COUNT];

	// psParams[0] - xy = Rand lookup, zw = vignetteScreenScale * invRadius
	pParams[0].x = cry_random(0, 1023) / (float)width;
	pParams[0].y = cry_random(0, 1023) / (float)height;
	pParams[0].z = vignetteScreenScale.x * inverseVignetteRadius;
	pParams[0].w = vignetteScreenScale.y * inverseVignetteRadius;

	// psParams[1] - xyz = color scale, w = grain strength
	pParams[1].x = colorScale.x;
	pParams[1].y = colorScale.y;
	pParams[1].z = colorScale.z;
	pParams[1].w = grainStrength;

	// psParams[2] - xyz = chroma shift, w = chroma shift color strength
	pParams[2] = chromaShift;

	// psParams[3] - x = blindAmount, y = blind grey scale, z = blindNoiseVignetteScale, w = blindNoiseMinScale
	pParams[3].x = blindAmount;
	pParams[3].y = blindGreyScale;
	pParams[3].z = blindNoiseVignetteScale;
	pParams[3].w = blindNoiseMinScale;

	CShaderMan::s_shPostEffects->FXSetPSFloat(m_paramName, pParams, PARAM_COUNT);

	PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	PostProcessUtils().ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

enum ENanoGlassDebugView
{
	eNANO_GLASS_DEBUG_VIEW_Off = 0,
	eNANO_GLASS_DEBUG_VIEW_WireFrame
};

void CNanoGlass::Render()
{
	// Update HUD Flash here so we can create a mask from it
	C3DHud* pHud3D = (C3DHud*) PostEffectMgr()->GetEffect(ePFX_3DHUD);
	const bool bIsHudRendering = pHud3D->Preprocess();
	if (bIsHudRendering)
	{
		pHud3D->FlashUpdateRT();
	}

	PROFILE_LABEL_SCOPE("NANO_GLASS_FILTER");

	// Clear sample flags
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] |
	                                       g_HWSR_MaskBit[HWSR_SAMPLE1] |
	                                       g_HWSR_MaskBit[HWSR_SAMPLE2] |
	                                       g_HWSR_MaskBit[HWSR_SAMPLE4] |
	                                       g_HWSR_MaskBit[HWSR_DEBUG1] |
	                                       g_HWSR_MaskBit[HWSR_DEBUG2]);

	// Update RT pointers
	const int HUD_MASK_INDEX = 1;
	m_pHudMask = CTexture::s_ptexBackBufferScaled[HUD_MASK_INDEX];
	m_pHudMaskTemp = CTexture::s_ptexBackBufferScaledTemp[HUD_MASK_INDEX];

	DownSampleBackBuffer();

	if (bIsHudRendering)
	{
		CreateHudMask(pHud3D);
	}

	bool bDebugPass = false;
	RenderPass(bDebugPass, bIsHudRendering);

#ifndef _RELEASE
	if (CRenderer::CV_r_PostProcessNanoGlassDebugView)
	{
		bDebugPass = true;
		RenderPass(bDebugPass, bIsHudRendering);
	}
#endif
}

void CNanoGlass::DownSampleBackBuffer()
{
	// Only need downsample when using back buffer as a brightness scalar
	const float backBufferBrightessScalar = m_pFilter->GetParamVec4().x;
	if (backBufferBrightessScalar > 0.0f)
	{
		PROFILE_LABEL_SCOPE("DOWN_SAMPLE_BACK_BUFFER");

		// Optimized platform specific down sampling of texture
		const bool bClearAlpha = false;
		const bool bDecodeSrcRGBK = false;
		const bool bEncodeDstRGBK = false;
		const bool bBigDownSample = true;
		GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[2], bClearAlpha, bDecodeSrcRGBK, bEncodeDstRGBK, bBigDownSample);
	}
}

void CNanoGlass::CreateHudMask(C3DHud* pHud3D)
{
	PROFILE_LABEL_SCOPE("CREATE_HUD_MASK");

	// Render HUD
	gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

	CD3DStereoRenderer& pS3DRend = gcpRendD3D->GetS3DRend();
	bool bPostProcStereo = pS3DRend.IsPostStereoEnabled();

	if (bPostProcStereo && CRenderer::CV_r_StereoHudScreenDist)
	{
		// Not perfect S3D solution, as mask will be slightly larger, but looks ok and is cheap
		// Ideally we want to render the NanoGlass twice (once for each eye) - but we don't have the budget!
		const float maxParallax = 0.005f * pS3DRend.GetStereoStrength();

		// Render left eye
		pHud3D->SetMaxParallax(-maxParallax);
		pHud3D->UpdateBloomRT(m_pHudMask, NULL);

		// Render right eye
		pHud3D->SetMaxParallax(maxParallax);
		pHud3D->UpdateBloomRT(m_pHudMask, NULL);

		pHud3D->SetMaxParallax(0);
	}
	else
	{
		pHud3D->UpdateBloomRT(m_pHudMask, NULL);
	}

	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE1];

	// Blur
	const int blurIterationCount = 2;

	const bool bDilate = false;
	GetUtils().TexBlurIterative(m_pHudMask, blurIterationCount, bDilate, m_pHudMaskTemp);
}

void CNanoGlass::RenderPass(bool bDebugPass, bool bIsHudRendering)
{
	// Set PS default params
	const int PARAM_COUNT = 7;
	Vec4 psParams[PARAM_COUNT];
	Vec4 filter = m_pFilter->GetParamVec4(); // x = backBufferBrightessScalar, y = mistAlpha, z = noiseStrength, w = overChargeStrength
	float overChargeStrength = filter.w;
	float backBufferBrightessScalar = filter.x;
	float refractionBlurScale = filter.w;
	float effectAlpha = m_pEffectAlpha->GetParam();
	float introAnimPos = m_pStartAnimPos->GetParam();
	float outroAnimPos = m_pEndAnimPos->GetParam();
	float brightness = m_pBrightness->GetParam();
	Vec4 movement0 = m_pMovement[0]->GetParamVec4();
	Vec4 movement1 = m_pMovement[1]->GetParamVec4();
	Vec4 vignette = m_pVignette->GetParamVec4(); // xy = vignetteScreenScale , z = mistSinStrength, w = free
	Vec4 hexColor = m_pHexColor->GetParamVec4();
	float hitStrength = hexColor.w;

	// Calc animPos
	const float animScale = 1.5f;
	const float animPos = (1.0 - clamp_tpl<float>((introAnimPos - outroAnimPos), 0.0f, 1.0f)) * animScale;

	// Calc vignette
	Vec2 vignetteScreenScale(max(vignette.x, 0.0f), max(vignette.y, 0.0f));

	// Update time (Use UI time as this is animated during menu)
	float frameTime = GetUtils().m_pTimer->GetFrameTime(ITimer::ETIMER_UI);
	m_noiseTime += frameTime * movement1.z;
	m_time += frameTime * movement0.x;

	// Calc mist
	float mistSinStrength = vignette.z;
	float mistAlpha = filter.y;
	float mistScale = fabs_tpl(sin_tpl(m_time)) * mistSinStrength;
	mistAlpha -= clamp_tpl<float>(((mistScale * mistScale * mistScale) * mistAlpha), 0.0f, 1.0f);

	// Corner Glow
	Vec4 cornerGlow = m_pCornerGlow->GetParamVec4();
	float minCornerGlow = cornerGlow.x;
	float maxCornerGlow = cornerGlow.y;
	float maxGlowRandTime = cornerGlow.z;
	m_cornerGlowCountdownTimer -= frameTime;
	if (m_cornerGlowCountdownTimer < 0.0f)
	{
		m_cornerGlowCountdownTimer = cry_random(0.0f, maxGlowRandTime);
		m_cornerGlowStrength = minCornerGlow + LERP(minCornerGlow, maxCornerGlow, cry_random(0.0f, 1.0f));
	}

	// Noise Thresh
	float noiseThresh = 0.83f;
	float overChargeNoiseThresh = 0.83f;
	float hitStrengthNoiseThresh = 0.79f;
	float overChargeHitStrengthNoiseThresh = 0.79f;
	float finalNoiseThresh = LERP(noiseThresh, overChargeNoiseThresh, overChargeStrength);
	float finalHitNoiseThresh = LERP(hitStrengthNoiseThresh, overChargeHitStrengthNoiseThresh, overChargeStrength);
	noiseThresh = LERP(finalNoiseThresh, finalHitNoiseThresh, hitStrength);

	// psParams[0] - x = hexTexScale y = backBufferBrightessScalar, z = vignetteTexOffset, w = vignetteSaturation
	psParams[0] = m_pHexParams->GetParamVec4();
	psParams[0].y = backBufferBrightessScalar;

	// psParams[1] - x = animPos, y = noiseThresh, z = time, w = brightness
	psParams[1].x = animPos;
	psParams[1].y = noiseThresh;
	psParams[1].z = m_time;
	psParams[1].w = brightness;

	// psParams[2] - x = noiseTime, y = movementWaveStrength, z = vignetteFallOffScale, w = movementWaveFrequency
	const float waveStrengthScale = 0.1f; // Use scalar here so post effect param is between 0 and 1
	psParams[2] = movement0;
	psParams[2].x = m_noiseTime;
	psParams[2].y *= waveStrengthScale;

	// psParams[3] - x = movementStrengthX, y = movementStrengthY, z = noiseStrength, w = vignetteStrength
	const float noiseSpeedScale = 0.05f; // Use scalar here so post effect param is between 0 and 1
	psParams[3] = movement1;
	psParams[3].z = filter.z;

	// psParams[4] - x = effectAlpha, y = mistAlpha, zw = vignetteScreenScale
	psParams[4].x = effectAlpha;
	psParams[4].y = mistAlpha;
	psParams[4].z = vignetteScreenScale.x;
	psParams[4].w = vignetteScreenScale.y;

	// psParams[5] - xyz = hex Color, w = hitStrength
	psParams[5] = hexColor;
	psParams[5].w = hitStrength;

	// psParams[6] - x = overChargeStrength, y = cornerGlowStrength, zw = free
	psParams[6].x = overChargeStrength;
	psParams[6].y = m_cornerGlowStrength * overChargeStrength;
	psParams[6].z = 0.0f;
	psParams[6].w = 0.0f;

	// Setup shader
	gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);
	CShaderMan::s_shPostEffects->FXSetPSFloat(m_paramName, psParams, PARAM_COUNT);

	const uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
	SShaderTechnique* pShaderTech = CShaderMan::s_shPostEffectsGame->mfFindTechnique(m_shaderTechName);
	SRenderData& pRenderData = m_pRenderData[nThreadID];

	if (pRenderData.pRenderElement && pShaderTech)
	{
		// Draw 3d mesh in 2d

		// Set scissor
		gcpRendD3D->FX_Commit();
		gcpRendD3D->EF_Scissor(true, 0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

		// Render state
		int32 nRenderState = GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

		// Set intro flag
		const bool bIntroPlaying = ((introAnimPos > 0.0f) && (introAnimPos < 1.0f)) ? true : false;
		const bool bOutroPlaying = ((outroAnimPos > 0.0f) && (outroAnimPos < 1.0f)) ? true : false;
		const bool bAnimPlaying = bIntroPlaying || bOutroPlaying;
		if (bAnimPlaying)
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
		}

		// Set overcharge strength
		if (overChargeStrength > 0.0f)
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		}

		// Set movement wave flag
		if (psParams[2].y > 0.0f)
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
		}

		// Set back buffer brightness scalar flag
		const bool bUsingBackBuffer = (backBufferBrightessScalar > 0.0f) ? true : false;
		if (bUsingBackBuffer)
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
		}

#ifndef _RELEASE
		// Turn on debug view in shader
		if (bDebugPass && CRenderer::CV_r_PostProcessNanoGlassDebugView)
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
			if (CRenderer::CV_r_PostProcessNanoGlassDebugView == eNANO_GLASS_DEBUG_VIEW_WireFrame)
			{
				nRenderState |= GS_WIREFRAME;
			}
		}
#endif

		GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, m_shaderTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		// Set view proj matrix
		Matrix44A mObjCurr, mViewProj;
		C3DHud* p3DHUD = reinterpret_cast<C3DHud*>(PostEffectMgr()->GetEffect(ePFX_3DHUD));
		float fHUDFov = 55.0f;  // Safe default
		if (p3DHUD)
		{
			CEffectParam* pFov = p3DHUD->GetFov();
			if (pFov)
			{
				fHUDFov = clamp_tpl<float>(pFov->GetParam(), 1.0f, 180.0f);
			}
		}

		// Precalculate the pixel aspect ratio for the nano glass on PC so as to better fit it to the screen
		float fBackBufferRatio = (float)CTexture::s_ptexBackBuffer->GetWidth() / (float)CTexture::s_ptexBackBuffer->GetHeight();
		float f16_9 = 16.0f / 9.0f;
		float fInvPAR = f16_9 / fBackBufferRatio;

		// Patch projection matrix to have HUD FOV
		mViewProj = gRenDev->m_ProjMatrix;

		// Apply overscan border aspect ratio
		const float aspect = GetUtils().GetOverscanBorderAspectRatio() * fInvPAR;

		float h = 1 / tanf(DEG2RAD(fHUDFov / 2));
		mViewProj.m00 = h / aspect;
		mViewProj.m11 = h;

		// Render in camera space to remove precision bugs
		const bool bCameraSpace = (pRenderData.pRenderObject->m_ObjFlags & FOB_NEAREST) ? true : false;
		Matrix44A mView;
		GetUtils().GetViewMatrix(mView, bCameraSpace);

		mViewProj = mView * mViewProj;
		mObjCurr = pRenderData.pRenderObject->m_II.m_Matrix.GetTransposed();
		mViewProj = mObjCurr * mViewProj;
		mViewProj.Transpose();

		CShaderMan::s_shPostEffectsGame->FXSetVSFloat(m_viewProjMatrixParamName, (Vec4*) mViewProj.GetData(), 4);

		// Set render state
		gcpRendD3D->FX_SetState(nRenderState);
		gcpRendD3D->SetCullMode(R_CULL_BACK);

		// Set ps params
		CShaderMan::s_shPostEffects->FXSetPSFloat(m_paramName, psParams, PARAM_COUNT);

		// Set textures
		CTexture* pHudMaskTex = bIsHudRendering ? m_pHudMask : CTexture::s_ptexBlackAlpha;

		GetUtils().SetTexture(m_pHexOutline, 0, FILTER_LINEAR, TADDR_MIRROR);
		GetUtils().SetTexture(pHudMaskTex, 1, FILTER_LINEAR, TADDR_CLAMP, true);
		GetUtils().SetTexture(m_pNoise, 2, FILTER_LINEAR, TADDR_WRAP);

		if (bUsingBackBuffer)
		{
			GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[2], 3, FILTER_LINEAR, TADDR_CLAMP);
		}

		if (bAnimPlaying)
		{
			GetUtils().SetTexture(m_pHexRand, 4, FILTER_LINEAR, TADDR_WRAP);
			GetUtils().SetTexture(m_pHexGrad, 5, FILTER_LINEAR, TADDR_WRAP);
		}

		// Draw mesh
		CD3D9Renderer* const __restrict rd = gcpRendD3D;
		CREMeshImpl* pRenderMesh = (CREMeshImpl*) pRenderData.pRenderElement;

		// Create/Update vertex buffer stream
		if (pRenderMesh->m_pRenderMesh)
			pRenderMesh->m_pRenderMesh->CheckUpdate(pRenderMesh->m_pRenderMesh->_GetVertexFormat(), 0);

		gcpRendD3D->m_RP.m_pRE = const_cast<CRenderElement*>(pRenderData.pRenderElement);
		if (gcpRendD3D->FX_CommitStreams(&pShaderTech->m_Passes[0], true))
		{
			gcpRendD3D->m_RP.m_FirstVertex = pRenderMesh->m_nFirstVertId;
			gcpRendD3D->m_RP.m_FirstIndex = pRenderMesh->m_nFirstIndexId;
			gcpRendD3D->m_RP.m_RendNumIndices = pRenderMesh->m_nNumIndices;
			gcpRendD3D->m_RP.m_RendNumVerts = pRenderMesh->m_nNumVerts;
			gcpRendD3D->m_RP.m_pRE->mfDraw(CShaderMan::s_shPostEffectsGame, &pShaderTech->m_Passes[0]);
		}

		gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);

		GetUtils().ShEndPass();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenBlood::Render()
{
	PROFILE_LABEL_SCOPE("SCREEN BLOOD");

	static CCryNameTSCRC pTechName("ScreenBlood");
	GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);
	gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_DSTCOL | GS_BLDST_SRCALPHA);

	// Border params
	const Vec4 borderParams = m_pBorder->GetParamVec4();
	const float borderRange = borderParams.z;
	const Vec2 borderOffset = Vec2(borderParams.x, borderParams.y);
	const float alpha = borderParams.w;

	// Use overscan borders to scale blood thickness around screen
	const float overscanScalar = 3.0f;
	Vec2 overscanBorders = Vec2(0.0f, 0.0f);
	gcpRendD3D->EF_Query(EFQ_OverscanBorders, overscanBorders);
	overscanBorders = Vec2(1.0f, 1.0f) + ((overscanBorders + borderOffset) * overscanScalar);

	const float borderScale = max(0.2f, borderRange - (m_pAmount->GetParam() * borderRange));
	Vec4 pParams = Vec4(overscanBorders.x, overscanBorders.y, alpha, borderScale);
	static CCryNameR pParamName("psParams");
	CShaderMan::s_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
	GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

	GetUtils().ShEndPass();

	//m_nRenderFlags = PSP_UPDATE_BACKBUFFER;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
