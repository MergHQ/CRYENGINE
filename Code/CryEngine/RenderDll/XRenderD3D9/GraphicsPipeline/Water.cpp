// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Water.h"

#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "Common/ReverseDepth.h"
#include "GraphicsPipeline/WaterRipples.h"
#include "GraphicsPipeline/VolumetricFog.h"

//////////////////////////////////////////////////////////////////////////

namespace water
{
// water stage unique parameters
struct SPerPassWater
{
	Matrix44 causticViewProjMatr;
	Vec4     waterRippleLookup;
};

// fog parameters
struct SPerPassFog
{
	Vec4  vfParams;      // PB_VolumetricFogParams
	Vec4  vfRampParams;  // PB_VolumetricFogRampParams
	Vec4  vfSunDir;      // PB_VolumetricFogSunDir
	Vec3  vfColGradBase; // PB_FogColGradColBase
	float padding0;
	Vec3  vfColGradDelta; // PB_FogColGradColDelta
	float padding1;
	Vec4  vfColGradParams; // PB_FogColGradParams
	Vec4  vfColGradRadial; // PB_FogColGradRadial
};

// Volumetric fog shadows
struct SPerPassVolFogShadow
{
	Vec4 volFogShadowDarkening;
	Vec4 volFogShadowDarkeningSunAmb;
};

// voxel-based volumetric fog parameters
struct SPerPassVoxelVolFog
{
	Vec4 vfSamplingParams;           // PB_VolumetricFogSamplingParams
	Vec4 vfDistributionParams;       // PB_VolumetricFogDistributionParams
	Vec4 vfScatteringParams;         // PB_VolumetricFogScatteringParams
	Vec4 vfScatteringBlendParams;    // PB_VolumetricFogScatteringBlendParams
	Vec4 vfScatteringColor;          // PB_VolumetricFogScatteringColor
	Vec4 vfScatteringSecondaryColor; // PB_VolumetricFogScatteringSecondaryColor
	Vec4 vfHeightDensityParams;      // PB_VolumetricFogHeightDensityParams
	Vec4 vfHeightDensityRampParams;  // PB_VolumetricFogHeightDensityRampParams
	Vec4 vfDistanceParams;           // PB_VolumetricFogDistanceParams
	Vec4 vfGlobalEnvProbeParams0;    // PB_VolumetricFogGlobalEnvProbe0
	Vec4 vfGlobalEnvProbeParams1;    // PB_VolumetricFogGlobalEnvProbe1
};

struct SPerPassConstantBuffer
{
	SPerPassWater                             cbPerPassWater;
	SPerPassFog                               cbPerPassFog;
	SPerPassVolFogShadow                      cbVolFogShadow;
	SPerPassVoxelVolFog                       cbVoxVolFog;
	CShadowUtils::SShadowCascadesSamplingInfo cbShadowSampling;
};

struct SPrimitiveConstants
{
	Matrix44 transformMatrix;
};

void GetSnapVector(Vec3& vVector, const float fSnapRange)
{
	Vec3 vSnapped = vVector / fSnapRange;
	vSnapped.Set(floor_tpl(vSnapped.x), floor_tpl(vSnapped.y), floor_tpl(vSnapped.z));
	vSnapped *= fSnapRange;
	vVector = vSnapped;
}
}

//////////////////////////////////////////////////////////////////////////

bool CWaterStage::UpdateCausticsGrid(N3DEngineCommon::SCausticInfo& causticInfo, bool& bVertexUpdated, CRenderer* pRenderer)
{
	// 16 bit index limit, can only do max 256x256 grid.
	// could use hardware tessellation to reduce memory and increase tessellation amount for higher precision.
	const uint32 nCausticMeshWidth = clamp_tpl(CRenderer::CV_r_watervolumecausticsdensity, 16, 255);
	const uint32 nCausticMeshHeight = clamp_tpl(CRenderer::CV_r_watervolumecausticsdensity, 16, 255);

	bVertexUpdated = false;

	// Update the grid mesh if required.
	if ((!causticInfo.m_pCausticQuadMesh || causticInfo.m_nCausticMeshWidth != nCausticMeshWidth || causticInfo.m_nCausticMeshHeight != nCausticMeshHeight))
	{
		// Make sure we aren't recreating the mesh.
		causticInfo.m_pCausticQuadMesh = NULL;

		const uint32 nCausticVertexCount = (nCausticMeshWidth + 1) * (nCausticMeshHeight + 1);
		const uint32 nCausticIndexCount = (nCausticMeshWidth * nCausticMeshHeight) * 6;

		// Store the new resolution and vertex/index counts.
		causticInfo.m_nCausticMeshWidth = nCausticMeshWidth;
		causticInfo.m_nCausticMeshHeight = nCausticMeshHeight;
		causticInfo.m_nVertexCount = nCausticVertexCount;
		causticInfo.m_nIndexCount = nCausticIndexCount;

		// Reciprocal for scaling.
		float fRecipW = 1.0f / (float)nCausticMeshWidth;
		float fRecipH = 1.0f / (float)nCausticMeshHeight;

		// Buffers.
		SVF_P3F_C4B_T2F* pCausticQuads = new SVF_P3F_C4B_T2F[nCausticVertexCount];
		vtx_idx* pCausticIndices = new vtx_idx[nCausticIndexCount];

		// Fill vertex buffer.
		for (uint32 y = 0; y <= nCausticMeshHeight; ++y)
		{
			for (uint32 x = 0; x <= nCausticMeshWidth; ++x)
			{
				pCausticQuads[y * (nCausticMeshWidth + 1) + x].xyz = Vec3(((float)x) * fRecipW, ((float)y) * fRecipH, 0.0f);
			}
		}

		// Fill index buffer.
		for (uint32 y = 0; y < nCausticMeshHeight; ++y)
		{
			for (uint32 x = 0; x < nCausticMeshWidth; ++x)
			{
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6] = y * (nCausticMeshWidth + 1) + x;
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 1] = y * (nCausticMeshWidth + 1) + x + 1;
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 2] = (y + 1) * (nCausticMeshWidth + 1) + x + 1;
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 3] = (y + 1) * (nCausticMeshWidth + 1) + x + 1;
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 4] = (y + 1) * (nCausticMeshWidth + 1) + x;
				pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 5] = y * (nCausticMeshWidth + 1) + x;
			}
		}

		// Create the mesh.
		causticInfo.m_pCausticQuadMesh = pRenderer->CreateRenderMeshInitialized(pCausticQuads, nCausticVertexCount, eVF_P3F_C4B_T2F, pCausticIndices, nCausticIndexCount, prtTriangleList, "WaterCausticMesh", "WaterCausticMesh");

		// Delete the temporary buffers.
		delete[] pCausticQuads;
		delete[] pCausticIndices;

		bVertexUpdated = true;
	}

	// If we created the mesh, return true.
	if (causticInfo.m_pCausticQuadMesh)
		return true;

	return false;
}

CWaterStage::CWaterStage()
	: m_pFoamTex(nullptr)
	, m_pPerlinNoiseTex(nullptr)
	, m_pJitterTex(nullptr)
	, m_pWaterGlossTex(nullptr)
	, m_pOceanWavesTex(nullptr)
	, m_pOceanCausticsTex(nullptr)
	, m_pOceanMaskTex(nullptr)
	, m_rainRippleTexIndex(0)
	, m_frameIdWaterSim(0)
	, m_bWaterNormalGen(false)
{
	std::fill(std::begin(m_pRainRippleTex), std::end(m_pRainRippleTex), nullptr);
	std::fill(std::begin(m_oceanAnimationParams), std::end(m_oceanAnimationParams), Vec4(0.0f));
}

CWaterStage::~CWaterStage()
{
	SAFE_RELEASE(m_pFoamTex);
	SAFE_RELEASE(m_pPerlinNoiseTex);
	SAFE_RELEASE(m_pJitterTex);
	SAFE_RELEASE(m_pWaterGlossTex);
	SAFE_RELEASE(m_pOceanWavesTex);
	SAFE_RELEASE(m_pOceanCausticsTex);
	SAFE_RELEASE(m_pOceanMaskTex);

	for (auto& pTex : m_pRainRippleTex)
	{
		SAFE_RELEASE(pTex);
	}
}

