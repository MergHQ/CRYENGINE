// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   D3DHDRRender.cpp : high dynamic range post-processing

    Revision history:
* Created by Honich Andrey
* 01.10.09: Started refactoring (Tiago Sousa)

   =============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include <Cry3DEngine/I3DEngine.h>
#include "../Common/ReverseDepth.h"
#include "GraphicsPipeline/Bloom.h"

//  render targets info - first gather list of hdr targets, sort by size and create after
struct SRenderTargetInfo
{
	SRenderTargetInfo() : nWidth(0), nHeight(0), cClearColor(Clr_Empty), Format(eTF_Unknown), nFlags(0), lplpStorage(0), nPitch(0), fPriority(0.0f), nCustomID(0)
	{};

	uint32      nWidth;
	uint32      nHeight;
	ColorF      cClearColor;
	ETEX_Format Format;
	uint32      nFlags;
	CTexture**  lplpStorage;
	char        szName[64];
	uint32      nPitch;
	float       fPriority;
	int32       nCustomID;
};

struct RenderTargetSizeSort
{
	bool operator()(const SRenderTargetInfo& drtStart, const SRenderTargetInfo& drtEnd) { return (drtStart.nPitch * drtStart.fPriority) > (drtEnd.nPitch * drtEnd.fPriority); }
};

class CHDRPostProcess
{

public:

	CHDRPostProcess()
	{
		nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
		nTexStateLinearWrap = CTexture::GetTexState(STexState(FILTER_LINEAR, false));
		nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
		nTexStatePointWrap = CTexture::GetTexState(STexState(FILTER_POINT, false));
		m_bHiQuality = false;
		m_shHDR = 0;
	}

	~CHDRPostProcess()
	{

	}

	void ScreenShot();

	void SetShaderParams();

	// Tone mapping steps
	void HalfResDownsampleHDRTarget();
	void QuarterResDownsampleHDRTarget();
	void MeasureLuminance();
	void EyeAdaptation();
	void BloomGeneration();
	void ProcessLensOptics();
	void ToneMapping();
	void ToneMappingDebug();
	void DrawDebugViews();

	void Begin();
	void Render();
	void End();

	void AddRenderTarget(uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Format Format, float fPriority, const char* szName, CTexture** pStorage, uint32 nFlags = 0, int32 nCustomID = -1, bool bDynamicTex = 0);
	bool CreateRenderTargetList();
	void ClearRenderTargetList()
	{
		m_pRenderTargets.clear();
	}

	static CHDRPostProcess* GetInstance()
	{
		static CHDRPostProcess pInstance;
		return &pInstance;
	}

private:

	std::vector<SRenderTargetInfo> m_pRenderTargets;
	CShader*                       m_shHDR;

	int32                          nTexStateLinear;
	int32                          nTexStateLinearWrap;
	int32                          nTexStatePoint;
	int32                          nTexStatePointWrap;

