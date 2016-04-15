// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PostAA.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

void CPostAAStage::Init()
{
	m_pTexAreaSMAA.Assign_NoAddRef(CTexture::ForName("EngineAssets/ScreenSpace/AreaTex.dds", FT_DONT_STREAM, eTF_Unknown));
	m_pTexSearchSMAA.Assign_NoAddRef(CTexture::ForName("EngineAssets/ScreenSpace/SearchTex.dds", FT_DONT_STREAM, eTF_Unknown));
	m_lastFrameID = -1;
}

void CPostAAStage::ApplySMAA(CTexture*& pCurrRT)
{
	CD3D9Renderer* rd = gcpRendD3D;

	CTexture* pEdgesTex = CTexture::s_ptexSceneNormalsMap;  // Reusing ESRAM resident target
	CTexture* pBlendTex = CTexture::s_ptexHDRTarget;        // Reusing ESRAM resident target (FP16 RT accessed using point filtering which gives full rate on GCN)

	if (!pEdgesTex || !pBlendTex)
		return;

	CShader* pShader = CShaderMan::s_shPostAA;
	const int width = rd->GetWidth();
	const int height = rd->GetHeight();

	// 1st pass: Edge detection
	{
		rd->FX_ClearTarget(pEdgesTex, Clr_Transparent);
		rd->FX_PushRenderTarget(0, pEdgesTex, &rd->m_DepthBufferOrig);

		static CCryNameTSCRC techLumaEdgeDetect("LumaEdgeDetectionSMAA");
		static const CCryNameR pPostAAParams("PostAAParams");

		rd->RT_SetViewport(0, 0, width, height);

		GetUtils().ShBeginPass(pShader, techLumaEdgeDetect, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		rd->FX_SetState(GS_NODEPTHTEST);
		if (CRenderer::CV_r_AntialiasingModeSCull)
			GetUtils().BeginStencilPrePass(false, true);

		GetUtils().SetTexture(pCurrRT, 0, FILTER_POINT);
		SD3DPostEffectsUtils::DrawFullScreenTriWPOS(width, height);

		GetUtils().ShEndPass();

		GetUtils().EndStencilPrePass();

		rd->FX_PopRenderTarget(0);
	}

	// 2nd pass: Generate blend texture
	{
		rd->FX_ClearTarget(pBlendTex, Clr_Transparent);
		rd->FX_PushRenderTarget(0, pBlendTex, &rd->m_DepthBufferOrig);

		static CCryNameTSCRC pszBlendWeightTechName("BlendWeightSMAA");
		GetUtils().ShBeginPass(pShader, pszBlendWeightTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		rd->FX_SetState(GS_NODEPTHTEST);

		if (CRenderer::CV_r_AntialiasingModeSCull)
			rd->FX_StencilTestCurRef(true, false);

		GetUtils().SetTexture(pEdgesTex, 0, FILTER_LINEAR);
		GetUtils().SetTexture(m_pTexAreaSMAA, 1, FILTER_LINEAR);
		GetUtils().SetTexture(m_pTexSearchSMAA, 2, FILTER_POINT);

		SD3DPostEffectsUtils::DrawFullScreenTriWPOS(width, height);

		GetUtils().ShEndPass();

		rd->FX_PopRenderTarget(0);
	}

	// Final pass: Blend neighborhood pixels
	{
		CTexture* pDestRT = CTexture::s_ptexSceneNormalsMap;
		rd->FX_PushRenderTarget(0, pDestRT, NULL);

		if (CRenderer::CV_r_AntialiasingModeSCull)
			rd->FX_StencilTestCurRef(false);

		static CCryNameTSCRC pszBlendNeighborhoodTechName("NeighborhoodBlendingSMAA");
		GetUtils().ShBeginPass(pShader, pszBlendNeighborhoodTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		rd->FX_SetState(GS_NODEPTHTEST);
		GetUtils().SetTexture(pBlendTex, 0, FILTER_POINT);
		GetUtils().SetTexture(pCurrRT, 1, FILTER_LINEAR);

		SD3DPostEffectsUtils::DrawFullScreenTriWPOS(width, height);

		GetUtils().ShEndPass();

		rd->FX_PopRenderTarget(0);
		pCurrRT = pDestRT;
	}
}

void CPostAAStage::ApplyTemporalAA(CTexture*& pCurrRT, CTexture*& pMgpuRT, uint32 aaMode)
{
	CD3D9Renderer* rd = gcpRendD3D;
	CShader* pShader = CShaderMan::s_shPostAA;

	if ((aaMode & eAT_SMAA_MASK) && (aaMode & eAT_REQUIRES_PREVIOUSFRAME_MASK) || (aaMode & eAT_FXAA_MASK))  // TODO: Relocate FXAA
	{
		CTexture* pDestRT = GetUtils().GetTaaRT(true);
		(pMgpuRT = pDestRT)->MgpuResourceUpdate(true);

		CTexture* pPrevRT = ((SPostEffectsUtils::m_iFrameCounter - m_lastFrameID) < 10) ? GetUtils().GetTaaRT(false) : pCurrRT;

		rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4]);
		if (aaMode & (eAT_SMAA_1TX_MASK))
			rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
		if (aaMode & eAT_FXAA_MASK)
			rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

		const f32 fWidthRcp = 1.0f / (float)rd->GetWidth();
		const f32 fHeightRcp = 1.0f / (float)rd->GetHeight();

		rd->FX_PushRenderTarget(0, pDestRT, NULL);
		static CCryNameTSCRC techName("PostAA");
		GetUtils().ShBeginPass(pShader, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		rd->SetCullMode(R_CULL_NONE);
		rd->FX_SetState(GS_NODEPTHTEST);

		static CCryNameR viewProjPrevName("mViewProjPrev");
		const Matrix44A mViewProjPrev = CMotionBlur::GetPrevView() * GetUtils().m_pProj * GetUtils().m_pScaleBias;
		pShader->FXSetPSFloat(viewProjPrevName, (Vec4*)mViewProjPrev.GetData(), 4);

		const Vec4 vRcpFrameOpt(-0.33f * fWidthRcp, -0.33f * fHeightRcp, 0.33f * fWidthRcp, 0.33f * fHeightRcp);// (1.0/sz.xy) * -0.33, (1.0/sz.xy) * 0.33. 0.5 -> softer
		const Vec4 vRcpFrameOpt2(-2.0f * fWidthRcp, -2.0f * fHeightRcp, 2.0f * fWidthRcp, 2.0f * fHeightRcp);// (1.0/sz.xy) * -2.0, (1.0/sz.xy) * 2.0
		static CCryNameR pRcpFrameOptParam("RcpFrameOpt");
		pShader->FXSetPSFloat(pRcpFrameOptParam, &vRcpFrameOpt, 1);
		static CCryNameR pRcpFrameOpt2Param("RcpFrameOpt2");
		pShader->FXSetPSFloat(pRcpFrameOpt2Param, &vRcpFrameOpt2, 1);

		static CCryNameR paramName("vParams");
		const Vec4 vParams = Vec4(max(CRenderer::CV_r_AntialiasingTAASharpening + 1.0f, 1.0f), 0.0f, CRenderer::CV_r_AntialiasingTAAFalloffLowFreq + 1e-6f, CRenderer::CV_r_AntialiasingTAAFalloffHiFreq + 1e-6f);
		pShader->FXSetPSFloat(paramName, &vParams, 1);

		// Compute reprojection matrix with highest possible precision to minimize numeric diffusion
		// TODO: Make sure NEAREST projection is handled correctly
		Matrix44_tpl<f64> mReprojection64;
		{
			Matrix44A mProj = GetUtils().m_pProj;

			// Ensure jittering is removed from projection matrix
			assert(rd->GetS3DRend().GetStereoMode() == STEREO_MODE_DUAL_RENDERING || (mProj.m20 == 0 && mProj.m21 == 0));

			Matrix44_tpl<f64> mViewInv, mProjInv;
			mathMatrixLookAtInverse(&mViewInv, &GetUtils().m_pView);
			const bool bCanInvert = mathMatrixPerspectiveFovInverse(&mProjInv, &mProj);
			assert(bCanInvert);

			mReprojection64 = mProjInv * mViewInv * Matrix44_tpl<f64>(CMotionBlur::GetPrevView()) * Matrix44_tpl<f64>(mProj);

			Matrix44_tpl<f64> mScaleBias1 = Matrix44_tpl<f64>(
			  0.5, 0, 0, 0,
			  0, -0.5, 0, 0,
			  0, 0, 1, 0,
			  0.5, 0.5, 0, 1);

			Matrix44_tpl<f64> mScaleBias2 = Matrix44_tpl<f64>(
			  2.0, 0, 0, 0,
			  0, -2.0, 0, 0,
			  0, 0, 1, 0,
			  -1.0, 1.0, 0, 1);

			mReprojection64 = mScaleBias2 * mReprojection64 * mScaleBias1;
		}

		static CCryNameR reprojectionName("mReprojection");
		const Matrix44 mReprojection = mReprojection64;
		pShader->FXSetPSFloat(reprojectionName, (Vec4*)mReprojection.GetData(), 4);

		assert((pCurrRT->GetFlags() & FT_USAGE_ALLOWREADSRGB) && (pPrevRT->GetFlags() & FT_USAGE_ALLOWREADSRGB));

		GetUtils().SetTexture(pCurrRT, 0, FILTER_LINEAR);
		GetUtils().SetTexture(pPrevRT, 1, FILTER_LINEAR);
		GetUtils().SetTexture(CTexture::s_ptexZTarget, 2, FILTER_POINT);
		GetUtils().SetTexture(GetUtils().GetVelocityObjectRT(), 3, FILTER_POINT);
		GetUtils().SetTexture(pCurrRT, 4, FILTER_LINEAR, 1, true);
		GetUtils().SetTexture(pPrevRT, 5, FILTER_LINEAR, 1, true);

		D3DShaderResource* pDepthReadOnlySRV = rd->m_DepthBufferOrigMSAA.pTexture->GetDeviceDepthReadOnlySRV(0, -1, false);
		rd->m_DevMan.BindSRV(CDeviceManager::TYPE_PS, &pDepthReadOnlySRV, 16, 1);

		SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

		ID3D11ShaderResourceView* pNullSRV[1] = { NULL };
		rd->m_DevMan.BindSRV(CDeviceManager::TYPE_PS, pNullSRV, 16, 1);
		rd->FX_Commit();

		GetUtils().ShEndPass();

		rd->FX_PopRenderTarget(0);
		pCurrRT = pDestRT;

		m_lastFrameID = SPostEffectsUtils::m_iFrameCounter;
		rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);
	}
}

void CPostAAStage::DoFinalComposition(CTexture*& pCurrRT, uint32 aaMode)
{
	CD3D9Renderer* rd = gcpRendD3D;

	PROFILE_LABEL_SCOPE("FLARES, GRAIN");

	if (!CRenderer::CV_r_AntialiasingMode)
		GetUtils().CopyScreenToTexture(pCurrRT);

	rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4]);

	if (aaMode & eAT_SMAA_2TX_MASK)
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];

	if (rd->m_RP.m_PersFlags2 & RBPF2_LENS_OPTICS_COMPOSITE)
	{
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		if (CRenderer::CV_r_FlaresChromaShift > 0.5f / (float) rd->GetWidth())  // only relevant if bigger than half pixel
			rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
	}

	if (CRenderer::CV_r_colorRangeCompression)
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

	static CCryNameTSCRC techName("PostAAComposites");

	GetUtils().ShBeginPass(CShaderMan::s_shPostAA, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	float sharpening = CRenderer::CV_r_AntialiasingTAASharpening;

	if (max(fabs(rd->m_vProjMatrixSubPixoffset.x), fabs(rd->m_vProjMatrixSubPixoffset.y)) > 0)
	{
		// Apply stronger unsharp masking when averaging jittered samples (tweaked for 2 samples)
		sharpening *= 2;
	}

	static CCryNameR paramsName("vParams");
	const Vec4 params(max(1.0f + sharpening, 1.0f), 0, 0, 0);
	CShaderMan::s_shPostAA->FXSetPSFloat(paramsName, &params, 1);

	CTexture* pLensOpticsComposite = CTexture::s_ptexSceneTargetR11G11B10F[0];
	GetUtils().SetTexture(pLensOpticsComposite, 5, FILTER_POINT);
	if (rd->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_SAMPLE3])
	{
		const Vec4 lensOpticsParam(1.0f, 1.0f, 1.0f, CRenderer::CV_r_FlaresChromaShift);
		static CCryNameR lensOpticsName("vLensOpticsParams");
		CShaderMan::s_shPostAA->FXSetPSFloat(lensOpticsName, &lensOpticsParam, 1);
	}

	// Apply grain (unfortunately final luminance texture doesn't get its final value baked, so have to replicate entire hdr eye adaption)
	{
		Vec4 vHDRSetupParams[5];
		gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

		CEffectParam* m_pFilterGrainAmount = PostEffectMgr()->GetByName("FilterGrain_Amount");
		CEffectParam* m_pFilterArtifactsGrain = PostEffectMgr()->GetByName("FilterArtifacts_Grain");
		const float fFiltersGrainAmount = max(m_pFilterGrainAmount->GetParam(), m_pFilterArtifactsGrain->GetParam());
		const Vec4 v = Vec4(0, 0, 0, max(fFiltersGrainAmount, max(vHDRSetupParams[1].w, CRenderer::CV_r_HDRGrainAmount)));
		static CCryNameR szHDRParam("HDRParams");
		CShaderMan::s_shPostAA->FXSetPSFloat(szHDRParam, &v, 1);
		static CCryNameR szHDREyeAdaptationParam("HDREyeAdaptation");
		CShaderMan::s_shPostAA->FXSetPSFloat(szHDREyeAdaptationParam, &vHDRSetupParams[4], 1);

		GetUtils().SetTexture(CTexture::s_ptexFilmGrainMap, 6, FILTER_POINT, 0);
		if (CTexture::s_ptexCurLumTexture && !CRenderer::CV_r_HDRRangeAdapt)
			GetUtils().SetTexture(CTexture::s_ptexCurLumTexture, 7, FILTER_POINT);
	}

	GetUtils().SetTexture(pCurrRT, 0, FILTER_LINEAR);
	SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
	GetUtils().ShEndPass();
}