void CWaterStage::Init()
{
	m_pDefaultPerInstanceResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	for (uint32 i = 0; i < ePass_Count; ++i)
	{
		m_pPerPassResources[i] = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
		m_pPerPassCB[i] = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(sizeof(water::SPerPassConstantBuffer));
	}

	CRY_ASSERT(m_pFoamTex == nullptr);
	m_pFoamTex = CTexture::ForName("%ENGINE%/EngineAssets/Shading/WaterFoam.tif", FT_DONT_STREAM, eTF_Unknown);

	const string fileNamePath = "%ENGINE%/EngineAssets/Textures/Rain/Ripple/ripple";
	const string fileNameExt = "_ddn.tif";
	uint32 index = 1;
	for (auto& pTex : m_pRainRippleTex)
	{
		CRY_ASSERT(pTex == nullptr);
		string fileName = fileNamePath + string().Format("%d", index) + fileNameExt;
		pTex = CTexture::ForName(fileName.c_str(), FT_DONT_STREAM, eTF_Unknown);
		++index;
	}

	CRY_ASSERT(m_pPerlinNoiseTex == nullptr);
	m_pPerlinNoiseTex = CTexture::ForName("%ENGINE%/EngineAssets/Textures/perlinNoise2d.tif", FT_DONT_STREAM, eTF_Unknown);

	CRY_ASSERT(m_pJitterTex == nullptr);
	m_pJitterTex = CTexture::ForName("%ENGINE%/EngineAssets/Textures/FogVolShadowJitter.tif", FT_DONT_STREAM, eTF_Unknown);

	CRY_ASSERT(m_pWaterGlossTex == nullptr);
	m_pWaterGlossTex = CTexture::ForName("%ENGINE%/EngineAssets/Textures/water_gloss.tif", FT_DONT_STREAM, eTF_Unknown);

	CRY_ASSERT(m_pOceanWavesTex == nullptr);
	m_pOceanWavesTex = CTexture::ForName("%ENGINE%/EngineAssets/Textures/oceanwaves_ddn.tif", FT_DONT_STREAM, eTF_Unknown);

	CRY_ASSERT(m_pOceanCausticsTex == nullptr);
	m_pOceanCausticsTex = CTexture::ForName("%ENGINE%/EngineAssets/Textures/caustics_sampler.dds", FT_DONT_STREAM, eTF_Unknown);

	CRY_ASSERT(m_pOceanMaskTex == nullptr);
	const uint32 flags = FT_NOMIPS | FT_DONT_STREAM | FT_USAGE_RENDERTARGET;
	const ETEX_Format format = eTF_R8;
	m_pOceanMaskTex = CTexture::CreateTextureObject("$OceanMask", 0, 0, 0, eTT_2D, flags, eTF_R8);

	CConstantBufferPtr pCB = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(sizeof(water::SPrimitiveConstants));
	m_deferredOceanStencilPrimitive[0].SetInlineConstantBuffer(eConstantBufferShaderSlot_PerBatch, pCB, EShaderStage_Vertex);
	m_deferredOceanStencilPrimitive[1].SetInlineConstantBuffer(eConstantBufferShaderSlot_PerBatch, pCB, EShaderStage_Vertex);

	bool bSuccess = PrepareDefaultPerInstanceResources();
	for (uint32 i = 0; i < ePass_Count; ++i)
	{
		bSuccess = bSuccess && PreparePerPassResources(nullptr, true, EPass(i));
	}
	bSuccess = bSuccess && PrepareResourceLayout();
	CRY_ASSERT(bSuccess);

	// TODO: move a texture to local member variable.
	//CRY_ASSERT(CTexture::IsTextureExist(CTexture::s_ptexWaterCaustics[0])); // this can fail because CTexture::s_ptexWaterCaustics[0] is null when launching the engine with r_watervolumecaustics=0.
	CRY_ASSERT(CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeRefl[0]));

	auto* pDepthTarget = &(gcpRendD3D->m_DepthBufferOrigMSAA);

	m_passOceanMaskGen.SetLabel("OCEAN_MASK_GEN");
	m_passOceanMaskGen.SetupPassContext(m_stageID, ePass_OceanMaskGen, TTYPE_GENERAL, FB_GENERAL, EFSLIST_WATER, 0, false);
	m_passOceanMaskGen.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_OceanMaskGen]);
	m_passOceanMaskGen.SetRenderTargets(pDepthTarget, m_pOceanMaskTex);

	auto* pDummyRenderTarget = CTexture::s_ptexSceneSpecular;
	CRY_ASSERT(pDummyRenderTarget && pDummyRenderTarget->GetTextureDstFormat() == eTF_R8G8B8A8);

	m_passWaterCausticsSrcGen.SetLabel("WATER_VOLUME_CAUSTICS_SRC_GEN");
	m_passWaterCausticsSrcGen.SetupPassContext(m_stageID, ePass_CausticsGen, TTYPE_WATERCAUSTICPASS, FB_WATER_CAUSTIC, EFSLIST_WATER, 0, false);
	m_passWaterCausticsSrcGen.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_CausticsGen]);
	m_passWaterCausticsSrcGen.SetRenderTargets(nullptr, pDummyRenderTarget);

	auto* pRenderTarget = CTexture::s_ptexHDRTarget;

	m_passWaterFogVolumeBeforeWater.SetLabel("WATER_FOG_VOLUME_BEFORE_WATER");
	m_passWaterFogVolumeBeforeWater.SetupPassContext(m_stageID, ePass_FogVolume, TTYPE_GENERAL, FB_BELOW_WATER, EFSLIST_WATER_VOLUMES, 0, false);
	m_passWaterFogVolumeBeforeWater.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_FogVolume]);
	m_passWaterFogVolumeBeforeWater.SetRenderTargets(pDepthTarget, pRenderTarget);

	m_passWaterReflectionGen.SetLabel("WATER_VOLUME_REFLECTION_GEN");
	m_passWaterReflectionGen.SetupPassContext(m_stageID, ePass_ReflectionGen, TTYPE_WATERREFLPASS, FB_WATER_REFL, EFSLIST_WATER, 0, false);
	m_passWaterReflectionGen.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_ReflectionGen]);
	m_passWaterReflectionGen.SetRenderTargets(nullptr, CTexture::s_ptexWaterVolumeRefl[0]);

	m_passWaterSurface.SetLabel("WATER_SURFACE");
	m_passWaterSurface.SetupPassContext(m_stageID, ePass_WaterSurface, TTYPE_GENERAL, FB_GENERAL, EFSLIST_WATER, 0, false);
	m_passWaterSurface.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_WaterSurface]);
	m_passWaterSurface.SetRenderTargets(pDepthTarget, pRenderTarget);

	m_passWaterFogVolumeAfterWater.SetLabel("WATER_FOG_VOLUME_AFTER_WATER");
	m_passWaterFogVolumeAfterWater.SetupPassContext(m_stageID, ePass_FogVolume, TTYPE_GENERAL, FB_GENERAL, EFSLIST_WATER_VOLUMES, FB_BELOW_WATER, false);
	m_passWaterFogVolumeAfterWater.SetPassResources(m_pResourceLayout, m_pPerPassResources[ePass_FogVolume]);
	m_passWaterFogVolumeAfterWater.SetRenderTargets(pDepthTarget, pRenderTarget);
}

void CWaterStage::Prepare(CRenderView* pRenderView)
{
	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;

	const int32 screenWidth = rd->GetWidth();
	const int32 screenHeight = rd->GetHeight();

	auto& waterRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER);
	auto& waterVolumeRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER_VOLUMES);
	const bool isEmpty = waterRenderItems.empty() && waterVolumeRenderItems.empty();

	const int32 nGridSize = 64;

	// Create Domain Shader Texture
	// NOTE: textures which are static uniform (with path in the code) are assigned before
	// the render-element is called, so if the render element allocated the required textures
	// then they will not make it into the first draw of the shader, and are 0 instead
	if (!isEmpty && !CTexture::IsTextureExist(CTexture::s_ptexWaterOcean))
	{
		CTexture::s_ptexWaterOcean->Create2DTexture(nGridSize, nGridSize, 1,
		                                            FT_DONT_RELEASE | FT_NOMIPS | FT_STAGE_UPLOAD,
		                                            0, eTF_R32G32B32A32F, eTF_R32G32B32A32F);
	}

	{
		bool bOceanMask = false;

		switch ((rd->GetShaderProfile(eST_Water)).GetShaderQuality())
		{
		case eSQ_High:
		case eSQ_VeryHigh:
			// ocean surface can be displaced in these settings.
			bOceanMask = true;
			break;
		}

		if (!bOceanMask && CTexture::IsTextureExist(m_pOceanMaskTex))
		{
			m_pOceanMaskTex->ReleaseDeviceTexture(false);
		}

		const uint32 flags = FT_NOMIPS | FT_DONT_STREAM | FT_USAGE_RENDERTARGET;
		const ETEX_Format format = eTF_R8;
		if (bOceanMask
		    && m_pOceanMaskTex
		    && (!CTexture::IsTextureExist(m_pOceanMaskTex) || m_pOceanMaskTex->Invalidate(screenWidth, screenHeight, format)))
		{
			m_pOceanMaskTex->Create2DTexture(screenWidth, screenHeight, 1, flags, nullptr, format, format);
		}
	}

	// Activate puddle generation
	m_bWaterNormalGen = (rd->m_RP.m_eQuality > eRQ_Low && !isEmpty) ? true : false;
	// TODO: remove after old graphics pipeline is removed.
	if (m_bWaterNormalGen)
	{
		CEffectParam* pParam = PostEffectMgr()->GetByName("WaterVolume_Amount");
		CRY_ASSERT(pParam);

		if (pParam)
			pParam->SetParam(1.0f);
	}
}