	bool                           m_bHiQuality;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::AddRenderTarget(uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Format Format, float fPriority, const char* szName, CTexture** pStorage, uint32 nFlags, int32 nCustomID, bool bDynamicTex)
{
	SRenderTargetInfo drt;

	drt.nWidth = nWidth;
	drt.nHeight = nHeight;
	drt.cClearColor = cClear;
	drt.nFlags = FT_USAGE_RENDERTARGET | FT_DONT_STREAM | nFlags;
	drt.Format = Format;
	drt.fPriority = fPriority;
	drt.lplpStorage = pStorage;
	drt.nCustomID = nCustomID;
	drt.nPitch = nWidth * CTexture::BytesPerPixel(Format);

	cry_strcpy(drt.szName, szName);
	m_pRenderTargets.push_back(drt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CHDRPostProcess::CreateRenderTargetList()
{
	std::sort(m_pRenderTargets.begin(), m_pRenderTargets.end(), RenderTargetSizeSort());

	for (uint32 t = 0; t < m_pRenderTargets.size(); ++t)
	{
		SRenderTargetInfo& drt = m_pRenderTargets[t];
		CTexture* pTex = (*drt.lplpStorage);
		if (!CTexture::IsTextureExist(pTex))
		{
			pTex = CTexture::CreateRenderTarget(drt.szName, drt.nWidth, drt.nHeight, drt.cClearColor, eTT_2D, drt.nFlags, drt.Format, drt.nCustomID);
			if (pTex)
			{
				pTex->Clear();
				(*drt.lplpStorage) = pTex;
			}
		}
		else
		{
			pTex->SetFlags(drt.nFlags);
			pTex->SetWidth(drt.nWidth);
			pTex->SetHeight(drt.nHeight);
			pTex->CreateRenderTarget(drt.Format, drt.cClearColor);
		}
	}

	m_pRenderTargets.clear();

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CTexture::GenerateHDRMaps()
{
	int i;
	char szName[256];

	CD3D9Renderer* r = gcpRendD3D;
	CHDRPostProcess* pHDRPostProcess = CHDRPostProcess::GetInstance();

	const int width  = r->GetWidth(),  width_r2  = (width  + 1) / 2, width_r4  = (width_r2  + 1) / 2, width_r8  = (width_r4  + 1) / 2, width_r16  = (width_r8  + 1) / 2;
	const int height = r->GetHeight(), height_r2 = (height + 1) / 2, height_r4 = (height_r2 + 1) / 2, height_r8 = (height_r4 + 1) / 2, height_r16 = (height_r8 + 1) / 2;
	
	pHDRPostProcess->ClearRenderTargetList();

	ETEX_Format nHDRFormat = eTF_R16G16B16A16F; // note: for main rendertarget R11G11B10 precision/range (even with rescaling) not enough for darks vs good blooming quality

	uint32 nHDRTargetFlags = FT_DONT_RELEASE | (CRenderer::CV_r_msaa ? FT_USAGE_MSAA : 0);
	uint32 nHDRTargetFlagsUAV = nHDRTargetFlags | (CRenderer::CV_r_msaa ? 0 : FT_USAGE_UNORDERED_ACCESS);  // UAV required for tiled deferred shading
	pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, nHDRFormat, 1.0f, "$HDRTarget", &s_ptexHDRTarget, nHDRTargetFlagsUAV);
	pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R11G11B10F, 1.0f, "$HDRTargetPrev", &s_ptexHDRTargetPrev);

	// Scaled versions of the HDR scene texture
	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled0", &s_ptexHDRTargetScaled[0], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp0", &s_ptexHDRTargetScaledTmp[0], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT0", &s_ptexHDRTargetScaledTempRT[0], FT_DONT_RELEASE);

	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled1", &s_ptexHDRTargetScaled[1], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp1", &s_ptexHDRTargetScaledTmp[1], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT1", &s_ptexHDRTargetScaledTempRT[1], 0);

	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRTempBloom0", &s_ptexHDRTempBloom[0], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRTempBloom1", &s_ptexHDRTempBloom[1], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r4, height_r4, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRFinalBloom", &s_ptexHDRFinalBloom, FT_DONT_RELEASE);

	pHDRPostProcess->AddRenderTarget(width_r8, height_r8, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled2", &s_ptexHDRTargetScaled[2], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r8, height_r8, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT2", &s_ptexHDRTargetScaledTempRT[2], FT_DONT_RELEASE);

	pHDRPostProcess->AddRenderTarget(width_r16, height_r16, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled3", &s_ptexHDRTargetScaled[3], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r16, height_r16, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp3", &s_ptexHDRTargetScaledTmp[3], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r16, height_r16, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT3", &s_ptexHDRTargetScaledTempRT[3], FT_DONT_RELEASE);

	for (i = 0; i < 8; i++)
	{
		sprintf(szName, "$HDRAdaptedLuminanceCur_%d", i);
		pHDRPostProcess->AddRenderTarget(1, 1, Clr_Unknown, eTF_R16G16F, 0.1f, szName, &s_ptexHDRAdaptedLuminanceCur[i], FT_DONT_RELEASE);
	}

	pHDRPostProcess->AddRenderTarget(width, height, Clr_Unknown, eTF_R11G11B10F, 1.0f, "$SceneTargetR11G11B10F_0", &s_ptexSceneTargetR11G11B10F[0], nHDRTargetFlagsUAV);
	pHDRPostProcess->AddRenderTarget(width, height, Clr_Unknown, eTF_R11G11B10F, 1.0f, "$SceneTargetR11G11B10F_1", &s_ptexSceneTargetR11G11B10F[1], nHDRTargetFlags);

	pHDRPostProcess->AddRenderTarget(width, height, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$Velocity", &s_ptexVelocity, FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(20, height, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$VelocityTilesTmp0", &s_ptexVelocityTiles[0], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(20, 20, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$VelocityTilesTmp1", &s_ptexVelocityTiles[1], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(20, 20, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$VelocityTiles", &s_ptexVelocityTiles[2], FT_DONT_RELEASE);

	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, nHDRFormat, 0.9f, "$HDRDofLayerNear", &s_ptexHDRDofLayers[0], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, nHDRFormat, 0.9f, "$HDRDofLayerFar", &s_ptexHDRDofLayers[1], FT_DONT_RELEASE);
	pHDRPostProcess->AddRenderTarget(width_r2, height_r2, Clr_Unknown, eTF_R16G16F, 1.0f, "$MinCoC_0_Temp", &s_ptexSceneCoCTemp, FT_DONT_RELEASE);
	for (i = 0; i < MIN_DOF_COC_K; i++)
	{
		cry_sprintf(szName, "$MinCoC_%d", i);
		pHDRPostProcess->AddRenderTarget(width_r2 / (i + 1), height_r2 / (i + 1), Clr_Unknown, eTF_R16G16F, 0.1f, szName, &s_ptexSceneCoC[i], FT_DONT_RELEASE, -1, true);
	}

	// TODO: make it a MIP-mapped resource and/or use compute
	// Luminance rt
	for (i = 0; i < NUM_HDR_TONEMAP_TEXTURES; i++)
	{
		int iSampleLen = 1 << (2 * i);
		cry_sprintf(szName, "$HDRToneMap_%d", i);

		pHDRPostProcess->AddRenderTarget(iSampleLen, iSampleLen, Clr_Dark, eTF_R16G16F, 0.7f, szName, &s_ptexHDRToneMaps[i], FT_DONT_RELEASE | (i == 0 ? FT_STAGE_READBACK : 0));
	}

	s_ptexHDRMeasuredLuminanceDummy = CTexture::CreateTextureObject("$HDRMeasuredLum_Dummy", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_R16G16F, TO_HDR_MEASURED_LUMINANCE);
	for (i = 0; i < MAX_GPU_NUM; ++i)
	{
		cry_sprintf(szName, "$HDRMeasuredLum_%d", i);
		s_ptexHDRMeasuredLuminance[i] = CTexture::Create2DTexture(szName, 1, 1, 0, FT_DONT_RELEASE | FT_DONT_STREAM, NULL, eTF_R16G16F, eTF_R16G16F);
	}

	pHDRPostProcess->CreateRenderTargetList();

	r->m_vSceneLuminanceInfo = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	r->m_fAdaptedSceneScaleLBuffer = r->m_fAdaptedSceneScale = r->m_fScotopicSceneScale = 1.0f;

	// Create resources if necessary - todo: refactor all this shared render targets stuff, quite cumbersome atm...
	SPostEffectsUtils::Create();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CTexture::DestroyHDRMaps()
{
	int i;

	SAFE_RELEASE(s_ptexHDRTarget);
	SAFE_RELEASE(s_ptexHDRTargetPrev);
	SAFE_RELEASE(s_ptexHDRTargetScaled[0]);
	SAFE_RELEASE(s_ptexHDRTargetScaled[1]);
	SAFE_RELEASE(s_ptexHDRTargetScaled[2]);
	SAFE_RELEASE(s_ptexHDRTargetScaled[3]);

	SAFE_RELEASE(s_ptexHDRTargetScaledTmp[0]);
	SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[0]);

	SAFE_RELEASE(s_ptexHDRTargetScaledTmp[1]);
	SAFE_RELEASE(s_ptexHDRTargetScaledTmp[3]);

	SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[1]);
	SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[2]);
	SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[3]);

	SAFE_RELEASE(s_ptexHDRTempBloom[0]);
	SAFE_RELEASE(s_ptexHDRTempBloom[1]);
	SAFE_RELEASE(s_ptexHDRFinalBloom);

	for (i = 0; i < 8; i++)
	{
		SAFE_RELEASE(s_ptexHDRAdaptedLuminanceCur[i]);
	}

	for (i = 0; i < NUM_HDR_TONEMAP_TEXTURES; i++)
	{
		SAFE_RELEASE(s_ptexHDRToneMaps[i]);
	}
	SAFE_RELEASE(s_ptexHDRMeasuredLuminanceDummy);
	for (i = 0; i < MAX_GPU_NUM; ++i)
	{
		SAFE_RELEASE(s_ptexHDRMeasuredLuminance[i]);
	}

	CTexture::s_ptexCurLumTexture = NULL;

	SAFE_RELEASE(s_ptexVelocity);
	SAFE_RELEASE(s_ptexVelocityTiles[0]);
	SAFE_RELEASE(s_ptexVelocityTiles[1]);
	SAFE_RELEASE(s_ptexVelocityTiles[2]);

