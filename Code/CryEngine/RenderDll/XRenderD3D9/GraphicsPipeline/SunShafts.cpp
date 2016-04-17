// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SunShafts.h"

#include "DriverD3D.h"
#include "D3DPostProcess.h"

// TODO: Add support for occlusion query to find out if sky is visible at all

void CSunShaftsStage::Init()
{
	m_samplerPoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
	m_samplerLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
}

bool CSunShaftsStage::IsActive()
{
	return CRenderer::CV_r_sunshafts && CRenderer::CV_r_PostProcess;
}

CTexture* CSunShaftsStage::GetFinalOutputRT()
{
	return gcpRendD3D->m_RP.m_eQuality >= eRQ_High ? CTexture::s_ptexBackBufferScaled[0] : CTexture::s_ptexBackBufferScaled[1];
}

CTexture* CSunShaftsStage::GetTempOutputRT()
{
	return gcpRendD3D->m_RP.m_eQuality >= eRQ_High ? CTexture::s_ptexBackBufferScaledTemp[0] : CTexture::s_ptexBackBufferScaledTemp[1];
}

void CSunShaftsStage::GetCompositionParams(Vec4& params0, Vec4& params1)
{
	CSunShafts* pSunShafts = (CSunShafts*)PostEffectMgr()->GetEffect(ePFX_SunShafts);
	Vec4 params[2];
	pSunShafts->GetSunShaftsParams(params);
	params0 = params[0];
	params1 = params[1];
}

void CSunShaftsStage::Execute()
{
	PROFILE_LABEL_SCOPE("SUNSHAFTS_GEN");

	if (!IsActive())
		return;

	CSunShafts* pSunShafts = (CSunShafts*)PostEffectMgr()->GetEffect(ePFX_SunShafts);
	float rayAttenuation = clamp_tpl<float>(pSunShafts->m_pRaysAttenuation->GetParam(), 0.0f, 10.0f);

	CShader* pShader = CShaderMan::s_shPostSunShafts;
	CTexture* pFinalRT = GetFinalOutputRT();
	CTexture* pTempRT = GetTempOutputRT();

	// Generate mask for sun shafts
	{
		if (m_passShaftsMask.InputChanged())
		{
			static CCryNameTSCRC techMaskGen("SunShaftsMaskGen");
			uint64 rtMask = g_HWSR_MaskBit[HWSR_SAMPLE0];
			m_passShaftsMask.SetTechnique(pShader, techMaskGen, rtMask);
			m_passShaftsMask.SetRenderTarget(0, pFinalRT);
			m_passShaftsMask.SetState(GS_NODEPTHTEST);

			m_passShaftsMask.SetTextureSamplerPair(0, CTexture::s_ptexZTargetScaled, m_samplerPoint);
			m_passShaftsMask.SetTextureSamplerPair(1, CTexture::s_ptexHDRTargetScaled[0], m_samplerPoint);  // TODO
		}

		static CCryNameR nameParams0("texToTexParams0");
		static CCryNameR nameParams1("texToTexParams1");

		m_passShaftsMask.BeginConstantUpdate();

		// Set sample positions (using rotated grid)
		float sampleSize = ((float)CTexture::s_ptexBackBuffer->GetWidth() / (float)pFinalRT->GetWidth()) * 0.5f;
		float s1 = sampleSize / (float)CTexture::s_ptexBackBuffer->GetWidth();
		float t1 = sampleSize / (float)CTexture::s_ptexBackBuffer->GetHeight();
		Vec4 params0 = Vec4(s1 * 0.95f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
		Vec4 params1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);

		pShader->FXSetPSFloat(nameParams0, &params0, 1);
		pShader->FXSetPSFloat(nameParams1, &params1, 1);

		m_passShaftsMask.Execute();
	}

	// Apply local radial blur to mask
	{
		static CCryNameR nameShaftParams("PI_sunShaftsParams");
		static CCryNameR nameSunPos("SunShafts_SunPos");
		static CCryNameR nameViewProj("SunShafts_ViewProj");

		Vec3 sunPos = gEnv->p3DEngine->GetSunDir() * 1000.0f;
		Vec4 sunPosScreen = PostProcessUtils().m_pViewProj * Vec4(sunPos, 1.0f);
		sunPosScreen.x = ((sunPosScreen.x + sunPosScreen.w) * 0.5f) / (1e-6f + sunPosScreen.w);
		sunPosScreen.y = ((-sunPosScreen.y + sunPosScreen.w) * 0.5f) / (1e-6f + sunPosScreen.w);
		sunPosScreen.w = gEnv->p3DEngine->GetSunDirNormalized().dot(PostProcessUtils().m_pViewProj.GetRow(2));

		// Pass 1
		{
			if (m_passShaftsGen0.InputChanged())
			{
				static CCryNameTSCRC techShaftsGen("SunShaftsGen");
				uint64 rtMask = g_HWSR_MaskBit[HWSR_SAMPLE0];
				m_passShaftsGen0.SetTechnique(pShader, techShaftsGen, rtMask);
				m_passShaftsGen0.SetRenderTarget(0, pTempRT);
				m_passShaftsGen0.SetState(GS_NODEPTHTEST);

				m_passShaftsGen0.SetTextureSamplerPair(0, pFinalRT, m_samplerLinear);
			}

			m_passShaftsGen0.BeginConstantUpdate();

			Vec4 shaftParams(0.1f, rayAttenuation, 0, 0);
			pShader->FXSetPSFloat(nameShaftParams, &shaftParams, 1);
			pShader->FXSetVSFloat(nameViewProj, (Vec4*)PostProcessUtils().m_pViewProj.GetData(), 4);
			pShader->FXSetPSFloat(nameSunPos, &sunPosScreen, 1);

			m_passShaftsGen0.Execute();
		}

		// Pass 2
		{
			if (m_passShaftsGen1.InputChanged())
			{
				static CCryNameTSCRC techShaftsGen("SunShaftsGen");
				uint64 rtMask = g_HWSR_MaskBit[HWSR_SAMPLE0];
				m_passShaftsGen1.SetTechnique(pShader, techShaftsGen, rtMask);
				m_passShaftsGen1.SetRenderTarget(0, pFinalRT);
				m_passShaftsGen1.SetState(GS_NODEPTHTEST);

				m_passShaftsGen1.SetTextureSamplerPair(0, pTempRT, m_samplerLinear);
			}

			m_passShaftsGen1.BeginConstantUpdate();

			Vec4 shaftParams(0.025f, rayAttenuation, 0, 0);
			pShader->FXSetPSFloat(nameShaftParams, &shaftParams, 1);
			pShader->FXSetVSFloat(nameViewProj, (Vec4*)PostProcessUtils().m_pViewProj.GetData(), 4);
			pShader->FXSetPSFloat(nameSunPos, &sunPosScreen, 1);

			m_passShaftsGen1.Execute();
		}
	}
}