void CWaterStage::ExecuteWaterVolumeCaustics()
{
	auto* pRenderView = RenderView();
	CRY_ASSERT(pRenderView);

	// must be called before all render passes in water stage.
	Prepare(pRenderView);

	if (pRenderView->IsRecursive())
	{
		return;
	}

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;

	const auto& ti = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID];
	const bool bWaterRipple = (!(ti.m_PersFlags & RBPF_MAKESPRITE) && (rd->m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS));

	auto& graphicsPipeline = rd->GetGraphicsPipeline();
	auto* pWaterRipplesStage = graphicsPipeline.GetWaterRipplesStage();

	const auto& waterRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER);
	const auto& waterVolumeRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER_VOLUMES);
	const bool bEmpty = waterRenderItems.empty() && waterVolumeRenderItems.empty();

	// process water ripples
	if (pWaterRipplesStage)
	{
		pWaterRipplesStage->Prepare(pRenderView);

		if (!bEmpty && bWaterRipple)
		{
			pWaterRipplesStage->Execute(pRenderView);
		}
	}

	// generate normal texture for water volumes and ocean.
	if (m_bWaterNormalGen && rd->m_CurRenderEye == LEFT_EYE)
	{
		ExecuteWaterNormalGen();
	}

	ExecuteOceanMaskGen(pRenderView);

	// Check if there are any water volumes that have caustics enabled
	bool isEmpty = bEmpty;
	if (!bEmpty)
	{
		auto& RESTRICT_REFERENCE RI = pRenderView->GetRenderItems(EFSLIST_WATER);

		int32 curRI = 0;
		int32 endRI = RI.size();

		isEmpty = true;

		while (curRI < endRI)
		{
			CRenderElement* pRE = RI[curRI++].pElem;
			if (pRE->m_Type == eDATA_WaterVolume
			    && ((CREWaterVolume*)pRE)->m_pParams
			    && ((CREWaterVolume*)pRE)->m_pParams->m_caustics == true)
			{
				isEmpty = false;
				break;
			}
		}
	}

	const uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_WATER);

	if (!isEmpty
	    && (nBatchMask & FB_WATER_CAUSTIC)
	    && CTexture::IsTextureExist(CTexture::s_ptexWaterCaustics[0])
	    && CTexture::IsTextureExist(CTexture::s_ptexWaterCaustics[1])
	    && CRenderer::CV_r_watercaustics
	    && CRenderer::CV_r_watercausticsdeferred
	    && CRenderer::CV_r_watervolumecaustics)
	{
		PROFILE_LABEL_SCOPE("WATERVOLUME_CAUSTICS");

		// TODO: move this to local member if it isn't used in other stages.
		// Caustics info.
		N3DEngineCommon::SCausticInfo& causticInfo = rd->m_p3DEngineCommon.m_CausticInfo;

		// NOTE: caustics gen texture is generated only once if stereo rendering is activated.
		if (rd->m_CurRenderEye == LEFT_EYE)
		{
			ExecuteWaterVolumeCausticsGen(causticInfo, pRenderView);
		}

		if (causticInfo.m_pCausticQuadMesh)
		{
			// NOTE: this function is called instead in CDeferredShading::Render() when tiled deferred shading is disabled.
			const bool bTiledDeferredShading = CRenderer::CV_r_DeferredShadingTiled >= 2;
			if (bTiledDeferredShading)
			{
				ExecuteDeferredWaterVolumeCaustics(bTiledDeferredShading);
			}
		}
	}
}

void CWaterStage::ExecuteDeferredWaterVolumeCaustics(bool bTiledDeferredShading)
{
	if (!CTexture::s_ptexBackBuffer || !CTexture::s_ptexSceneTarget)
	{
		return;
	}

	PROFILE_LABEL_SCOPE("DEFERRED_WATERVOLUME_CAUSTICS");

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	N3DEngineCommon::SCausticInfo& causticInfo = rd->m_p3DEngineCommon.m_CausticInfo;

	auto* pTargetTex =
	  bTiledDeferredShading
	  ? CTexture::s_ptexSceneTargetR11G11B10F[1]
	  : CTexture::s_ptexSceneDiffuseAccMap;

	auto& pass = m_passDeferredWaterVolumeCaustics;

	if (pass.InputChanged(pTargetTex->GetTextureID()))
	{
		static CCryNameTSCRC techName = "WaterVolumeCaustics";
		pass.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);

		pass.SetRenderTarget(0, pTargetTex);

		int32 nRState = GS_NODEPTHTEST;
		if (!bTiledDeferredShading)
		{
			// Blend directly into light accumulation buffer
			nRState |= GS_BLSRC_ONE | GS_BLDST_ONE;
		}
		pass.SetState(nRState);

		pass.SetTexture(0, CTexture::s_ptexZTarget);
		pass.SetTexture(1, CTexture::s_ptexSceneNormalsMap);
		pass.SetTexture(2, CTexture::s_ptexWaterCaustics[0]);

		pass.SetSampler(0, rd->m_nTrilinearClampSampler);
		pass.SetSampler(1, rd->m_nPointClampSampler);

		pass.SetRequirePerViewConstantBuffer(true);
	}

	pass.BeginConstantUpdate();

	static CCryNameR paramLightView("mLightView");
	pass.SetConstantArray(paramLightView, (Vec4*)causticInfo.m_mCausticMatr.GetData(), 4);

	pass.Execute();

	if (bTiledDeferredShading)
	{
		rd->GetTiledShading().NotifyCausticsVisible();
	}
}