	SAFE_RELEASE(s_ptexHDRDofLayers[0]);
	SAFE_RELEASE(s_ptexHDRDofLayers[1]);
	SAFE_RELEASE(s_ptexSceneCoCTemp);
	for (i = 0; i < MIN_DOF_COC_K; i++)
	{
		SAFE_RELEASE(s_ptexSceneCoC[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// deprecated
void DrawQuad3D(float s0, float t0, float s1, float t1)
{
	const float fZ = 0.5f;
	const float fX0 = -1.0f, fX1 = 1.0f;
	const float fY0 = 1.0f, fY1 = -1.0f;

	gcpRendD3D->DrawQuad3D(Vec3(fX0, fY1, fZ), Vec3(fX1, fY1, fZ), Vec3(fX1, fY0, fZ), Vec3(fX0, fY0, fZ), Col_White, s0, t0, s1, t1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// deprecated
void DrawFullScreenQuadTR(float xpos, float ypos, float w, float h)
{
	// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
	CryStackAllocWithSizeVector(SVF_P3F_C4B_T2F, 4, vQuad, CDeviceBufferManager::AlignBufferSizeForStreaming);

	const DWORD col = ~0;
	const float s0 = 0;
	const float s1 = 1;
	const float t0 = 1;
	const float t1 = 0;

	// Define the quad
	vQuad[0].xyz = Vec3(xpos, ypos, 1.0f);
	vQuad[0].color.dcolor = col;
	vQuad[0].st = Vec2(s0, 1.0f - t0);

	vQuad[1].xyz = Vec3(xpos + w, ypos, 1.0f);
	vQuad[1].color.dcolor = col;
	vQuad[1].st = Vec2(s1, 1.0f - t0);

	vQuad[3].xyz = Vec3(xpos + w, ypos + h, 1.0f);
	vQuad[3].color.dcolor = col;
	vQuad[3].st = Vec2(s1, 1.0f - t1);

	vQuad[2].xyz = Vec3(xpos, ypos + h, 1.0f);
	vQuad[2].color.dcolor = col;
	vQuad[2].st = Vec2(s0, 1.0f - t1);

	TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(vQuad, 4, 0);

	gcpRendD3D->FX_Commit();

	if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
		gcpRendD3D->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
}

#define NV_CACHE_OPTS_ENABLED TRUE

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// deprecated
bool DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV, bool bClampToScreenRes)
{
	CD3D9Renderer* rd = gcpRendD3D;

	rd->FX_Commit();

	// Acquire render target width and height
	int nWidth = rd->m_NewViewport.nWidth;
	int nHeight = rd->m_NewViewport.nHeight;

	// Ensure that we're directly mapping texels to pixels by offset by 0.5
	// For more info see the doc page titled "Directly Mapping Texels to Pixels"
	if (bClampToScreenRes)
	{
		nWidth = min(nWidth, rd->GetWidth());
		nHeight = min(nHeight, rd->GetHeight());
	}

	float fWidth5 = (float)nWidth;
	float fHeight5 = (float)nHeight;
	fWidth5 = fWidth5 - 0.5f;
	fHeight5 = fHeight5 - 0.5f;

	// Draw the quad
	{
		// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
		CryStackAllocWithSizeVector(SVF_TP3F_C4B_T2F, 4, Verts, CDeviceBufferManager::AlignBufferSizeForStreaming);

		fTopV = 1 - fTopV;
		fBottomV = 1 - fBottomV;
		Verts[0].pos = Vec4(-0.5f, -0.5f, 0.0f, 1.0f);
		Verts[0].color.dcolor = ~0;
		Verts[0].st = Vec2(fLeftU, fTopV);

		Verts[1].pos = Vec4(fWidth5, -0.5f, 0.0f, 1.0f);
		Verts[1].color.dcolor = ~0;
		Verts[1].st = Vec2(fRightU, fTopV);

		Verts[2].pos = Vec4(-0.5f, fHeight5, 0.0f, 1.0f);
		Verts[2].color.dcolor = ~0;
		Verts[2].st = Vec2(fLeftU, fBottomV);

		Verts[3].pos = Vec4(fWidth5, fHeight5, 0.0f, 1.0f);
		Verts[3].color.dcolor = ~0;
		Verts[3].st = Vec2(fRightU, fBottomV);

		TempDynVB<SVF_TP3F_C4B_T2F>::CreateFillAndBind(Verts, 4, 0);

		rd->FX_SetState(GS_NODEPTHTEST);
		if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_TP3F_C4B_T2F)))
			rd->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// deprecated
bool DrawFullScreenQuad(CoordRect c, bool bClampToScreenRes)
{
	return DrawFullScreenQuad(c.fLeftU, c.fTopV, c.fRightU, c.fBottomV, bClampToScreenRes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void GetSampleOffsets_DownScale4x4(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
	float tU = 1.0f / (float)nWidth;
	float tV = 1.0f / (float)nHeight;

	// Sample from the 16 surrounding points. Since the center point will be in
	// the exact center of 16 texels, a 0.5f offset is needed to specify a texel
	// center.
	int index = 0;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			avSampleOffsets[index].x = (x - 1.5f) * tU;
			avSampleOffsets[index].y = (y - 1.5f) * tV;
			avSampleOffsets[index].z = 0;
			avSampleOffsets[index].w = 1;

			index++;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void GetSampleOffsets_DownScale4x4Bilinear(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
	float tU = 1.0f / (float)nWidth;
	float tV = 1.0f / (float)nHeight;

	// Sample from the 16 surrounding points.  Since bilinear filtering is being used, specific the coordinate
	// exactly halfway between the current texel center (k-1.5) and the neighboring texel center (k-0.5)

	int index = 0;
	for (int y = 0; y < 4; y += 2)
	{
		for (int x = 0; x < 4; x += 2, index++)
		{
			avSampleOffsets[index].x = (x - 1.f) * tU;
			avSampleOffsets[index].y = (y - 1.f) * tV;
			avSampleOffsets[index].z = 0;
			avSampleOffsets[index].w = 1;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void GetSampleOffsets_DownScale2x2(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
	float tU = 1.0f / (float)nWidth;
	float tV = 1.0f / (float)nHeight;

	// Sample from the 4 surrounding points. Since the center point will be in
	// the exact center of 4 texels, a 0.5f offset is needed to specify a texel
	// center.
	int index = 0;
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			avSampleOffsets[index].x = (x - 0.5f) * tU;
			avSampleOffsets[index].y = (y - 0.5f) * tV;
			avSampleOffsets[index].z = 0;
			avSampleOffsets[index].w = 1;

			index++;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::SetShaderParams()
{
	CD3D9Renderer* r = gcpRendD3D;

	Vec4 vHDRSetupParams[5];
	gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

	static CCryNameR szHDRMiscParams("HDRMiscParams");
	const Vec4 vHDRMiscParams = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_shHDR->FXSetPSFloat(szHDRMiscParams, &vHDRMiscParams, 1);

	static CCryNameR szHDREyeAdaptationParam("HDREyeAdaptation");
	m_shHDR->FXSetPSFloat(szHDREyeAdaptationParam, CRenderer::CV_r_HDREyeAdaptationMode == 2 ? &vHDRSetupParams[4] : &vHDRSetupParams[3], 1);

	/////////////////////////////////////////////////////////////////////////
	// TOD HDR setup params

	// RGB Film curve setup
	static CCryNameR szHDRFilmCurve("HDRFilmCurve");

	const Vec4 vHDRFilmCurve = vHDRSetupParams[0]; //* Vec4(0.22f, 0.3f, 0.01f, 1.0f);
	m_shHDR->FXSetPSFloat(szHDRFilmCurve, &vHDRFilmCurve, 1);

	static CCryNameR szHDRColorBalance("HDRColorBalance");
	const Vec4 vHDRColorBalance = vHDRSetupParams[2];
	m_shHDR->FXSetPSFloat(szHDRColorBalance, &vHDRColorBalance, 1);

	static CCryNameR szHDRBloomColor("HDRBloomColor");
	const Vec4 vHDRBloomColor = vHDRSetupParams[1] * Vec4(Vec3((1.0f / 8.0f)), 1.0f);   // division by 8.0f was done in shader before, remove this at some point
	m_shHDR->FXSetPSFloat(szHDRBloomColor, &vHDRBloomColor, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::HalfResDownsampleHDRTarget()
{
	PROFILE_LABEL_SCOPE("HALFRES_DOWNSAMPLE_HDRTARGET");

	CTexture* pSrcRT = CTexture::s_ptexHDRTarget;
	CTexture* pDstRT = CTexture::s_ptexHDRTargetScaled[0];

	if (CRenderer::CV_r_HDRBloomQuality >= 2)
		PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, true);
	else
		PostProcessUtils().StretchRect(pSrcRT, pDstRT, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::QuarterResDownsampleHDRTarget()
{
	PROFILE_LABEL_SCOPE("QUARTER_RES_DOWNSAMPLE_HDRTARGET");

	CTexture* pSrcRT = CTexture::s_ptexHDRTargetScaled[0];
	CTexture* pDstRT = CTexture::s_ptexHDRTargetScaled[1];

	if (CRenderer::CV_r_HDRBloomQuality >= 1)
		PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, true);
	else
		PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::MeasureLuminance()
{
	PROFILE_LABEL_SCOPE("MEASURE_LUMINANCE");
	int x, y, index;
	Vec4 avSampleOffsets[16];
	CD3D9Renderer* rd = gcpRendD3D;

	uint64 nFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
	gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

	int32 dwCurTexture = NUM_HDR_TONEMAP_TEXTURES - 1;
	static CCryNameR Param1Name("SampleOffsets");

	float tU = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth());
	float tV = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

	index = 0;
	for (x = -1; x <= 1; x++)
	{
		for (y = -1; y <= 1; y++)
		{
			avSampleOffsets[index].x = x * tU;
			avSampleOffsets[index].y = y * tV;
			avSampleOffsets[index].z = 0;
			avSampleOffsets[index].w = 1;

			index++;
		}
	}

	uint32 nPasses;

	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);
	rd->FX_SetActiveRenderTargets();
	rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

	static CCryNameTSCRC TechName("HDRSampleLumInitial");
	m_shHDR->FXSetTechnique(TechName);
	m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	m_shHDR->FXBeginPass(0);

	CTexture::s_ptexHDRTargetScaled[1]->Apply(0, nTexStateLinear);
	CTexture::s_ptexSceneNormalsMap->Apply(1, nTexStateLinear);
	CTexture::s_ptexSceneDiffuse->Apply(2, nTexStateLinear);
	CTexture::s_ptexSceneSpecular->Apply(3, nTexStateLinear);

	float s1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[1]->GetWidth();
	float t1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[1]->GetHeight();

	// Use rotated grid
	Vec4 vSampleLumOffsets0 = Vec4(s1 * 0.95f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
	Vec4 vSampleLumOffsets1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);

	static CCryNameR pSampleLumOffsetsName0("SampleLumOffsets0");
	static CCryNameR pSampleLumOffsetsName1("SampleLumOffsets1");

	m_shHDR->FXSetPSFloat(pSampleLumOffsetsName0, &vSampleLumOffsets0, 1);
	m_shHDR->FXSetPSFloat(pSampleLumOffsetsName1, &vSampleLumOffsets1, 1);

	SetShaderParams();

	bool ret = DrawFullScreenQuad(0.0f, 1.0f - 1.0f * gcpRendD3D->m_CurViewportScale.y, 1.0f * gcpRendD3D->m_CurViewportScale.x, 1.0f);

	// important that we always write out valid luminance, even if quad draw fails
	if (!ret)
	{
		rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[dwCurTexture], Clr_Dark);
	}

	m_shHDR->FXEndPass();

	rd->FX_PopRenderTarget(0);

	dwCurTexture--;

	// Initialize the sample offsets for the iterative luminance passes
	while (dwCurTexture >= 0)
	{
		rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);
		rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

		if (!dwCurTexture)
			gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
		if (dwCurTexture == 1)
			gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

		static CCryNameTSCRC TechNameLI("HDRSampleLumIterative");
		m_shHDR->FXSetTechnique(TechNameLI);
		m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
		m_shHDR->FXBeginPass(0);

		GetSampleOffsets_DownScale4x4Bilinear(CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->GetHeight(), avSampleOffsets);
		m_shHDR->FXSetPSFloat(Param1Name, avSampleOffsets, 4);
		CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->Apply(0, nTexStateLinear);

		// Draw a fullscreen quad to sample the RT
		ret = DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

		// important that we always write out valid luminance, even if quad draw fails
		if (!ret)
		{
			rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[dwCurTexture], Clr_Dark);
		}

		m_shHDR->FXEndPass();

		rd->FX_PopRenderTarget(0);

		dwCurTexture--;
	}

	gcpRendD3D->GetDeviceContext().CopyResource(
	  CTexture::s_ptexHDRMeasuredLuminance[gcpRendD3D->RT_GetCurrGpuID()]->GetDevTexture()->GetBaseTexture(),
	  CTexture::s_ptexHDRToneMaps[0]->GetDevTexture()->GetBaseTexture());

	gRenDev->m_RP.m_FlagsShader_RT = nFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CHDRPostProcess::EyeAdaptation()
{
	PROFILE_LABEL_SCOPE("EYEADAPTATION");

	CD3D9Renderer* rd = gcpRendD3D;

	// Swap current & last luminance
	const int lumMask = (int32)(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0])) - 1;
	const int32 numTextures = (int32)max(min(gRenDev->GetActiveGPUCount(), (uint32)(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))), 1u);

	CTexture::s_nCurLumTextureIndex++;
	CTexture* pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(CTexture::s_nCurLumTextureIndex - numTextures) & lumMask];
	CTexture* pTexCur = CTexture::s_ptexHDRAdaptedLuminanceCur[CTexture::s_nCurLumTextureIndex & lumMask];
	CTexture::s_ptexCurLumTexture = pTexCur;
	assert(pTexCur);

	uint32 nPasses;
	static CCryNameTSCRC TechName("HDRCalculateAdaptedLum");
	m_shHDR->FXSetTechnique(TechName);
	m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	rd->FX_PushRenderTarget(0, pTexCur, NULL);
	rd->RT_SetViewport(0, 0, pTexCur->GetWidth(), pTexCur->GetHeight());

	m_shHDR->FXBeginPass(0);

	SetShaderParams();

	Vec4 v;
	v[0] = iTimer->GetFrameTime() * numTextures;
	v[1] = 1.0f - expf(-CRenderer::CV_r_HDREyeAdaptationSpeed * v[0]);
	v[2] = 1.0f - expf(-CRenderer::CV_r_HDRRangeAdaptationSpeed * v[0]);
	v[3] = 0;

	if (rd->GetCamera().IsJustActivated() || rd->m_nDisableTemporalEffects > 0)
	{
		v[1] = 1.0f;
		v[2] = 1.0f;
	}

	static CCryNameR Param1Name("ElapsedTime");
	m_shHDR->FXSetPSFloat(Param1Name, &v, 1);
	pTexPrev->Apply(0, nTexStatePoint);
	CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStatePoint);

	// Draw a fullscreen quad to sample the RT
	DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

	m_shHDR->FXEndPass();

	rd->FX_PopRenderTarget(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_HDRRangeAdaptUpdate()
{
	CD3D9Renderer* rd = gcpRendD3D;

	if (CRenderer::CV_r_HDRRangeAdapt)
	{
		// Swap current & last luminance
		const int lumMask = (int32)(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0])) - 1;
		int nCurLum = CTexture::s_nCurLumTextureIndex;
		int nCurLumAdaptIdx = (nCurLum - 2) & lumMask;

		//D3DSurface *pSrcDevSurf = pTex->GetSurface(0, 0);
		//D3DSurface *pDstDevSurfSys = CTexture::s_ptexCurLumTextureSys->GetSurface(0, 0);

		//		D3DXFLOAT16 *hSurfSys = 0;
		float pLumInfo[2];
		pLumInfo[0] = pLumInfo[1] = 0;

		// note: RG16F - channels are swapped
		rd->m_vSceneLuminanceInfo.x = max(0.0f, min(16.0f, pLumInfo[1])); // avg
		rd->m_vSceneLuminanceInfo.y = max(0.0f, min(16.0f, pLumInfo[0]));//0;//pLumInfo[2];	// min - instead of min, maybe we can simplify and output maybe 4x smaller luminance
		//rd->m_vSceneLuminanceInfo.z = ;	// max

		if (_isnan(rd->m_vSceneLuminanceInfo.x))
			rd->m_vSceneLuminanceInfo.x = 0.0f;
		if (_isnan(rd->m_vSceneLuminanceInfo.y))
			rd->m_vSceneLuminanceInfo.y = 0.0f;

		// For night/very dark scenes boost maximum range
		const float fStocopicThreshold = 0.01f;
		rd->m_fScotopicSceneScale += (((rd->m_vSceneLuminanceInfo.x < fStocopicThreshold) ? 2.0f : 1.0f) - rd->m_fScotopicSceneScale) * gEnv->pTimer->GetFrameTime();

		// Tweakable scene range scale.
		// Optimaly we could rescale to take advantage of full texture format range into account, but we need to maintain acceptable range for blooming (else we get very poor bloom quality)
		rd->m_fAdaptedSceneScale = CRenderer::CV_r_HDRRangeAdaptMax / (1e-6f + rd->m_vSceneLuminanceInfo.y);
		rd->m_fAdaptedSceneScale = max(1.0f, min(rd->m_fAdaptedSceneScale, CRenderer::CV_r_HDRRangeAdaptMaxRange * rd->m_fScotopicSceneScale));

		// todo: we need to try out different strategies for light buffers (different scales for day/night scenes? we hit top clamping too often)

		// Light buffers range scale
		rd->m_fAdaptedSceneScaleLBuffer = CRenderer::CV_r_HDRRangeAdaptLBufferMax / (1e-6f + rd->m_vSceneLuminanceInfo.y);
		rd->m_fAdaptedSceneScaleLBuffer = max(1.0f, min(rd->m_fAdaptedSceneScaleLBuffer, CRenderer::CV_r_HDRRangeAdaptLBufferMaxRange * rd->m_fScotopicSceneScale));

		//rd->m_fAdaptedSceneScaleLBuffer = max(1.0f, min(rd->m_vSceneLuminanceInfo.y, CRenderer::CV_r_HDRRangeAdaptLBufferMaxRange * rd->m_fScotopicSceneScale) );

	}
	else
	{
		rd->m_vSceneLuminanceInfo = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		;
		rd->m_fAdaptedSceneScale = 1.0f;
		rd->m_fAdaptedSceneScaleLBuffer = 1.0f;
		rd->m_fScotopicSceneScale = 1.0f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::BloomGeneration()
{
	// Approximate function (1 - r)^4 by a sum of Gaussians: 0.0174*G(0.008,r) + 0.192*G(0.0576,r)
	const float sigma1 = sqrtf(0.008f);
	const float sigma2 = sqrtf(0.0576f - 0.008f);

	PROFILE_LABEL_SCOPE("BLOOM_GEN");

	CD3D9Renderer* rd = gcpRendD3D;
	static CCryNameTSCRC TechName("HDRBloomGaussian");
	static CCryNameR szHDRParam0("HDRParams0");

	uint64 nPrevFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]);

	int width = CTexture::s_ptexHDRFinalBloom->GetWidth();
	int height = CTexture::s_ptexHDRFinalBloom->GetHeight();

	// Note: Just scaling the sampling offsets depending on the resolution is not very accurate but works acceptably
	assert(CTexture::s_ptexHDRFinalBloom->GetWidth() == CTexture::s_ptexHDRTarget->GetWidth() / 4);
	float scaleW = ((float)width / 400.0f) / (float)width;
	float scaleH = ((float)height / 225.0f) / (float)height;
	int32 texFilter = (CTexture::s_ptexHDRFinalBloom->GetWidth() == 400 && CTexture::s_ptexHDRFinalBloom->GetHeight() == 225) ? nTexStatePoint : nTexStateLinear;

	rd->FX_SetState(GS_NODEPTHTEST);
	rd->RT_SetViewport(0, 0, width, height);

	CTexture::s_ptexHDRToneMaps[0]->Apply(2, nTexStatePoint);    // Current luminance

	// Pass 1 Horizontal
	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[1], NULL);
	SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	Vec4 v = Vec4(scaleW, 0, 0, 0);
	m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
	CTexture::s_ptexHDRTargetScaled[1]->Apply(0, texFilter);
	SPostEffectsUtils::DrawFullScreenTri(width, height);
	SD3DPostEffectsUtils::ShEndPass();
	rd->FX_PopRenderTarget(0);

	// Pass 1 Vertical
	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[0], NULL);
	SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	v = Vec4(0, scaleH, 0, 0);
	m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
	CTexture::s_ptexHDRTempBloom[1]->Apply(0, texFilter);
	SPostEffectsUtils::DrawFullScreenTri(width, height);
	SD3DPostEffectsUtils::ShEndPass();
	rd->FX_PopRenderTarget(0);

	// Pass 2 Horizontal
	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[1], NULL);
	SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	v = Vec4((sigma2 / sigma1) * scaleW, 0, 0, 0);
	m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
	CTexture::s_ptexHDRTempBloom[0]->Apply(0, texFilter);
	SPostEffectsUtils::DrawFullScreenTri(width, height);
	SD3DPostEffectsUtils::ShEndPass();
	rd->FX_PopRenderTarget(0);

	// Pass 2 Vertical
	rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRFinalBloom, NULL);
	SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	v = Vec4(0, (sigma2 / sigma1) * scaleH, 0, 0);
	m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
	CTexture::s_ptexHDRTempBloom[1]->Apply(0, texFilter);
	CTexture::s_ptexHDRTempBloom[0]->Apply(1, texFilter);
	SPostEffectsUtils::DrawFullScreenTri(width, height);
	SD3DPostEffectsUtils::ShEndPass();
	rd->FX_PopRenderTarget(0);

	rd->m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::ProcessLensOptics()
{
	gcpRendD3D->m_RP.m_PersFlags2 &= ~RBPF2_LENS_OPTICS_COMPOSITE;
	if (CRenderer::CV_r_flares)
	{
		const uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_LENSOPTICS);
		if (nBatchMask & (FB_GENERAL | FB_TRANSPARENT))
		{
			PROFILE_LABEL_SCOPE("LENS_OPTICS");

			CTexture* pLensOpticsComposite = CTexture::s_ptexSceneTargetR11G11B10F[0];
			gcpRendD3D->FX_ClearTarget(pLensOpticsComposite, Clr_Transparent);
			gcpRendD3D->FX_PushRenderTarget(0, pLensOpticsComposite, 0);

			gcpRendD3D->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA | RBPF2_LENS_OPTICS_COMPOSITE;

			GetUtils().Log(" +++ Begin lens-optics scene +++ \n");
			gcpRendD3D->FX_ProcessRenderList(EFSLIST_LENSOPTICS, FB_GENERAL);
			gcpRendD3D->FX_ProcessRenderList(EFSLIST_LENSOPTICS, FB_TRANSPARENT);
			gcpRendD3D->FX_ResetPipe();
			GetUtils().Log(" +++ End lens-optics scene +++ \n");

			gcpRendD3D->FX_SetActiveRenderTargets();
			gcpRendD3D->FX_PopRenderTarget(0);
			gcpRendD3D->FX_SetActiveRenderTargets();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::ToneMapping()
{
	CD3D9Renderer* rd = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

	if (!gcpRendD3D->m_pColorGradingControllerD3D)
		return;

	CSunShafts* pSunShaftsTech = (CSunShafts*)PostEffectMgr()->GetEffect(ePFX_SunShafts);

	CTexture* pSunShaftsRT = CTexture::s_ptexBlack;
	if (CRenderer::CV_r_sunshafts && CRenderer::CV_r_PostProcess && pSunShaftsTech && pSunShaftsTech->IsVisible())
	{
		/////////////////////////////////////////////
		// Create shafts mask texture

		uint32 nWidth = CTexture::s_ptexBackBufferScaled[1]->GetWidth();
		uint32 nHeight = CTexture::s_ptexBackBufferScaled[1]->GetHeight();
		pSunShaftsRT = CTexture::s_ptexBackBufferScaled[1];
		CTexture* pSunShaftsPingPongRT = CTexture::s_ptexBackBufferScaledTemp[1];
		if (rRP.m_eQuality >= eRQ_High)
		{
			pSunShaftsRT = CTexture::s_ptexBackBufferScaled[0];
			pSunShaftsPingPongRT = CTexture::s_ptexBackBufferScaledTemp[0];
			nWidth = CTexture::s_ptexBackBufferScaled[0]->GetWidth();
			nHeight = CTexture::s_ptexBackBufferScaled[0]->GetHeight();
		}

		pSunShaftsTech->SunShaftsGen(pSunShaftsRT, pSunShaftsPingPongRT);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Update color grading

	bool bColorGrading = false;

	SColorGradingMergeParams pMergeParams;
	if (CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts)
	{
		CColorGrading* pColorGrad = 0;
		if (!PostEffectMgr()->GetEffects().empty())
			pColorGrad = static_cast<CColorGrading*>(PostEffectMgr()->GetEffect(ePFX_ColorGrading));

		if (pColorGrad && pColorGrad->UpdateParams(pMergeParams))
			bColorGrading = true;
	}

	CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
	CTexture* pTexColorChar = pCtrl ? pCtrl->GetColorChart() : 0;

	rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());

	PROFILE_LABEL_SCOPE("TONEMAPPING");

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Enable corresponding shader variation
	rRP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE5] | g_HWSR_MaskBit[HWSR_SAMPLE4]);

	if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

	if (bColorGrading && pTexColorChar)
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

	if (CRenderer::CV_r_HDRDebug == 5)
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];

	const uint32 nAAMode = rd->FX_GetAntialiasingType();
	if (nAAMode & eAT_FXAA_MASK)
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

#if defined(SUPPORTS_MSAA)
	rd->FX_SetMSAAFlagsRT();
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final bloom RT

	CTexture* pBloom = CTexture::s_ptexBlack;
	if (CRenderer::CV_r_HDRBloom && CRenderer::CV_r_PostProcess)
		pBloom = CTexture::s_ptexHDRFinalBloom;

	PREFAST_ASSUME(pBloom);
	assert(pBloom);

	uint32 nPasses;
	static CCryNameTSCRC TechFinalName("HDRFinalPass");
	m_shHDR->FXSetTechnique(TechFinalName);
	m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	m_shHDR->FXBeginPass(0);

	gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

	// Set all shader params
	SetShaderParams();

	// For Post SSAA
	Matrix44A mPrevViewProj = CMotionBlur::GetPrevView() * (GetUtils().m_pProj * GetUtils().m_pScaleBias);
	mPrevViewProj.Transpose();
	static CCryNameR pParamPrevViewProj("mViewProjPrev");
	m_shHDR->FXSetPSFloat(pParamPrevViewProj, (Vec4*)mPrevViewProj.GetData(), 4);

	Vec4 vHDRParams3 = Vec4(0.0f, 1, 0, 0);
	static CCryNameR pszHDRParam3("HDRParams3");
	m_shHDR->FXSetPSFloat(pszHDRParam3, &vHDRParams3, 1);

	static uint32 dwNoiseOffsetX = 0;
	static uint32 dwNoiseOffsetY = 0;
	dwNoiseOffsetX = (dwNoiseOffsetX + 27) & 0x3f;
	dwNoiseOffsetY = (dwNoiseOffsetY + 19) & 0x3f;

	Vec4 pFrameRand(
	  dwNoiseOffsetX / 64.0f,
	  dwNoiseOffsetX / 64.0f,
	  cry_random(0, 1023) / 1024.0f,
	  cry_random(0, 1023) / 1024.0f);

	static CCryNameR Param4Name("FrameRand");
	m_shHDR->FXSetVSFloat(Param4Name, &pFrameRand, 1);

	static CCryNameR pSunShaftsParamSName("SunShafts_SunCol");
	Vec4 pShaftsSunCol(0, 0, 0, 0);
	if (pSunShaftsRT)
	{
		Vec4 pSunShaftsParams[2];
		pSunShaftsTech->GetSunShaftsParams(pSunShaftsParams);
		Vec3 pSunColor = gEnv->p3DEngine->GetSunColor();
		pSunColor.Normalize();
		pSunColor.SetLerp(Vec3(pSunShaftsParams[0].x, pSunShaftsParams[0].y, pSunShaftsParams[0].z), pSunColor, pSunShaftsParams[1].w);

		pShaftsSunCol = Vec4(pSunColor * pSunShaftsParams[1].z, 1);
	}

	m_shHDR->FXSetPSFloat(pSunShaftsParamSName, &pShaftsSunCol, 1);

	// Force commit before setting samplers - workaround for per frame samplers hardcoded/overriding sampler slots
	rd->FX_Commit();

	SResourceView::KeyType nResourceID = rRP.m_MSAAData.Type ? SResourceView::DefaultViewMS : SResourceView::DefaultView;

	CTexture::s_ptexHDRTarget->Apply(0, nTexStateLinear, EFTT_UNKNOWN, -1, nResourceID);

	if (CTexture::s_ptexCurLumTexture && !CRenderer::CV_r_HDRRangeAdapt)
	{
		if (!gRenDev->m_CurViewportID)
			CTexture::s_ptexCurLumTexture->Apply(1, nTexStateLinear /*nTexStatePoint*/);
		else
			CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStateLinear);
	}

