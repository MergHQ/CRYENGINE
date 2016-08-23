// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "UtilityPasses.h"
#include "FullscreenPass.h"
#include "DriverD3D.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CStretchRectPass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CStretchRectPass::Execute(CTexture* pSrcRT, CTexture* pDestRT)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (pSrcRT == NULL || pDestRT == NULL)
		return;

	PROFILE_LABEL_SCOPE("STRETCHRECT");

	bool bResample = pSrcRT->GetWidth() != pDestRT->GetWidth() || pSrcRT->GetHeight() != pDestRT->GetHeight();
	const D3DFormat destFormat = CTexture::DeviceFormatFromTexFormat(pDestRT->GetDstFormat());
	const D3DFormat srcFormat = CTexture::DeviceFormatFromTexFormat(pSrcRT->GetDstFormat());

	if (!bResample && destFormat == srcFormat)
	{
		rd->GetDeviceContext().CopyResource(pDestRT->GetDevTexture()->GetBaseTexture(), pSrcRT->GetDevTexture()->GetBaseTexture());
		return;
	}

	if (!m_pass.InputChanged(pSrcRT->GetTextureID(), pDestRT->GetTextureID()))
	{
		m_pass.Execute();
		return;
	}

	static CCryNameTSCRC techTexToTex("TextureToTexture");
	static CCryNameTSCRC techTexToTexResampled("TextureToTextureResampled");

	m_pass.SetRenderTarget(0, pDestRT);
	m_pass.SetTechnique(CShaderMan::s_shPostEffects, bResample ? techTexToTexResampled : techTexToTex, 0);
	m_pass.SetState(GS_NODEPTHTEST);
	int texFilter = CTexture::GetTexState(STexState(bResample ? FILTER_LINEAR : FILTER_POINT, true));
	m_pass.SetTextureSamplerPair(0, pSrcRT, texFilter);

	static CCryNameR param0Name("texToTexParams0");
	static CCryNameR param1Name("texToTexParams1");

	const bool bBigDownsample = false;  // TODO
	CTexture* pOffsetTex = bBigDownsample ? pDestRT : pSrcRT;

	float s1 = 0.5f / (float) pOffsetTex->GetWidth();  // 2.0 better results on lower res images resizing
	float t1 = 0.5f / (float) pOffsetTex->GetHeight();

	Vec4 params0, params1;
	if (bBigDownsample)
	{
		// Use rotated grid + middle sample (~Quincunx)
		params0 = Vec4(s1 * 0.96f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
		params1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);
	}
	else
	{
		// Use box filtering (faster - can skip bilinear filtering, only 4 taps)
		params0 = Vec4(-s1, -t1, s1, -t1);
		params1 = Vec4(s1, t1, -s1, t1);
	}

	m_pass.BeginConstantUpdate();
	m_pass.SetConstant(param0Name, params0, eHWSC_Pixel);
	m_pass.SetConstant(param1Name, params1, eHWSC_Pixel);
	m_pass.Execute();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CStableDownsamplePass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CStableDownsamplePass::Execute(CTexture* pSrcRT, CTexture* pDestRT, bool bKillFireflies)
{
	PROFILE_LABEL_SCOPE("DOWNSAMPLE_STABLE");

	if (!pSrcRT || !pDestRT)
		return;

	if (!m_pass.InputChanged(pSrcRT->GetTextureID(), pDestRT->GetTextureID(), (int)bKillFireflies))
	{
		m_pass.Execute();
		return;
	}

	static CCryNameTSCRC techName("DownsampleStable");

	m_pass.SetRenderTarget(0, pDestRT);
	m_pass.SetTechnique(CShaderMan::s_shPostEffects, techName, bKillFireflies ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0);
	m_pass.SetState(GS_NODEPTHTEST);
	m_pass.SetTextureSamplerPair(0, pSrcRT, CTexture::GetTexState(STexState(FILTER_LINEAR, true)));
	m_pass.BeginConstantUpdate();
	m_pass.Execute();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDepthDownsamplePass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDepthDownsamplePass::Execute(CTexture* pSrcRT, CTexture* pDestRT, bool bLinearizeSrcDepth, bool bFromSingleChannel)
{
	PROFILE_LABEL_SCOPE("DOWNSAMPLE_DEPTH");

	if (!pSrcRT || !pDestRT)
		return;

	if (!m_pass.InputChanged(pSrcRT->GetTextureID(), pDestRT->GetTextureID(), (int)bLinearizeSrcDepth, (int)bFromSingleChannel))
	{
		m_pass.Execute();
		return;
	}

	static CCryNameTSCRC techName("DownsampleDepth");

	uint64 rtMask = 0;
	rtMask |= bLinearizeSrcDepth ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;
	rtMask |= bFromSingleChannel ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;

	m_pass.SetRenderTarget(0, pDestRT);
	m_pass.SetTechnique(CShaderMan::s_shPostEffects, techName, rtMask);
	m_pass.SetState(GS_NODEPTHTEST);
	m_pass.SetRequirePerViewConstantBuffer(true);
	m_pass.SetTextureSamplerPair(0, pSrcRT, CTexture::GetTexState(STexState(FILTER_POINT, true)));

	static CCryNameR paramName("DownsampleDepth_Params");

	m_pass.BeginConstantUpdate();

	// Handle uneven source size by dropping last row/column
	Vec4 params(0, 0, 0, 0);
	int scaledWidth = (pSrcRT->GetWidth() + 1) / 2;
	int scaledHeight = (pSrcRT->GetHeight() + 1) / 2;
	params.x = (float)pDestRT->GetWidth() / (float)scaledWidth;
	params.y = (float)pDestRT->GetHeight() / (float)scaledHeight;
	m_pass.SetConstant( paramName, params, eHWSC_Pixel);
	m_pass.Execute();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CGaussianBlurPass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float CGaussianBlurPass::GaussianDistribution1D(float x, float rho)
{
	float g = 1.0f / (rho * sqrtf(2.0f * PI));
	g *= expf(-(x * x) / (2.0f * rho * rho));
	return g;
}

void CGaussianBlurPass::ComputeParams(int texWidth, int texHeight, int numSamples, float scale, float distribution)
{
	assert(numSamples <= 16);
	const int halfNumSamples = (numSamples >> 1);

	float s1 = 1.0f / (float)texWidth;
	float t1 = 1.0f / (float)texHeight;

	float weights[16];
	float weightSum = 0.0f;

	// Compute Gaussian weights
	for (int s = 0; s < numSamples; ++s)
	{
		if (distribution != 0.0f)
			weights[s] = GaussianDistribution1D((float)(s - halfNumSamples), distribution);
		else
			weights[s] = 0.0f;
		weightSum += weights[s];
	}

	// Normalize weights
	for (int s = 0; s < numSamples; ++s)
	{
		weights[s] /= weightSum;
	}

	// Compute bilinear offsets
	for (int s = 0; s < halfNumSamples; ++s)
	{
		float off_a = weights[s * 2];
		float off_b = ((s * 2 + 1) <= numSamples - 1) ? weights[s * 2 + 1] : 0;
		float a_plus_b = (off_a + off_b);
		if (a_plus_b == 0)
			a_plus_b = 1.0f;
		float offset = off_b / a_plus_b;

		weights[s] = off_a + off_b;
		weights[s] *= scale;
		m_weights[s] = Vec4(1, 1, 1, 1) * weights[s];

		float currOffset = (float)s * 2.0f + offset - (float)halfNumSamples;
		m_paramsH[s] = Vec4(s1 * currOffset, 0, 0, 0);
		m_paramsV[s] = Vec4(0, t1 * currOffset, 0, 0);
	}
}

void CGaussianBlurPass::Execute(CTexture* pScrDestRT, CTexture* pTempRT, float scale, float distribution)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (pScrDestRT == NULL || pTempRT == NULL)
		return;

	PROFILE_LABEL_SCOPE("TEXBLUR_GAUSSIAN");

	if (!m_passH.InputChanged(pScrDestRT->GetTextureID(), pTempRT->GetTextureID()) &&
	    !m_passV.InputChanged(pScrDestRT->GetTextureID(), pTempRT->GetTextureID()) &&
	    m_scale == scale && m_distribution == distribution)
	{
		m_passH.Execute();
		m_passV.Execute();
		return;
	}

	CShader* pShader = CShaderMan::s_shPostEffects;
	int texFilter = CTexture::GetTexState(STexState(FILTER_LINEAR, true));

	static CCryNameTSCRC techDefault("GaussBlurBilinear");
	static CCryNameR clampTCName("clampTC");
	static CCryNameR param0Name("psWeights");
	static CCryNameR param1Name("PI_psOffsets");

	Vec4 clampTC(0.0f, 1.0f, 0.0f, 1.0f);
	if (pScrDestRT->GetWidth() == rd->GetWidth() && pScrDestRT->GetHeight() == rd->GetHeight())
	{
		// Clamp manually in shader since texture clamp won't apply for smaller viewport
		clampTC = Vec4(0.0f, rd->m_RP.m_CurDownscaleFactor.x, 0.0f, rd->m_RP.m_CurDownscaleFactor.y);
	}

	const int numSamples = 16;
	if (m_scale != scale || m_distribution != distribution)
	{
		ComputeParams(pScrDestRT->GetWidth(), pScrDestRT->GetHeight(), numSamples, scale, distribution);
		m_scale = scale;
		m_distribution = distribution;
	}

	// Horizontal
	m_passH.SetRenderTarget(0, pTempRT);
	m_passH.SetTechnique(pShader, techDefault, 0);
	m_passH.SetState(GS_NODEPTHTEST);
	m_passH.SetTextureSamplerPair(0, pScrDestRT, texFilter);

	m_passH.BeginConstantUpdate();
	m_passH.SetConstantArray(param1Name, m_paramsH, numSamples / 2, eHWSC_Vertex);
	m_passH.SetConstantArray(param0Name, m_weights, numSamples / 2, eHWSC_Pixel);
	m_passH.SetConstant(clampTCName, clampTC, eHWSC_Pixel);
	m_passH.Execute();

	// Vertical
	m_passV.SetRenderTarget(0, pScrDestRT);
	m_passV.SetTechnique(pShader, techDefault, 0);
	m_passV.SetState(GS_NODEPTHTEST);
	m_passV.SetTextureSamplerPair(0, pTempRT, texFilter);

	m_passV.BeginConstantUpdate();
	m_passV.SetConstantArray(param1Name, m_paramsV, numSamples / 2, eHWSC_Vertex);
	m_passV.SetConstantArray(param0Name, m_weights, numSamples / 2, eHWSC_Pixel);
	m_passV.SetConstant(clampTCName, clampTC, eHWSC_Pixel);
	m_passV.Execute();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMipmapGenPass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMipmapGenPass::Execute(CTexture* pScrDestRT, int mipCount)
{
	static CCryNameTSCRC techDownsample("TextureToTexture");
	const int texFilter = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
	const int numPasses = mipCount == 0 ? pScrDestRT->GetNumMips() - 1 : std::min(pScrDestRT->GetNumMips() - 1, mipCount);

	for (int i = 0; i < numPasses; ++i)
	{
		auto& curPass = m_downsamplePasses[i];

		if (curPass.InputChanged(pScrDestRT->GetID()))
		{
			auto rtv = SResourceView::RenderTargetView(pScrDestRT->GetDstFormat(), 0, -1, i + 1);
			auto srv = SResourceView::ShaderResourceView(pScrDestRT->GetDstFormat(), 0, -1, i, 1);

			curPass.SetRenderTarget(0, pScrDestRT, rtv.m_Desc.Key);
			curPass.SetTechnique(CShaderMan::s_shPostEffects, techDownsample, 0);
			curPass.SetState(GS_NODEPTHTEST);
			curPass.SetTextureSamplerPair(0, pScrDestRT, texFilter, srv.m_Desc.Key);
		}

		curPass.Execute();
	}

#if CRY_USE_DX12
	// Revert state of resource to one coherent resource-state after mip-mapping
	CDeviceCommandListPtr pCommandList = CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
	CCryDX12Resource<ID3D11ResourceToImplement>* DX11res = reinterpret_cast<CCryDX12Resource<ID3D11ResourceToImplement>*>(pScrDestRT->GetDevTexture()->GetBaseTexture());
	NCryDX12::CResource& DX12res = DX11res->GetDX12Resource();
	NCryDX12::CCommandList* DX12cmd = pCommandList->GetDX12CommandList();

	DX12cmd->SetResourceState(DX12res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
#endif
}