void CWaterStage::ExecuteDeferredOceanCaustics()
{
	if (!CRenderer::CV_r_watercaustics
	    || !CRenderer::CV_r_watercausticsdeferred
	    || !CTexture::s_ptexBackBuffer
	    || !CTexture::s_ptexSceneTarget)
	{
		return;
	}

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	I3DEngine* RESTRICT_POINTER pEng = gEnv->p3DEngine;

	const Vec4 causticParams = gEnv->p3DEngine->GetOceanAnimationCausticsParams();
	const float fCausticsHeight = causticParams.y;
	const float fCausticsDepth = causticParams.z;
	const float fCausticsIntensity = causticParams.w;

	N3DEngineCommon::SOceanInfo& OceanInfo = rd->m_p3DEngineCommon.m_OceanInfo;
	const bool bOceanVolumeVisible = (OceanInfo.m_nOceanRenderFlags & OCR_OCEANVOLUME_VISIBLE) != 0;
	if (!bOceanVolumeVisible || iszero(fCausticsIntensity))
	{
		return;
	}

	PROFILE_LABEL_SCOPE("OCEAN_CAUSTICS");
	PROFILE_FRAME(DrawShader_DeferredCausticsPass);

	// Caustics are done with projection from sun - hence they update too fast with regular
	// sun direction. Use a smooth sun direction update instead to workaround this
	SCGParamsPF& PF = rd->m_cEF.m_PF[rd->m_RP.m_nProcessThreadID];
	if (PF.nCausticsFrameID != rd->GetFrameID(false))
	{
		PF.nCausticsFrameID = rd->GetFrameID(false);

		Vec3 pRealtimeSunDirNormalized = pEng->GetRealtimeSunDirNormalized();

		const float fSnapDot = 0.98f;
		float fDot = fabs(PF.vCausticsCurrSunDir.Dot(pRealtimeSunDirNormalized));
		if (fDot < fSnapDot)
			PF.vCausticsCurrSunDir = pRealtimeSunDirNormalized;

		const float factor = 0.005f;
		PF.vCausticsCurrSunDir += (pRealtimeSunDirNormalized - PF.vCausticsCurrSunDir) * factor * gEnv->pTimer->GetFrameTime();
		PF.vCausticsCurrSunDir.Normalize();
	}

	const float fWatLevel = OceanInfo.m_fWaterLevel;
	const float fCausticsLevel = fWatLevel + fCausticsHeight;
	const Vec4 pCausticsParams = Vec4(CRenderer::CV_r_watercausticsdistance,
	                                  OceanInfo.m_vCausticsParams.z * fCausticsIntensity,
	                                  OceanInfo.m_vCausticsParams.w,
	                                  fCausticsLevel);

	CStandardGraphicsPipeline::SViewInfo viewInfo[2];
	const int32 viewInfoCount = rd->GetGraphicsPipeline().GetViewInfo(viewInfo);
	CRY_ASSERT(viewInfoCount > 0);

	static const uint8 stencilReadWriteMask = 0xFF & ~BIT_STENCIL_RESERVED;
	uint8 stencilRef = 0xFF;

	auto* pTargetTex = CTexture::s_ptexHDRTarget;
	auto* pDepthTarget = &(gcpRendD3D->m_DepthBufferOrigMSAA);

	// Stencil pre-pass
	if (CRenderer::CV_r_watercausticsdeferred == 2)
	{
		Matrix34 mLocal[2];
		const float fDist = sqrtf((pCausticsParams.x * 5.0f) * 13.333f); // Hard cut off when caustic would be attenuated to 0.2 (1/5.0f)
		for (uint32 i = 0; i < viewInfoCount; ++i)
		{
			Vec3 vCamPos = viewInfo[i].pRenderCamera->vOrigin;

			float heightAboveWater = max(0.0f, vCamPos.z - fCausticsLevel);
			const float dist = sqrtf(max((fDist * fDist) - (heightAboveWater * heightAboveWater), 0.0f));

			mLocal[i].SetIdentity();
			mLocal[i].SetScale(Vec3(dist * 2.0f, dist * 2.0f, fCausticsHeight + fCausticsDepth));
			mLocal[i].SetTranslation(Vec3(vCamPos.x - dist, vCamPos.y - dist, fWatLevel - fCausticsDepth));
		}

		static CCryNameTSCRC techName("DeferredOceanCausticsStencil");

		const bool bReverseDepth = (viewInfo[0].flags & CStandardGraphicsPipeline::SViewInfo::eFlags_ReverseDepth) != 0;
		const int32 gsDepthFunc = bReverseDepth ? GS_DEPTHFUNC_GEQUAL : GS_DEPTHFUNC_LEQUAL;

		// update shared stencil ref value. the code is copied from CD3D9Renderer::FX_StencilCullPass().
		rd->m_nStencilMaskRef += 1;
		if (rd->m_nStencilMaskRef > STENC_MAX_REF)
		{
			rd->FX_ClearTarget(pDepthTarget, FRT_CLEAR_STENCIL, Clr_Unused.r, 1);
			rd->m_nStencilMaskRef = 2;
		}
		stencilRef = rd->m_nStencilMaskRef;
		CRY_ASSERT(rd->m_nStencilMaskRef > 0 && rd->m_nStencilMaskRef <= STENC_MAX_REF);

		auto& pass = m_passDeferredOceanCausticsStencil;
		pass.ClearPrimitives();
		pass.SetDepthTarget(pDepthTarget);

		D3DViewPort viewport;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(pTargetTex->GetWidth());
		viewport.Height = static_cast<float>(pTargetTex->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		pass.SetViewport(viewport);

		auto& backFacePrim = m_deferredOceanStencilPrimitive[0];
		if (backFacePrim.IsDirty())
		{
			backFacePrim.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);

			backFacePrim.SetPrimitiveType(CRenderPrimitive::ePrim_UnitBox);

			backFacePrim.SetCullMode(eCULL_Front);
			backFacePrim.SetRenderState(GS_STENCIL | GS_COLMASK_NONE | gsDepthFunc);

			backFacePrim.SetInlineConstantBuffer(eConstantBufferShaderSlot_PerView, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), EShaderStage_Vertex);
		}

		static const int32 stencilStateBackFace = STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
		                                          STENCOP_FAIL(FSS_STENCOP_KEEP) |
		                                          STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
		                                          STENCOP_PASS(FSS_STENCOP_KEEP);
		backFacePrim.SetStencilState(stencilStateBackFace, stencilRef, stencilReadWriteMask, stencilReadWriteMask);

		// Update constant buffer
		{
			auto& constantManager = backFacePrim.GetConstantManager();

			auto constants = constantManager.BeginTypedConstantUpdate<water::SPrimitiveConstants>(eConstantBufferShaderSlot_PerBatch, EShaderStage_Vertex);
			constants->transformMatrix = mLocal[0];

			if (viewInfoCount > 1)
			{
				constants.BeginStereoOverride();
				constants->transformMatrix = mLocal[1];
			}

			constantManager.EndTypedConstantUpdate(constants);
		}

		pass.AddPrimitive(&backFacePrim);

		auto& frontFacePrim = m_deferredOceanStencilPrimitive[1];
		if (frontFacePrim.IsDirty())
		{
			frontFacePrim.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);

			frontFacePrim.SetPrimitiveType(CRenderPrimitive::ePrim_UnitBox);

			frontFacePrim.SetCullMode(eCULL_Back);
			frontFacePrim.SetRenderState(GS_STENCIL | GS_COLMASK_NONE | gsDepthFunc);

			frontFacePrim.SetInlineConstantBuffer(eConstantBufferShaderSlot_PerView, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), EShaderStage_Vertex);
		}

		static const int32 stencilStateFrontFace = STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
		                                           STENCOP_FAIL(FSS_STENCOP_KEEP) |
		                                           STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
		                                           STENCOP_PASS(FSS_STENCOP_KEEP);
		frontFacePrim.SetStencilState(stencilStateFrontFace, stencilRef, stencilReadWriteMask, stencilReadWriteMask);

		pass.AddPrimitive(&frontFacePrim);

		pass.Execute();
	}

	// Deferred ocean caustic pass
	{
		auto* pOceanMask = CTexture::IsTextureExist(m_pOceanMaskTex) ? m_pOceanMaskTex : CTexture::s_ptexBlack;

		auto& pass = m_passDeferredOceanCaustics;

		if (pass.InputChanged(CRenderer::CV_r_watercausticsdeferred, pOceanMask->GetID()))
		{
			static CCryNameTSCRC techName = "General";
			pass.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);

			pass.SetRenderTarget(0, pTargetTex);
			pass.SetDepthTarget(pDepthTarget);

			int32 nRState = GS_NODEPTHTEST
			                | ((CRenderer::CV_r_watercausticsdeferred == 2) ? GS_STENCIL : 0)
			                | (GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA);
			pass.SetState(nRState);

			pass.SetTexture(0, CTexture::s_ptexZTarget);
			pass.SetTexture(1, CTexture::s_ptexSceneNormalsMap);
			pass.SetTexture(2, m_pOceanWavesTex);
			pass.SetTexture(3, m_pOceanCausticsTex);
			pass.SetTexture(4, pOceanMask);

			pass.SetSampler(0, rd->m_nTrilinearClampSampler);
			pass.SetSampler(1, rd->m_nTrilinearWrapSampler);
			pass.SetSampler(2, rd->m_nPointClampSampler);

			pass.SetRequirePerViewConstantBuffer(true);
		}

		static const int32 stencilStateTest = STENC_FUNC(FSS_STENCFUNC_EQUAL) |
		                                      STENCOP_FAIL(FSS_STENCOP_KEEP) |
		                                      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
		                                      STENCOP_PASS(FSS_STENCOP_KEEP);
		pass.SetStencilState(stencilStateTest, stencilRef, stencilReadWriteMask, stencilReadWriteMask);

		pass.BeginConstantUpdate();

		static CCryNameR nameCausticParams("vCausticParams");
		pass.SetConstant(nameCausticParams, pCausticsParams);

		float fTime = 0.125f * rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_RealTime;
		Vec4 vAnimParams(0.06f * fTime, 0.05f * fTime, 0.1f * fTime, -0.11f * fTime);

		static CCryNameR nameAnimParams("vAnimParams");
		pass.SetConstant(nameAnimParams, vAnimParams);

		Vec3 up = Vec3(0, 0, 1);
		Vec3 dirZ = -PF.vCausticsCurrSunDir.GetNormalized();
		Vec3 dirX = up.Cross(dirZ).GetNormalized();
		Vec3 dirY = dirZ.Cross(dirX).GetNormalized();

		Matrix44 lightView;
		lightView.SetIdentity();
		lightView.SetRow(0, dirX);
		lightView.SetRow(1, dirY);
		lightView.SetRow(2, dirZ);

		static CCryNameR nameLightView("mLightView");
		pass.SetConstantArray(nameLightView, (Vec4*)lightView.GetData(), 4);

		pass.Execute();
	}
}

void CWaterStage::ExecuteWaterFogVolumeBeforeTransparent()
{
	auto* pRenderView = RenderView();
	CRY_ASSERT(pRenderView);

	if (pRenderView->IsRecursive())
	{
		return;
	}

	// prepare per pass device resource
	// this update must be called once after water ripple stage's execute() because waterRippleLookup parameter depends on it.
	// this update needs to be called after the execute() of fog and volumetric fog because their data is updated in their execute().
	PreparePerPassResources(pRenderView, false, ePass_FogVolume);

	// render water fog volumes before water.
	ExecuteSceneRenderPass(pRenderView, m_passWaterFogVolumeBeforeWater, EFSLIST_WATER_VOLUMES);
}

void CWaterStage::Execute()
{
	auto* pRenderView = RenderView();
	CRY_ASSERT(pRenderView);

	if (pRenderView->IsRecursive())
	{
		return;
	}

	PROFILE_LABEL_SCOPE("WATER");

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;

	// TODO: these should be kept in new graphics pipeline when main viewport is rendered.
	CRY_ASSERT(!(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE));
	CRY_ASSERT(rd->m_RP.m_nRendFlags & (SHDF_ALLOW_WATER | SHDF_ALLOWPOSTPROCESS));

	auto& graphicsPipeline = rd->GetGraphicsPipeline();
	auto* pWaterRipplesStage = graphicsPipeline.GetWaterRipplesStage();

	const auto& waterRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER);
	const auto& waterVolumeRenderItems = pRenderView->GetRenderItems(EFSLIST_WATER_VOLUMES);
	const bool bEmpty = waterRenderItems.empty() && waterVolumeRenderItems.empty();

	// Pre-process refraction
	if (!bEmpty && CTexture::IsTextureExist(CTexture::s_ptexCurrSceneTarget))
	{
		if (CRenderer::CV_r_debugrefraction == 0)
		{
			m_passCopySceneTarget.Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexCurrSceneTarget);
		}
		else
		{
			rd->FX_ClearTarget(CTexture::s_ptexCurrSceneTarget, Clr_Debug);
		}
	}

	// generate water volume reflection.
	PreparePerPassResources(pRenderView, false, ePass_ReflectionGen);
	ExecuteReflection(pRenderView);

	// render water volume and ocean surfaces.
	PreparePerPassResources(pRenderView, false, ePass_WaterSurface);
	ExecuteSceneRenderPass(pRenderView, m_passWaterSurface, EFSLIST_WATER);

	// render water fog volumes after water.
	ExecuteSceneRenderPass(pRenderView, m_passWaterFogVolumeAfterWater, EFSLIST_WATER_VOLUMES);
}

bool CWaterStage::CreatePipelineStates(uint32 passMask, DevicePipelineStatesArray& stateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache)
{
	if (pStateCache->Find(stateDesc, stateArray))
		return true;

	bool bFullyCompiled = (passMask != 0) ? true : false;

	SGraphicsPipelineStateDescription _stateDesc = stateDesc;

	if (passMask & ePassMask_WaterSurface)
	{
		_stateDesc.technique = TTYPE_GENERAL;
		bFullyCompiled &= CreatePipelineState(stateArray[ePass_WaterSurface], _stateDesc, ePass_WaterSurface, nullptr);
	}

	if (passMask & ePassMask_ReflectionGen)
	{
		_stateDesc.technique = TTYPE_WATERREFLPASS;
		bFullyCompiled &= CreatePipelineState(stateArray[ePass_ReflectionGen], _stateDesc, ePass_ReflectionGen, nullptr);
	}

	if (passMask & ePassMask_FogVolume)
	{
		_stateDesc.technique = TTYPE_GENERAL;
		bFullyCompiled &= CreatePipelineState(stateArray[ePass_FogVolume], _stateDesc, ePass_FogVolume, nullptr);
	}

	if (passMask & ePassMask_CausticsGen)
	{
		_stateDesc.technique = TTYPE_WATERCAUSTICPASS;
		bFullyCompiled &= CreatePipelineState(stateArray[ePass_CausticsGen], _stateDesc, ePass_CausticsGen, nullptr);
	}

	if (bFullyCompiled)
	{
		pStateCache->Put(stateDesc, stateArray);
	}

	return bFullyCompiled;
}