	pBloom->Apply(2, nTexStateLinear);

	CTexture::s_ptexZTarget->Apply(5, nTexStatePoint);

	if (CRenderer::CV_r_PostProcess && CRenderer::CV_r_HDRVignetting)
		CTexture::s_ptexVignettingMap->Apply(7, nTexStateLinear);
	else
		CTexture::s_ptexWhite->Apply(7, nTexStateLinear);

	if (pTexColorChar)
		pTexColorChar->Apply(8, nTexStateLinear);

	if (pSunShaftsRT)
		pSunShaftsRT->Apply(9, nTexStateLinear);

	SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::ToneMappingDebug()
{
	CD3D9Renderer* rd = gcpRendD3D;
	Vec4 avSampleOffsets[4];

	if (CRenderer::CV_r_HDRDebug == 1)
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
	else
		gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEBUG0];

	uint32 nPasses;
	static CCryNameTSCRC techName("HDRFinalDebugPass");
	m_shHDR->FXSetTechnique(techName);
	m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	m_shHDR->FXBeginPass(0);

	GetSampleOffsets_DownScale2x2(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight(), avSampleOffsets);
	static CCryNameR SampleOffsetsName("SampleOffsets");
	m_shHDR->FXSetPSFloat(SampleOffsetsName, avSampleOffsets, 4);

