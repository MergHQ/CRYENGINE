// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScreenSpaceObscurance.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "D3D_SVO.h"

void CScreenSpaceObscuranceStage::Init()
{
	m_samplerPoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
	m_samplerLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
	m_samplerPointWrap = CTexture::GetTexState(STexState(FILTER_POINT, false));
}

void CScreenSpaceObscuranceStage::Execute(ShadowMapFrustum* pHeightMapFrustum, CTexture* pHeightMapAOScreenDepthTex, CTexture* pHeightMapAOTex)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (!CRenderer::CV_r_ssdo)
	{
		rd->FX_ClearTarget(CTexture::s_ptexSceneNormalsBent, Clr_Median);
		return;
	}

	PROFILE_LABEL_SCOPE("DIRECTIONAL_OCC");

#if defined(DURANGO_USE_ESRAM)
	m_passCopyFromESRAM.Execute(CTexture::s_ptexSceneSpecularESRAM, CTexture::s_ptexSceneSpecular);
#endif

	CTexture* pDestRT = CTexture::s_ptexStereoR;
#if defined(DURANGO_USE_ESRAM)
	pDestRT = CTexture::s_ptexSceneSpecularESRAM;
#endif

	const bool bLowResOutput = (CRenderer::CV_r_ssdoHalfRes == 3);
	if (bLowResOutput)
		pDestRT = CTexture::s_ptexBackBufferScaled[0];

	// Obscurance generation
	{
		CShader* pShader = CShaderMan::s_shDeferredShading;

		uint64 rtMask = 0;
		rtMask |= CRenderer::CV_r_ssdoHalfRes ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;
		rtMask |= pHeightMapFrustum ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;

		// Extreme magnification as happening with small FOVs will cause banding issues with half-res depth
		if (CRenderer::CV_r_ssdoHalfRes == 2 && RAD2DEG(rd->GetCamera().GetFov()) < 30)
			rtMask &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

		static CCryNameTSCRC techOcclusion("DirOccPass");

		m_passObscurance.SetTechnique(pShader, techOcclusion, rtMask);
		m_passObscurance.SetRenderTarget(0, pDestRT);
		m_passObscurance.SetState(GS_NODEPTHTEST);

		m_passObscurance.SetTextureSamplerPair(0, CTexture::s_ptexSceneNormalsMap, m_samplerPoint);
		m_passObscurance.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, m_samplerPoint);
		m_passObscurance.SetTextureSamplerPair(3, CTexture::s_ptexAOVOJitter, m_samplerPointWrap);
		m_passObscurance.SetTextureSamplerPair(5, bLowResOutput ? CTexture::s_ptexZTargetScaled2 : CTexture::s_ptexZTargetScaled, m_samplerPoint);
		m_passObscurance.SetTextureSamplerPair(11, pHeightMapAOScreenDepthTex ? pHeightMapAOScreenDepthTex : CTexture::s_ptexWhite, m_samplerPoint);
		m_passObscurance.SetTexture(12, pHeightMapAOTex ? pHeightMapAOTex : CTexture::s_ptexWhite);

		static CCryNameR ssdoParamsName("SSDOParams");
		static CCryNameR viewspaceParamName("ViewSpaceParams");
		static CCryNameR camMatrixName("SSDO_CameraMatrix");
		static CCryNameR camMatrixInvName("SSDO_CameraMatrixInv");
		static CCryNameR paramsHMAOName("HMAO_Params");

		m_passObscurance.BeginConstantUpdate();

		float radius = CRenderer::CV_r_ssdoRadius / rd->GetRCamera().fFar;
#if defined(FEATURE_SVO_GI)
		if (CSvoRenderer::GetInstance()->IsActive())
			radius *= CSvoRenderer::GetInstance()->GetSsaoAmount();