bool CWaterStage::CreatePipelineState(
  CDeviceGraphicsPSOPtr& outPSO,
  const SGraphicsPipelineStateDescription& desc,
  EPass passID,
  std::function<void(CDeviceGraphicsPSODesc& psoDesc)> modifier)
{
	CD3D9Renderer* pRenderer = gcpRendD3D;

	outPSO = nullptr;

	if (!(ePass_ReflectionGen <= passID && passID <= ePass_Count))
		return true; // psssID doesn't exit in water stage.

	auto shaderType = desc.shaderItem.m_pShader->GetShaderType();
	if (shaderType != eST_Water)
		return true; // non water type shader can't be rendered in water stage.

	CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), desc);
	if (!pRenderer->GetGraphicsPipeline().FillCommonScenePassStates(desc, psoDesc))
		return true; // technique doesn't exist so null PSO is returned.

	if (modifier)
	{
		modifier(psoDesc);
	}

	bool bWaterRipples = false;

	switch (passID)
	{
	case CWaterStage::ePass_ReflectionGen:
		{
			m_passWaterReflectionGen.ExtractRenderTargetFormats(psoDesc);

			bWaterRipples = true;
		}
		break;
	case CWaterStage::ePass_FogVolume:
		{
			m_passWaterFogVolumeBeforeWater.ExtractRenderTargetFormats(psoDesc);
		}
		break;
	case CWaterStage::ePass_WaterSurface:
		{
			m_passWaterSurface.ExtractRenderTargetFormats(psoDesc);

			bWaterRipples = true;
		}
		break;
	case CWaterStage::ePass_CausticsGen:
		{
			m_passWaterCausticsSrcGen.ExtractRenderTargetFormats(psoDesc);

			bWaterRipples = true;
		}
		break;
	case CWaterStage::ePass_OceanMaskGen:
		{
			m_passOceanMaskGen.ExtractRenderTargetFormats(psoDesc);

			// use optimized shader to generate mask.
			psoDesc.m_ShaderFlags_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];

			bWaterRipples = true;
		}
		break;
	default:
		CRY_ASSERT(false);
		return true;
	}

	// always adds the shader runtime mask for water ripples to avoid shader permutations.
	// TODO: move shader runtime masks to shader constants to avoid shader permutations after removing old graphics pipeline.
	psoDesc.m_ShaderFlags_RT |= bWaterRipples ? g_HWSR_MaskBit[HWSR_SAMPLE4] : 0;

	const bool bReverseDepth = (pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
	if (bReverseDepth)
	{
		psoDesc.m_RenderState = ReverseDepthHelper::ConvertDepthFunc(psoDesc.m_RenderState);
	}

	outPSO = CCryDeviceWrapper::GetObjectFactory().CreateGraphicsPSO(psoDesc);

	return (outPSO != nullptr);
}

bool CWaterStage::PrepareResourceLayout()
{
	SDeviceResourceLayoutDesc layoutDesc;

	layoutDesc.SetConstantBuffer(EResourceLayoutSlot_PerInstanceCB, eConstantBufferShaderSlot_PerInstance, EShaderStage_AllWithoutCompute);
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerMaterialRS, gcpRendD3D->GetGraphicsPipeline().GetDefaultMaterialResources());
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerInstanceExtraRS, m_pDefaultPerInstanceResources);
	layoutDesc.SetResourceSet(EResourceLayoutSlot_PerPassRS, m_pPerPassResources[ePass_ReflectionGen]);

	m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(layoutDesc);
	return m_pResourceLayout != nullptr;
}

bool CWaterStage::PrepareDefaultPerInstanceResources()
{
	CD3D9Renderer* RESTRICT_POINTER pRenderer = gcpRendD3D;

	auto& res = m_pDefaultPerInstanceResources;

	res->Clear();

	// default textures for water volume
	{
		res->SetTexture(ePerInstanceTexture_PerlinNoise, m_pPerlinNoiseTex, SResourceView::DefaultView, EShaderStage_Pixel | EShaderStage_Vertex | EShaderStage_Domain);
		res->SetTexture(ePerInstanceTexture_Jitter, m_pJitterTex, SResourceView::DefaultView, EShaderStage_Pixel);
	}

	res->Build();
	return res->IsValid();
}

bool CWaterStage::PreparePerPassResources(CRenderView* RESTRICT_POINTER pRenderView, bool bOnInit, EPass passId)
{
	auto& pResSet = m_pPerPassResources[passId];
	CRY_ASSERT(pResSet);

	CD3D9Renderer* RESTRICT_POINTER pRenderer = gcpRendD3D;
	const int32 frameID = pRenderer->GetFrameID(false);

	pResSet->Clear();

	// Samplers
	{
		// default material samplers
		auto materialSamplers = pRenderer->GetGraphicsPipeline().GetDefaultMaterialSamplers();
		for (int32 i = 0; i < materialSamplers.size(); ++i)
		{
			pResSet->SetSampler(EEfResSamplers(i), materialSamplers[i], EShaderStage_AllWithoutCompute);
		}
		// Hardcoded point samplers
		pResSet->SetSampler(ePerPassSampler_PointWrap, pRenderer->m_nPointWrapSampler, EShaderStage_AllWithoutCompute);
		pResSet->SetSampler(ePerPassSampler_PointClamp, pRenderer->m_nPointClampSampler, EShaderStage_AllWithoutCompute);

		// per pass samplers
		static int32 aniso16xClampSampler = CTexture::GetTexState(STexState(FILTER_ANISO16X, TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP, 0x0));
		pResSet->SetSampler(ePerPassSampler_Aniso16xClamp, aniso16xClampSampler, EShaderStage_AllWithoutCompute);
		pResSet->SetSampler(ePerPassSampler_LinearClampComp, pRenderer->m_nLinearClampComparisonSampler, EShaderStage_AllWithoutCompute);

		// NOTE: overwrite default material sampler to avoid the limitation of DXOrbis.
		static int32 aniso16xWrapSampler = CTexture::GetTexState(STexState(FILTER_ANISO16X, TADDR_WRAP, TADDR_WRAP, TADDR_WRAP, 0x0));
		pResSet->SetSampler(ePerPassSampler_Aniso16xWrap, aniso16xWrapSampler, EShaderStage_AllWithoutCompute);
	}

	CTexture* pVolFogShadowTex = CTexture::s_ptexBlack;
#if defined(VOLUMETRIC_FOG_SHADOWS)
	if (pRenderer->m_bVolFogShadowsEnabled)
	{
		pVolFogShadowTex = CTexture::s_ptexVolFogShadowBuf[0];
	}
#endif

	CTexture* pVolumetricFogTex = CTexture::s_ptexBlack;
	if (pRenderer->m_bVolumetricFogEnabled && CTexture::IsTextureExist(CTexture::s_ptexVolumetricFog))
	{
		pVolumetricFogTex = CTexture::s_ptexVolumetricFog;
	}

	auto* pWaterNormalTex = m_bWaterNormalGen ? CTexture::s_ptexWaterVolumeDDN : CTexture::s_ptexFlatBump;

	// NOTE: update this resource set at every frame, otherwise have double resource sets.
	const int32 currWaterVolID = GetCurrentFrameID(frameID);
	const int32 prevWaterVolID = GetPreviousFrameID(frameID);
	CTexture* pCurrWaterVolRefl = CTexture::s_ptexWaterVolumeRefl[currWaterVolID];
	CTexture* pPrevWaterVolRefl = CTexture::s_ptexWaterVolumeRefl[prevWaterVolID];

	// NOTE: only water surface needs water reflection texture.
	const bool bWaterSurface = (passId == ePass_WaterSurface);
	auto* pWaterReflectionTex = bWaterSurface ? pCurrWaterVolRefl : CTexture::s_ptexBlack;

	// Textures
	{
		if (passId == ePass_FogVolume)
		{
			auto* pOceanMask = CTexture::IsTextureExist(m_pOceanMaskTex) ? m_pOceanMaskTex : CTexture::s_ptexBlack;
			pResSet->SetTexture(ePerPassTexture_WaterGloss, pOceanMask, SResourceView::DefaultView, EShaderStage_Pixel);
		}
		else
		{
			pResSet->SetTexture(ePerPassTexture_WaterGloss, m_pWaterGlossTex, SResourceView::DefaultView, EShaderStage_Pixel);
		}

		pResSet->SetTexture(ePerPassTexture_Foam, m_pFoamTex, SResourceView::DefaultView, EShaderStage_Pixel);

		if (!pRenderer->m_bPauseTimer)
		{
			// flip rain ripple texture
			const float elapsedTime = pRenderer->m_RP.m_TI[pRenderer->m_RP.m_nProcessThreadID].m_RealTime;
			CRY_ASSERT(elapsedTime >= 0.0f);
			const float AnimTexFlipTime = 0.05f;
			m_rainRippleTexIndex = (uint32)(elapsedTime / AnimTexFlipTime) % m_pRainRippleTex.size();
		}
		CRY_ASSERT(m_rainRippleTexIndex < m_pRainRippleTex.size());
		pResSet->SetTexture(ePerPassTexture_RainRipple, m_pRainRippleTex[m_rainRippleTexIndex], SResourceView::DefaultView, EShaderStage_Pixel);

		// forward shadow textures.
		CShadowUtils::SShadowCascades cascades;
		if (bOnInit)
		{
			std::fill(std::begin(cascades.pShadowMap), std::end(cascades.pShadowMap), CTexture::s_ptexFarPlane);
		}
		else
		{
			CShadowUtils::GetShadowCascades(cascades, RenderView());
		}
		pResSet->SetTexture(ePerPassTexture_ShadowMap0, cascades.pShadowMap[0], SResourceView::DefaultView, EShaderStage_Pixel);
		pResSet->SetTexture(ePerPassTexture_ShadowMap1, cascades.pShadowMap[1], SResourceView::DefaultView, EShaderStage_Pixel);
		pResSet->SetTexture(ePerPassTexture_ShadowMap2, cascades.pShadowMap[2], SResourceView::DefaultView, EShaderStage_Pixel);
		pResSet->SetTexture(ePerPassTexture_ShadowMap3, cascades.pShadowMap[3], SResourceView::DefaultView, EShaderStage_Pixel);

		// volumetric fog shadow
		pResSet->SetTexture(ePerPassTexture_VolFogShadow, pVolFogShadowTex, SResourceView::DefaultView, EShaderStage_Pixel);

		// voxel-based volumetric fog
		auto* pVolFogStage = pRenderer->GetGraphicsPipeline().GetVolumetricFogStage();
		pResSet->SetTexture(ePerPassTexture_VolumetricFog, pVolumetricFogTex, SResourceView::DefaultView, EShaderStage_Pixel);
		pResSet->SetTexture(ePerPassTexture_VolFogGlobalEnvProbe0, pVolFogStage->GetGlobalEnvProbeTex0(), SResourceView::DefaultView, EShaderStage_Pixel);
		pResSet->SetTexture(ePerPassTexture_VolFogGlobalEnvProbe1, pVolFogStage->GetGlobalEnvProbeTex1(), SResourceView::DefaultView, EShaderStage_Pixel);

		auto* pRippleStage = pRenderer->GetGraphicsPipeline().GetWaterRipplesStage();
		pResSet->SetTexture(ePerPassTexture_WaterRipple, pRippleStage->GetWaterRippleTex(), SResourceView::DefaultView, EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Domain);
		pResSet->SetTexture(ePerPassTexture_WaterNormal, pWaterNormalTex, SResourceView::DefaultView, EShaderStage_Pixel | EShaderStage_Domain);

		if (passId == ePass_ReflectionGen)
		{
			pResSet->SetTexture(ePerPassTexture_SceneDepth, CTexture::s_ptexZTargetScaled, SResourceView::DefaultView, EShaderStage_Pixel);
			pResSet->SetTexture(ePerPassTexture_Refraction, CTexture::s_ptexHDRTargetPrev, SResourceView::DefaultView, EShaderStage_Pixel);
			pResSet->SetTexture(ePerPassTexture_Reflection, pPrevWaterVolRefl, SResourceView::DefaultView, EShaderStage_Pixel);
		}
		else
		{
			pResSet->SetTexture(ePerPassTexture_SceneDepth, CTexture::s_ptexZTarget, SResourceView::DefaultView, EShaderStage_Pixel);
			pResSet->SetTexture(ePerPassTexture_Refraction, CTexture::s_ptexSceneTarget, SResourceView::DefaultView, EShaderStage_Pixel);
			pResSet->SetTexture(ePerPassTexture_Reflection, pCurrWaterVolRefl, SResourceView::DefaultView, EShaderStage_Pixel);
		}
	}

	// Constant buffers
	{
		// update constant buffer if needed.
		if (!bOnInit && pRenderView)
		{
			UpdatePerPassResources(*pRenderView, passId);
		}

		pResSet->SetConstantBuffer(eConstantBufferShaderSlot_PerPass, m_pPerPassCB[passId], EShaderStage_AllWithoutCompute);

		CConstantBufferPtr pPerViewCB;
		if (bOnInit)  // Handle case when no view is available in the initialization of the stage
		{
			pPerViewCB = CDeviceBufferManager::CreateNullConstantBuffer();
		}
		else
		{
			pPerViewCB = pRenderer->GetGraphicsPipeline().GetPerViewConstantBuffer();
		}

		pResSet->SetConstantBuffer(eConstantBufferShaderSlot_PerView, pPerViewCB, EShaderStage_AllWithoutCompute);

		if (bOnInit)
			return true;
	}

	pResSet->Build();
	return pResSet->IsValid();
}