void CPostAAStage::Execute()
{
	// TODO: Handle rapid camera position changes in a better way

	PROFILE_LABEL_SCOPE("POST_AA");

	CD3D9Renderer* rd = gcpRendD3D;
	CTexture* pCurrRT = CTexture::s_ptexSceneDiffuse;
	CTexture* pMgpuRT = NULL;

	uint32 aaMode = rd->FX_GetAntialiasingType();

	if (rd->IsHDRModeEnabled() && (aaMode & (eAT_SMAA_MASK | eAT_FXAA_MASK | eAT_MSAA_MASK)) && pCurrRT)
	{
		rd->FX_PopRenderTarget(0);
	}
	rd->FX_SetActiveRenderTargets();

	uint64 prevRTFlags = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA;

	if (aaMode & eAT_SMAA_MASK)
	{
		ApplySMAA(pCurrRT);
	}

	rd->SetFullscreenViewport();

	ApplyTemporalAA(pCurrRT, pMgpuRT, aaMode);

	rd->SetCurDownscaleFactor(Vec2(1, 1));
	rd->RT_SetViewport(0, 0, rd->GetWidth(), rd->GetHeight());

	DoFinalComposition(pCurrRT, aaMode);

	rd->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA;
	CTexture::s_ptexBackBuffer->SetResolved(true);
	rd->m_RP.m_FlagsShader_RT = prevRTFlags;

	if (pMgpuRT)
	{
		pMgpuRT->MgpuResourceUpdate(false);
	}
}
