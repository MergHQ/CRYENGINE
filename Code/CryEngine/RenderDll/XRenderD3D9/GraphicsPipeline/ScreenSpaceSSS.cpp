// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScreenSpaceSSS.h"
#include "DriverD3D.h"

void CScreenSpaceSSSStage::Init()
{
	m_samplerPoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
	m_samplerLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
}

void CScreenSpaceSSSStage::Execute(CTexture* pIrradianceTex)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (!CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
		return;

	PROFILE_LABEL_SCOPE("SSSSS");

	CShader* pShader = CShaderMan::s_shDeferredShading;

	Vec4 viewSpaceParams(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
	float fProjScaleX = 0.5f * rd->m_ProjMatrix.m00;
	float fProjScaleY = 0.5f * rd->m_ProjMatrix.m11;

	static CCryNameTSCRC techBlur("SSSSS_Blur");
	static CCryNameR viewSpaceParamsName("ViewSpaceParams");
	static CCryNameR blurDirName("SSSBlurDir");

	// Horizontal pass
	{
		if (m_passH.InputChanged(pIrradianceTex->GetTextureID()))
		{
			m_passH.SetTechnique(pShader, techBlur, 0);
			m_passH.SetRenderTarget(0, CTexture::s_ptexSceneTargetR11G11B10F[1]);
			m_passH.SetState(GS_NODEPTHTEST);

			m_passH.SetTextureSamplerPair(0, pIrradianceTex, m_samplerPoint);
			m_passH.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, m_samplerPoint);
			m_passH.SetTextureSamplerPair(2, CTexture::s_ptexSceneNormalsMap, m_samplerPoint);
			m_passH.SetTextureSamplerPair(3, CTexture::s_ptexSceneDiffuse, m_samplerPoint);
			m_passH.SetTextureSamplerPair(4, CTexture::s_ptexSceneSpecular, m_samplerPoint);
		}

		m_passH.BeginConstantUpdate();

		m_passH.SetConstant(eHWSC_Pixel, viewSpaceParamsName, viewSpaceParams);
		m_passH.SetConstant(eHWSC_Pixel, blurDirName, Vec4(fProjScaleX, 0, 0, 0));

		m_passH.Execute();
	}

	// Vertical pass
	{
		if (m_passV.InputChanged(pIrradianceTex->GetTextureID()))
		{
			m_passV.SetTechnique(pShader, techBlur, g_HWSR_MaskBit[HWSR_SAMPLE0]);
			m_passV.SetRenderTarget(0, CTexture::s_ptexHDRTarget);
			m_passV.SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

			m_passV.SetTextureSamplerPair(0, CTexture::s_ptexSceneTargetR11G11B10F[1], m_samplerPoint);
			m_passV.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, m_samplerPoint);
			m_passV.SetTextureSamplerPair(2, CTexture::s_ptexSceneNormalsMap, m_samplerPoint);
			m_passV.SetTextureSamplerPair(3, CTexture::s_ptexSceneDiffuse, m_samplerPoint);
			m_passV.SetTextureSamplerPair(4, CTexture::s_ptexSceneSpecular, m_samplerPoint);
			m_passV.SetTextureSamplerPair(5, pIrradianceTex, m_samplerPoint);
		}

		m_passV.BeginConstantUpdate();

		m_passV.SetConstant(eHWSC_Pixel, viewSpaceParamsName, viewSpaceParams);	
		m_passV.SetConstant(eHWSC_Pixel, blurDirName, Vec4(0, fProjScaleY, 0, 0));

		m_passV.Execute();
	}
}