void CWaterStage::UpdatePerPassResources(CRenderView& renderView, EPass passId)
{
	CD3D9Renderer* RESTRICT_POINTER pRenderer = gcpRendD3D;
	SRenderPipeline& rp(pRenderer->m_RP);
	SCGParamsPF& PF = pRenderer->m_cEF.m_PF[rp.m_nProcessThreadID];

	pRenderer->GetGraphicsPipeline().UpdatePerViewConstantBuffer();

	// update per pass constant buffer.
	{
		auto& graphicsPipeline = pRenderer->GetGraphicsPipeline();
		auto* pWaterRipplesStage = graphicsPipeline.GetWaterRipplesStage();
		auto* pVolFogStage = graphicsPipeline.GetVolumetricFogStage();
#if defined(VOLUMETRIC_FOG_SHADOWS)
		const bool bRenderFogShadow = pRenderer->m_bVolFogShadowsEnabled;
#else
		const bool bRenderFogShadow = false;
#endif
		N3DEngineCommon::SCausticInfo& causticInfo = pRenderer->m_p3DEngineCommon.m_CausticInfo;

		CRY_ASSERT(pWaterRipplesStage);
		CRY_ASSERT(pVolFogStage);

		CryStackAllocWithSize(water::SPerPassConstantBuffer, cb, CDeviceBufferManager::AlignBufferSizeForStreaming);

		// water related parameters
		auto& cbWater = cb->cbPerPassWater;
		cbWater.causticViewProjMatr = causticInfo.m_mCausticMatr;
		cbWater.waterRippleLookup = pWaterRipplesStage->GetWaterRippleLookupParam();

		// fog
		auto& cbFog = cb->cbPerPassFog;
		cbFog.vfParams = PF.pVolumetricFogParams;          // PB_VolumetricFogParams
		cbFog.vfRampParams = PF.pVolumetricFogRampParams;  // PB_VolumetricFogRampParams
		cbFog.vfSunDir = PF.pVolumetricFogSunDir;          // PB_VolumetricFogSunDir
		cbFog.vfColGradBase = Vec3(PF.pFogColGradColBase); // PB_FogColGradColBase
		cbFog.padding0 = 0.0f;
		cbFog.vfColGradDelta = Vec3(PF.pFogColGradColDelta); // PB_FogColGradColDelta
		cbFog.padding1 = 0.0f;
		cbFog.vfColGradParams = PF.pFogColGradParams; // PB_FogColGradParams
		cbFog.vfColGradRadial = PF.pFogColGradRadial; // PB_FogColGradRadial

		// volumetric fog shadow
		auto& cbVolFogShadow = cb->cbVolFogShadow;
#if defined(VOLUMETRIC_FOG_SHADOWS)
		if (bRenderFogShadow)
		{
			Vec3 volFogShadowDarkeningP;
			gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);
			cbVolFogShadow.volFogShadowDarkening = Vec4(volFogShadowDarkeningP, 0.0f);

			const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
			const float bSun = 1.0f - aSun;
			const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
			const float bAmb = 1.0f - aAmb;
			cbVolFogShadow.volFogShadowDarkeningSunAmb = Vec4(aSun, bSun, aAmb, bAmb);
		}
		else
		{
			cbVolFogShadow.volFogShadowDarkening = Vec4(0.0f);
			cbVolFogShadow.volFogShadowDarkeningSunAmb = Vec4(0.0f);
		}
#else
		cbVolFogShadow.volFogShadowDarkening = Vec4(0.0f);
		cbVolFogShadow.volFogShadowDarkeningSunAmb = Vec4(0.0f);
#endif

		// voxel-based volumetric fog
		auto& cbVolFog = cb->cbVoxVolFog;
		cbVolFog.vfSamplingParams = PF.pVolumetricFogSamplingParams;                      // PB_VolumetricFogSamplingParams
		cbVolFog.vfDistributionParams = PF.pVolumetricFogDistributionParams;              // PB_VolumetricFogDistributionParams
		cbVolFog.vfScatteringParams = PF.pVolumetricFogScatteringParams;                  // PB_VolumetricFogScatteringParams
		cbVolFog.vfScatteringBlendParams = PF.pVolumetricFogScatteringBlendParams;        // PB_VolumetricFogScatteringBlendParams
		cbVolFog.vfScatteringColor = PF.pVolumetricFogScatteringColor;                    // PB_VolumetricFogScatteringColor
		cbVolFog.vfScatteringSecondaryColor = PF.pVolumetricFogScatteringSecondaryColor;  // PB_VolumetricFogScatteringSecondaryColor
		cbVolFog.vfHeightDensityParams = PF.pVolumetricFogHeightDensityParams;            // PB_VolumetricFogHeightDensityParams
		cbVolFog.vfHeightDensityRampParams = PF.pVolumetricFogHeightDensityRampParams;    // PB_VolumetricFogHeightDensityRampParams
		cbVolFog.vfDistanceParams = PF.pVolumetricFogDistanceParams;                      // PB_VolumetricFogDistanceParams
		cbVolFog.vfGlobalEnvProbeParams0 = pVolFogStage->GetGlobalEnvProbeShaderParam0(); // PB_VolumetricFogGlobalEnvProbe0
		cbVolFog.vfGlobalEnvProbeParams1 = pVolFogStage->GetGlobalEnvProbeShaderParam1(); // PB_VolumetricFogGlobalEnvProbe1

		// forward shadow sampling parameters.
		CShadowUtils::SShadowCascadesSamplingInfo shadowSamplingInfo;
		CShadowUtils::GetShadowCascadesSamplingInfo(shadowSamplingInfo, &renderView);
		cb->cbShadowSampling = shadowSamplingInfo;

		CRY_ASSERT(m_pPerPassCB[passId]);
		m_pPerPassCB[passId]->UpdateBuffer(cb, cbSize);
	}
}