	SetShaderParams();

	CTexture::s_ptexHDRTarget->Apply(0, nTexStatePoint);
	CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStateLinear);
	DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

	gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEBUG0];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::DrawDebugViews()
{
	if (CRenderer::CV_r_HDRDebug != 1 && CRenderer::CV_r_HDRDebug != 3 && CRenderer::CV_r_HDRDebug != 4)
		return;

	if (!CRenderer::CV_r_HDRRendering)
		return;

	CD3D9Renderer* rd = gcpRendD3D;
	uint32 nPasses = 0;

	if (CRenderer::CV_r_HDRDebug == 1)
	{
		// We use screen shots to create minimaps, and we don't want to
		// have any debug text on minimaps.
		{
			ICVar* const pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
			if (pVar && pVar->GetIVal() != 0)
			{
				return;
			}
		}

		STALL_PROFILER("read scene luminance")

		float fLuminance = -1;
		float fIlluminance = -1;

		CDeviceTexture* pSrcDevTex = CTexture::s_ptexHDRToneMaps[0]->GetDevTexture();
		pSrcDevTex->DownloadToStagingResource(0, [&](void* pData, uint32 rowPitch, uint32 slicePitch)
		{
			CryHalf* pRawPtr = (CryHalf*)pData;

			fLuminance = CryConvertHalfToFloat(pRawPtr[0]);
			fIlluminance = CryConvertHalfToFloat(pRawPtr[1]);

			return true;
		});

		char str[256];
		Vec3 color(1, 0, 1);
		cry_sprintf(str, "Average Luminance (cd/m2): %.2f", fLuminance * RENDERER_LIGHT_UNIT_SCALE);
		IRenderAuxText::Draw2dText(5, 35, color, str);
		cry_sprintf(str, "Estimated Illuminance (lux): %.1f", fIlluminance * RENDERER_LIGHT_UNIT_SCALE);
		IRenderAuxText::Draw2dText(5, 55, color, str);

		Vec4 vHDRSetupParams[5];
		gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

		if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
		{
			// Compute scene key and exposure in the same way as in the tone mapping shader
			float sceneKey = 1.03f - 2.0f / (2.0f + log(fLuminance + 1.0f) / log(2.0f));
			float exposure = clamp_tpl<float>(sceneKey / fLuminance, vHDRSetupParams[4].y, vHDRSetupParams[4].z);

			cry_sprintf(str, "Exposure: %.2f  SceneKey: %.2f", exposure, sceneKey);
			IRenderAuxText::Draw2dText(5, 75, color, str);
		}
		else
		{
			float exposure = log(fIlluminance * RENDERER_LIGHT_UNIT_SCALE * 100.0f / 330.0f) / log(2.0f);
			float sceneKey = log(fIlluminance * RENDERER_LIGHT_UNIT_SCALE + 1.0f) / log(10.0f);
			float autoCompensation = (clamp_tpl<float>(sceneKey, 0.1f, 5.2f) - 3.0f) / 2.0f * vHDRSetupParams[3].z;
			float finalExposure = clamp_tpl<float>(exposure - autoCompensation, vHDRSetupParams[3].x, vHDRSetupParams[3].y);

			cry_sprintf(str, "Measured EV: %.1f  Auto-EC: %.1f  Final EV: %.1f", exposure, autoCompensation, finalExposure);
			IRenderAuxText::Draw2dText(5, 75, color, str);
		}

		return;
	}

	rd->FX_SetState(GS_NODEPTHTEST);
	int iTmpX, iTmpY, iTempWidth, iTempHeight;
	rd->GetViewport(&iTmpX, &iTmpY, &iTempWidth, &iTempHeight);

	rd->EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
	rd->Set2DMode(true, 1, 1);

	gRenDev->m_cEF.mfRefreshSystemShader("Debug", CShaderMan::s_ShaderDebug);
	CShader* pSH = CShaderMan::s_ShaderDebug;

	CTexture* pSceneTargetHalfRes = CTexture::s_ptexHDRTargetScaled[0];
	CTexture::s_ptexHDRTarget->SetResolved(false);
	CTexture::s_ptexHDRTarget->Resolve();
	// 1
	uint32 nPosX = 10;
	rd->RT_SetViewport(nPosX, 500, 100, 100);
	rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTarget->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

	// 2
	rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
	rd->DrawImage(0, 0, 1, 1, pSceneTargetHalfRes->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

	// 3
	if (CRenderer::CV_r_HDRBloom)
	{
		// Bloom generation/intermediate render-targets

		// Quarter res
		rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
		rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTempBloom[0]->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

		rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
		rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTempBloom[1]->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

		rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
		rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRFinalBloom->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);
	}

	nPosX = 10;

	pSH->FXSetTechnique("Debug_ShowR");
	pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	pSH->FXBeginPass(0);

	CTexture::s_ptexHDRToneMaps[3]->Apply(0, nTexStatePoint);
	rd->RT_SetViewport(nPosX, 610, 100, 100);
	DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);

	if (CTexture::s_ptexHDRToneMaps[2])
	{
		CTexture::s_ptexHDRToneMaps[2]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexHDRToneMaps[1])
	{
		CTexture::s_ptexHDRToneMaps[1]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexHDRToneMaps[0])
	{
		CTexture::s_ptexHDRToneMaps[0]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexCurLumTexture)
	{
		CTexture::s_ptexCurLumTexture->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	pSH->FXEndPass();
	pSH->FXEnd();

	rd->Set2DMode(false, 1, 1);
	rd->RT_SetViewport(iTmpX, iTmpY, iTempWidth, iTempHeight);

	{
		char str[256];

		cry_sprintf(str, "HDR rendering debug");
		IRenderAuxText::Draw2dText(5, 310, Vec3(1), str);

		if (CRenderer::CV_r_HDRRangeAdapt)
		{
			cry_sprintf(str, "Range adaption enabled");
			IRenderAuxText::Draw2dText(10, 325, Vec3(1), str);
		}
	}

	if (CRenderer::CV_r_HDRDebug == 4)
	{
		char str[256];
		cry_sprintf(str, "Avg Luminance: %.5f \nMin Luminance: %.5f \nAdapted scene scale: %.5f\nAdapted light buffer scale: %.5f",
		            rd->m_vSceneLuminanceInfo.x, rd->m_vSceneLuminanceInfo.y, rd->m_fAdaptedSceneScale, rd->m_fAdaptedSceneScaleLBuffer);
		IRenderAuxText::Draw2dText(5, 5, Vec3(1), str);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::ScreenShot()
{
	if (CRenderer::CV_r_GetScreenShot == 1)
	{
		iLog->LogError("HDR screen shots are not yet supported on DX11!");    // TODO: D3DXSaveSurfaceToFile()
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::Begin()
{
	gcpRendD3D->GetModelViewMatrix(PostProcessUtils().m_pView.GetData());
	gcpRendD3D->GetProjectionMatrix(PostProcessUtils().m_pProj.GetData());

	// Store some commonly used per-frame data

	PostProcessUtils().m_pViewProj = PostProcessUtils().m_pView * PostProcessUtils().m_pProj;
	if (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
		PostProcessUtils().m_pViewProj = ReverseDepthHelper::Convert(PostProcessUtils().m_pViewProj);

	PostProcessUtils().m_pViewProj.Transpose();

	m_shHDR = CShaderMan::s_shHDRPostProcess;

	if (CTexture::s_ptexHDRTarget->GetWidth() != gcpRendD3D->GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != gcpRendD3D->GetHeight())
		CTexture::GenerateHDRMaps();

	gcpRendD3D->FX_ResetPipe();
	PostProcessUtils().SetFillModeSolid(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::End()
{
	gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_HDR;
	gcpRendD3D->m_RP.m_PersFlags2 &= ~(RBPF2_HDR_FP16 | RBPF2_LIGHTSHAFTS);

	gcpRendD3D->FX_ResetPipe();

	PostProcessUtils().SetFillModeSolid(false);

	// (re-set back-buffer): due to lazy RT updates/setting there's strong possibility we run into problems on x360 when we try to resolve from edram with no RT set
	gcpRendD3D->FX_SetActiveRenderTargets();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHDRPostProcess::Render()
{
	PROFILE_LABEL_SCOPE("HDR_POSTPROCESS");

	Begin();

	assert(gcpRendD3D->m_RTStack[0][gcpRendD3D->m_nRTStackLevel[0]].m_pTex == CTexture::s_ptexHDRTarget);

	gcpRendD3D->FX_SetActiveRenderTargets(); // Called explicitly to work around RT stack problems on 360
	gcpRendD3D->RT_UnbindTMUs();// Avoid d3d error due to potential rtv still bound as shader input.
	gcpRendD3D->FX_PopRenderTarget(0);
	gcpRendD3D->EF_ClearTargetsLater(0);

	// Skip hdr/post processing when rendering different camera views
	if ((gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL) || (gcpRendD3D->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN))
	{
		End();
		return;
	}

	// Use specular RT as temporary backbuffer when native resolution rendering is enabled; the RT gets popped before upscaling
	if (gcpRendD3D->IsNativeScalingEnabled())
		gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexSceneSpecular, NULL);

	const uint32 nAAMode = gcpRendD3D->FX_GetAntialiasingType();
	CTexture* pDstRT = CTexture::s_ptexSceneDiffuse;
	if ((nAAMode & (eAT_SMAA_MASK | eAT_FXAA_MASK)) && gRenDev->CV_r_PostProcess && pDstRT)
	{
		assert(pDstRT);
		// Render to intermediate target, to avoid redundant imagecopy/stretchrect for post aa
		gcpRendD3D->FX_PushRenderTarget(0, pDstRT, &gcpRendD3D->m_DepthBufferOrigMSAA);
		gcpRendD3D->RT_SetViewport(0, 0, pDstRT->GetWidth(), pDstRT->GetHeight());
	}

#if !defined(_RELEASE) || CRY_PLATFORM_WINDOWS || defined(ENABLE_LW_PROFILERS)
	ScreenShot();
#endif

	gcpRendD3D->m_RP.m_FlagsShader_RT = 0;

	gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0]; // enable srgb. Can save this flag, always enabled
	if (CRenderer::CV_r_HDRRangeAdapt)
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

	gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);
	m_bHiQuality = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Low, eSQ_VeryHigh);

#ifdef SUPPORTS_MSAA
	gcpRendD3D->FX_ResetMSAAFlagsRT();
	CTexture::s_ptexHDRTarget->SetResolved(false);
	CTexture::s_ptexHDRTarget->Resolve();
	gcpRendD3D->FX_SetMSAAFlagsRT();
#endif

	// Rain
	CSceneRain* pSceneRain = (CSceneRain*)PostEffectMgr()->GetEffect(ePFX_SceneRain);
	const SRainParams& rainInfo = gcpRendD3D->m_p3DEngineCommon.m_RainInfo;
	if (pSceneRain && pSceneRain->IsActive() && rainInfo.fRainDropsAmount > 0.01f)
	{
		pSceneRain->Render();
	}

	// Note: MB uses s_ptexHDRTargetPrev to avoid doing another copy, so this should be right before the MB pass
	PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetPrev, false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, &gcpRendD3D->m_FullResRect);

	if (CRenderer::CV_r_PostProcess)
	{
		if (CRenderer::CV_r_dof >= 1 && CRenderer::CV_r_DofMode == 1)
		{
			CDepthOfField* pDof = (CDepthOfField*) PostEffectMgr()->GetEffect(ePFX_eDepthOfField);
			pDof->Render();
		}

		if ((gcpRendD3D->GetWireframeMode() == R_SOLID_MODE) && CRenderer::CV_r_MotionBlur)
		{
			CMotionBlur* pMotionBlur = (CMotionBlur*) PostEffectMgr()->GetEffect(ePFX_eMotionBlur);
			pMotionBlur->Render();
		}

		{
			CSceneSnow* pSceneSnow = (CSceneSnow*) PostEffectMgr()->GetEffect(ePFX_SceneSnow);
			if (pSceneSnow->IsActiveSnow())
				pSceneSnow->Render();
		}

		HalfResDownsampleHDRTarget();

		gcpRendD3D->SetCurDownscaleFactor(Vec2(1, 1));

		QuarterResDownsampleHDRTarget();
	}

	gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);

	// Update eye adaptation
	MeasureLuminance();
	EyeAdaptation();

	if (CRenderer::CV_r_HDRBloom && CRenderer::CV_r_PostProcess)
		BloomGeneration();

	gcpRendD3D->SetCurDownscaleFactor(gcpRendD3D->m_CurViewportScale);

	// Render final scene to the back buffer
	if (CRenderer::CV_r_HDRDebug != 1 && CRenderer::CV_r_HDRDebug != 2)
		ToneMapping();
	else
		ToneMappingDebug();

	if (CRenderer::CV_r_HDRDebug > 0)
		DrawDebugViews();

#ifdef SUPPORTS_MSAA
	if (gRenDev->m_RP.m_MSAAData.Type && CRenderer::CV_r_msaa_debug >= 2)
	{
		gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
		gRenDev->m_RP.m_FlagsShader_RT |= (CRenderer::CV_r_msaa_debug > 2) ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;

		static CCryNameTSCRC pMsaaDebugTech("MsaaDebug");
		PostProcessUtils().ShBeginPass(CShaderMan::s_shPostAA, pMsaaDebugTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
		gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
		PostProcessUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
		PostProcessUtils().DrawQuadFS(CShaderMan::s_shPostAA, false, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
		PostProcessUtils().ShEndPass();
	}
#endif
	End();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_HDRPostProcessing()
{
	PROFILE_FRAME(Draw_HDR_PostProcessing);

	if (gcpRendD3D->m_bDeviceLost)
		return;

	if (!CTexture::IsTextureExist(CTexture::s_ptexHDRTarget))
		return;

	CHDRPostProcess* pHDRPostProcess = CHDRPostProcess::GetInstance();
	pHDRPostProcess->Render();
	pHDRPostProcess->ProcessLensOptics();
}