#endif

		Vec4 param1(radius * 0.5f * rd->m_ProjMatrix.m00, radius * 0.5f * rd->m_ProjMatrix.m11,
		            CRenderer::CV_r_ssdoRadiusMin, CRenderer::CV_r_ssdoRadiusMax);
		pShader->FXSetPSFloat(ssdoParamsName, &param1, 1);

		Vec4 viewSpaceParam(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
		pShader->FXSetPSFloat(viewspaceParamName, &viewSpaceParam, 1);

		Matrix44A matView = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();
		// Adjust the camera matrix so that the camera space will be: +y = down, +z - towards, +x - right
		Vec3 zAxis = matView.GetRow(1);
		matView.SetRow(1, -matView.GetRow(2));
		matView.SetRow(2, zAxis);
		float z = matView.m13;
		matView.m13 = -matView.m23;
		matView.m23 = z;
		pShader->FXSetPSFloat(camMatrixName, (Vec4*)matView.GetData(), 3);

		matView.Invert();
		pShader->FXSetPSFloat(camMatrixInvName, (Vec4*)matView.GetData(), 3);

		if (pHeightMapFrustum)  // Heightmap AO
		{
			Vec4 paramsHMAO(CRenderer::CV_r_HeightMapAOAmount, 1.0f / pHeightMapFrustum->nTexSize, 0, 0);
			pShader->FXSetPSFloat(paramsHMAOName, &paramsHMAO, 1);
		}

		m_passObscurance.Execute();
	}

	// Filtering pass
	if (CRenderer::CV_r_ssdo != 99)
	{
		CShader* pShader = rd->m_cEF.s_ShaderShadowBlur;
		const int32 sizeX = CTexture::s_ptexZTarget->GetWidth();
		const int32 sizeY = CTexture::s_ptexZTarget->GetHeight();
		const int32 srcSizeX = pDestRT->GetWidth();
		const int32 srcSizeY = pDestRT->GetHeight();

		if (m_passFilter.InputChanged((int)bLowResOutput))
		{
			static CCryNameTSCRC techBlur("SSDO_Blur");
			m_passFilter.SetTechnique(pShader, techBlur, 0);
			m_passFilter.SetRenderTarget(0, CTexture::s_ptexSceneNormalsBent);
			m_passFilter.SetState(GS_NODEPTHTEST);
			m_passFilter.SetTextureSamplerPair(0, pDestRT, m_samplerLinear);
			m_passFilter.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, m_samplerPoint);
		}

		static CCryNameR pixelOffsetName("PixelOffset");
		static CCryNameR blurOffsetName("BlurOffset");
		static CCryNameR blurKernelName("SSAO_BlurKernel");

		m_passFilter.BeginConstantUpdate();

		Vec4 v(0, 0, (float)srcSizeX, (float)srcSizeY);
		pShader->FXSetVSFloat(pixelOffsetName, &v, 1);
		v = Vec4(0.5f / (float)sizeX, 0.5f / (float)sizeY, 1.0f / (float)srcSizeX, 1.0f / (float)srcSizeY);
		pShader->FXSetPSFloat(blurOffsetName, &v, 1);
		v = Vec4(2.0f / srcSizeX, 0, 2.0f / srcSizeY, 10.0f); // w: weight coef
		pShader->FXSetPSFloat(blurKernelName, &v, 1);

		m_passFilter.Execute();
	}
	else  // For debugging
	{
		PostProcessUtils().StretchRect(pDestRT, CTexture::s_ptexSceneNormalsBent);
	}

	if (CRenderer::CV_r_ssdoColorBleeding)
	{
		// Generate low frequency scene albedo for color bleeding (convolution not gamma correct but acceptable)
		m_passAlbedoDownsample0.Execute(CTexture::s_ptexSceneDiffuse, CTexture::s_ptexBackBufferScaled[0]);
		m_passAlbedoDownsample1.Execute(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);
		m_passAlbedoDownsample2.Execute(CTexture::s_ptexBackBufferScaled[1], CTexture::s_ptexAOColorBleed);
		m_passAlbedoBlur.Execute(CTexture::s_ptexAOColorBleed, CTexture::s_ptexBackBufferScaled[0], 1.0f, 4.0f);
	}
}