void CWaterStage::ExecuteWaterNormalGen()
{
	PROFILE_LABEL_SCOPE("WATER_NORMAL_GEN");

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	CRY_ASSERT(rd->m_CurRenderEye == LEFT_EYE);

	const int32 nGridSize = 64;

	const int32 frameID = SPostEffectsUtils::m_iFrameCounter % 2;

	// Create texture if required
	CTexture* pTexture = CTexture::s_ptexWaterVolumeTemp[frameID];
	if (!CTexture::IsTextureExist(pTexture))
	{
		if (!pTexture->Create2DTexture(nGridSize, nGridSize, 1,
		                               FT_DONT_RELEASE | FT_NOMIPS | FT_STAGE_UPLOAD,
		                               0, eTF_R32G32B32A32F, eTF_R32G32B32A32F))
		{
			return;
		}
	}

	// spawn water simulation job for next frame.
	{
		int32 nCurFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;
		if (m_frameIdWaterSim != nCurFrameID)
		{
			Vec4 pCurrParams0, pCurrParams1;
			gEnv->p3DEngine->GetOceanAnimationParams(pCurrParams0, pCurrParams1);

			// TODO: this water sim is shared with CREWaterOcean and CWaterStage. the sim result also could be shared.
			// Update sim settings
			auto& pParams0 = m_oceanAnimationParams[0];
			auto& pParams1 = m_oceanAnimationParams[1];
			if (WaterSimMgr()->NeedInit() || pCurrParams0.x != pParams0.x || pCurrParams0.y != pParams0.y ||
			    pCurrParams0.z != pParams0.z || pCurrParams0.w != pParams0.w || pCurrParams1.x != pParams1.x ||
			    pCurrParams1.y != pParams1.y || pCurrParams1.z != pParams1.z || pCurrParams1.w != pParams1.w)
			{
				pParams0 = pCurrParams0;
				pParams1 = pCurrParams1;
				WaterSimMgr()->Create(1.0, pParams0.x, pParams0.z, 1.0f, 1.0f);
			}

			// Copy data..
			if (CTexture::IsTextureExist(pTexture))
			{
				//const float fUpdateTime = 2.f*0.125f*gEnv->pTimer->GetCurrTime();
				const float fUpdateTime = 0.125f * gEnv->pTimer->GetCurrTime();// / clamp_tpl<float>(pParams1.x, 0.55f, 1.0f);

				void* pRawPtr = nullptr;
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
			m_frameIdWaterSim = nCurFrameID;
		}
	}

	// generate water normal from the simulation result.
	{
		auto& pass = m_passWaterNormalGen;

		if (pass.InputChanged())
		{
			static CCryNameTSCRC techName("WaterVolumesNormalGen");
			pass.SetTechnique(CShaderMan::s_shPostEffectsGame, techName, 0);
			pass.SetRenderTarget(0, CTexture::s_ptexWaterVolumeDDN);
			pass.SetState(GS_NODEPTHTEST);

			pass.SetTextureSamplerPair(0, pTexture, rd->m_nBilinearWrapSampler);
		}

		pass.BeginConstantUpdate();

		static CCryNameR pParamName("waterVolumesParams");
		Vec4 vParams = Vec4(64.0f, 64.0f, 64.0f, 64.0f);
		pass.SetConstant(pParamName, vParams);

		pass.Execute();
	}

	// generate mipmap.
	m_passWaterNormalMipmapGen.Execute(CTexture::s_ptexWaterVolumeDDN);
}

void CWaterStage::ExecuteOceanMaskGen(CRenderView* pRenderView)
{
	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;

	N3DEngineCommon::SOceanInfo& OceanInfo = rd->m_p3DEngineCommon.m_OceanInfo;
	const bool bOceanVolumeVisible = (OceanInfo.m_nOceanRenderFlags & OCR_OCEANVOLUME_VISIBLE) != 0;

	if (!bOceanVolumeVisible)
	{
		return;
	}

	if (!CTexture::IsTextureExist(m_pOceanMaskTex))
	{
		return;
	}

	CRY_ASSERT(pRenderView);

	PROFILE_LABEL_SCOPE("OCEAN_MASK_GEN");

	// prepare per pass device resource
	// this update must be called after water ripple stage's execute() because waterRippleLookup parameter depends on it.
	// this update must be called after updating N3DEngineCommon::SCausticInfo.
	PreparePerPassResources(pRenderView, false, ePass_OceanMaskGen);

	// TODO: replace this with clear command to command list.
	rd->FX_ClearTarget(m_pOceanMaskTex, Clr_Transparent);

	// render ocean surface to generate the mask to identify the pixels where ocean is behind the others and invisible.
	ExecuteSceneRenderPass(pRenderView, m_passOceanMaskGen, EFSLIST_WATER);
}

void CWaterStage::ExecuteWaterVolumeCausticsGen(N3DEngineCommon::SCausticInfo& causticInfo, CRenderView* pRenderView)
{
	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;

	CRY_ASSERT(pRenderView);
	CRY_ASSERT(rd->m_CurRenderEye == LEFT_EYE);

	PROFILE_LABEL_SCOPE("WATER_VOLUME_CAUSTICS_GEN");

	// update view-projection matrix to render water volumes to caustics gen texture.
	{
		// NOTE: caustics gen texture is generated only once if stereo rendering is activated.
		auto& rcam = pRenderView->GetRenderCamera(CCamera::eEye_Left);
		Vec3 vDir = rcam.ViewDir();
		Vec3 vPos = rcam.vOrigin;

		const float fMaxDistance = CRenderer::CV_r_watervolumecausticsmaxdistance;
		const float fOffsetDist = fMaxDistance * 0.25f;

		// Offset in viewing direction to maximize view distance.
		vPos += Vec3(vDir.x * fOffsetDist, vDir.y * fOffsetDist, 0.0f);

		// Snap to avoid some aliasing.
		const float fSnapRange = CRenderer::CV_r_watervolumecausticssnapfactor;
		if (fSnapRange > 0.05f) // don't bother snapping if the value is low.
		{
			water::GetSnapVector(vPos, fSnapRange);
		}

		Vec3 vEye = vPos + Vec3(0.0f, 0.0f, 10.0f);

		// Create the matrices.
		Matrix44A mOrthoMatr;
		Matrix44A mViewMatr;
		mOrthoMatr.SetIdentity();
		mViewMatr.SetIdentity();
		mathMatrixOrtho(&mOrthoMatr, fMaxDistance, fMaxDistance, 0.25f, 100.0f);
		mathMatrixLookAt(&mViewMatr, vEye, vPos, Vec3(0.0f, 1.0f, 0.0f));

		// Store for projection onto the scene.
		causticInfo.m_mCausticMatr = mViewMatr * mOrthoMatr;
		causticInfo.m_mCausticMatr.Transpose();
	}

	// prepare per pass device resource
	// this update must be called after water ripple stage's execute() because waterRippleLookup parameter depends on it.
	// this update must be called after updating N3DEngineCommon::SCausticInfo.
	PreparePerPassResources(pRenderView, false, ePass_CausticsGen);

	// TODO: replace this with clear command to command list.
	rd->FX_ClearTarget(CTexture::s_ptexWaterCaustics[0], Clr_Transparent);

	// render water volumes to caustics gen texture.
	{
		SThreadInfo& threadInfo = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID];
		const bool bReverseDepth = (threadInfo.m_PersFlags & RBPF_REVERSE_DEPTH) != 0;

		const auto renderList = EFSLIST_WATER;
		auto& pass = m_passWaterCausticsSrcGen;
		float fWidth = static_cast<float>(CTexture::s_ptexWaterCaustics[0]->GetWidth());
		float fHeight = static_cast<float>(CTexture::s_ptexWaterCaustics[0]->GetHeight());

		// need to set render target becuase CTexture::s_ptexWaterCaustics[0] is recreated when cvars are changed.
		pass.ExchangeRenderTarget(0, CTexture::s_ptexWaterCaustics[0]);

		D3DViewPort viewport = { 0.0f, 0.0f, fWidth, fHeight, 0.0f, 1.0f };
		rd->RT_SetViewport(0, 0, int32(viewport.Width), int32(viewport.Height));
		pass.SetViewport(viewport);

		CSceneRenderPass::EPassFlags passFlags = CSceneRenderPass::ePassFlags_None;
		passFlags |= bReverseDepth ? CSceneRenderPass::ePassFlags_ReverseDepth : CSceneRenderPass::ePassFlags_None;
		pass.SetFlags(passFlags);

		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
		pass.PrepareRenderPassForUse(commandList);

		auto& renderItemDrawer = pRenderView->GetDrawer();
		renderItemDrawer.InitDrawSubmission();

		pass.BeginExecution();
		pass.DrawRenderItems(pRenderView, renderList);
		pass.EndExecution();

		renderItemDrawer.JobifyDrawSubmission();

		renderItemDrawer.WaitForDrawSubmission();
	}

	// Dilate caustics gen texture.
	{
		PROFILE_LABEL_SCOPE("DILATION");

		auto& pass = m_passWaterCausticsDilation;

		if (pass.InputChanged())
		{
			static CCryNameTSCRC techName("WaterCausticsInfoDilate");
			pass.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);
			pass.SetRenderTarget(0, CTexture::s_ptexWaterCaustics[1]);
			pass.SetState(GS_NODEPTHTEST);
			pass.SetTextureSamplerPair(0, CTexture::s_ptexWaterCaustics[0], rd->m_nPointClampSampler);
		}

		pass.Execute();
	}

	// Super blur for alpha to mask edges of volumes.
	m_passBlurWaterCausticsGen0.Execute(CTexture::s_ptexWaterCaustics[1], CTexture::s_ptexWaterCaustics[0], 1.0f, 10.0f, true);

	////////////////////////////////////////////////
	// Procedural caustic generation
	// Generate the caustics map using the grid mesh.
	// for future:
	// - merge this somehow with shadow gen for correct projection/intersection with geometry (and lighting) - can use shadow map for position reconstruction of world around volume and project caustic geometry to it.
	// - try hardware tessellation to increase quality and reduce memory (perhaps do projection per volume instead of as a single pass, that way it's basically screen-space)
	bool bVertexUpdated = false;
	if (UpdateCausticsGrid(causticInfo, bVertexUpdated, gRenDev)) // returns true if the mesh is valid
	{
		{
			PROFILE_LABEL_SCOPE("CAUSTICS_GEN");

			auto& pass = m_passRenderCausticsGrid;
			auto& prim = m_causticsGridPrimitive;

			if (bVertexUpdated || prim.IsDirty())
			{
				pass.ClearPrimitives();

				CRenderMesh* pCausticQuadMesh = static_cast<CRenderMesh*>(causticInfo.m_pCausticQuadMesh.get());
				pCausticQuadMesh->CheckUpdate(pCausticQuadMesh->_GetVertexFormat(), 0);
				buffer_handle_t hVertexStream = pCausticQuadMesh->_GetVBStream(VSF_GENERAL);
				buffer_handle_t hIndexStream = pCausticQuadMesh->_GetIBStream();

				if (hVertexStream != ~0u && hIndexStream != ~0u)
				{
					auto* pTargetTex = CTexture::s_ptexWaterCaustics[0];
					D3DViewPort viewport;
					viewport.TopLeftX = 0.0f;
					viewport.TopLeftY = 0.0f;
					viewport.Width = static_cast<float>(pTargetTex->GetWidth());
					viewport.Height = static_cast<float>(pTargetTex->GetHeight());
					viewport.MinDepth = 0.0f;
					viewport.MaxDepth = 1.0f;

					pass.SetRenderTarget(0, pTargetTex);
					pass.SetViewport(viewport);

					prim.SetCullMode(eCULL_None);
					prim.SetRenderState(GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_A);

					prim.SetCustomVertexStream(hVertexStream, pCausticQuadMesh->_GetVertexFormat(), pCausticQuadMesh->GetStreamStride(VSF_GENERAL));
					prim.SetCustomIndexStream(hIndexStream, (sizeof(vtx_idx) == 2 ? RenderIndexType::Index16 : RenderIndexType::Index32));
					prim.SetDrawInfo(eptTriangleList, 0, 0, causticInfo.m_nIndexCount);

					static CCryNameTSCRC techName("WaterCausticsGen");
					prim.SetTechnique(CShaderMan::s_ShaderDeferredCaustics, techName, 0);

					prim.SetTexture(0, CTexture::s_ptexWaterCaustics[1], SResourceView::DefaultView, EShaderStage_Vertex);
					prim.SetSampler(0, rd->m_nTrilinearWrapSampler, EShaderStage_Vertex);

					prim.SetInlineConstantBuffer(eConstantBufferShaderSlot_PerView, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), EShaderStage_Vertex);

					pass.AddPrimitive(&prim);
				}
			}

			pass.Execute();
		}

		// NOTE: this is needed to avoid a broken consistency of the cached state in SSharedState,
		//       because d3d11 runtime removes SRV (s_ptexWaterCaustics[0]) from pixel shader slot 0 but it still remains in the cached state.
		CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->Reset();

		// Smooth out any inconsistencies in the caustic map (pixels, etc).
		m_passBlurWaterCausticsGen1.Execute(CTexture::s_ptexWaterCaustics[0], CTexture::s_ptexWaterCaustics[1], 1.0f, 1.0f);
	}
}

void CWaterStage::ExecuteReflection(CRenderView* pRenderView)
{
	CRY_ASSERT(pRenderView);

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	const SThreadInfo& threadInfo = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID];
	const bool bReverseDepth = (threadInfo.m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
	const int32 frameID = rd->GetFrameID(false);

	const auto renderList = EFSLIST_WATER;
	const uint32 batchMask = pRenderView->GetBatchFlags(renderList);

	if ((batchMask & FB_WATER_REFL)
	    && CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeRefl[0]))
	{
		PROFILE_LABEL_SCOPE("WATER_REFLECTION_GEN");

		const int32 currWaterVolID = GetCurrentFrameID(frameID);

		CTexture* pCurrWaterVolRefl = CTexture::s_ptexWaterVolumeRefl[currWaterVolID];

		// TODO: Is this copy redundant? both texture is same size so CTexture::s_ptexCurrSceneTarget is directly used as reflection source.
		const bool bBigDownsample = true; // TODO: use this flag for strech rect pass?
		m_passCopySceneTargetReflection.Execute(CTexture::s_ptexCurrSceneTarget, CTexture::s_ptexHDRTargetPrev);

		// draw render items to generate water reflection texture.
		{
			// TODO: Is m_CurDownscaleFactor needed for new graphics pipeline?
			const int32 nWidth = int32(pCurrWaterVolRefl->GetWidth() * (rd->m_RP.m_CurDownscaleFactor.x));
			const int32 nHeight = int32(pCurrWaterVolRefl->GetHeight() * (rd->m_RP.m_CurDownscaleFactor.y));

			// TODO: looks something wrong between rect and viewport?
			const RECT rect = { 0, pCurrWaterVolRefl->GetHeight() - nHeight, nWidth, nHeight };
			D3DViewPort viewport = { 0.0f, float(pCurrWaterVolRefl->GetHeight() - nHeight), float(nWidth), float(nHeight), 0.0f, 1.0f };
			//rd->RT_SetViewport(0, pCurrWaterVolRefl->GetHeight() - nHeight, nWidth, nHeight);
			rd->FX_ClearTarget(pCurrWaterVolRefl, Clr_Transparent, 1, &rect, true);

			auto& pass = m_passWaterReflectionGen;

			// alternates render targets along with updating per pass resource set.
			pass.ExchangeRenderTarget(0, pCurrWaterVolRefl);

			CSceneRenderPass::EPassFlags passFlags = CSceneRenderPass::ePassFlags_None;
			passFlags |= bReverseDepth ? CSceneRenderPass::ePassFlags_ReverseDepth : CSceneRenderPass::ePassFlags_None;
			pass.SetFlags(passFlags);
			pass.SetViewport(viewport);

			auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
			pass.PrepareRenderPassForUse(commandList);

			auto& renderItemDrawer = pRenderView->GetDrawer();
			renderItemDrawer.InitDrawSubmission();

			pass.BeginExecution();
			pass.DrawRenderItems(pRenderView, renderList);
			pass.EndExecution();

			renderItemDrawer.JobifyDrawSubmission();

			renderItemDrawer.WaitForDrawSubmission();
		}

		m_passWaterReflectionMipmapGen.Execute(pCurrWaterVolRefl);
	}
}

void CWaterStage::ExecuteSceneRenderPass(CRenderView* pRenderView, CSceneRenderPass& pass, ERenderListID renderList)
{
	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	const SThreadInfo& threadInfo = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID];
	const bool bReverseDepth = (threadInfo.m_PersFlags & RBPF_REVERSE_DEPTH) != 0;

	D3DViewPort viewport = { 0.f, 0.f, float(rd->m_MainViewport.nWidth), float(rd->m_MainViewport.nHeight), 0.0f, 1.0f };
	rd->RT_SetViewport(0, 0, int32(viewport.Width), int32(viewport.Height));
	pass.SetViewport(viewport);

	CSceneRenderPass::EPassFlags passFlags = CSceneRenderPass::ePassFlags_None;
	passFlags |= bReverseDepth ? CSceneRenderPass::ePassFlags_ReverseDepth : CSceneRenderPass::ePassFlags_None;
	pass.SetFlags(passFlags);

	auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
	pass.PrepareRenderPassForUse(commandList);

	auto& renderItemDrawer = pRenderView->GetDrawer();
	renderItemDrawer.InitDrawSubmission();

	pass.BeginExecution();
	pass.DrawRenderItems(pRenderView, renderList);
	pass.EndExecution();

	renderItemDrawer.JobifyDrawSubmission();

	renderItemDrawer.WaitForDrawSubmission();
}

int32 CWaterStage::GetCurrentFrameID(const int32 frameID) const
{
	return (frameID % 2);
}

int32 CWaterStage::GetPreviousFrameID(const int32 frameID) const
{
	return (frameID + 1) % 2;
}
