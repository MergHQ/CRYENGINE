// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "D3DVolumetricClouds.h"

#include <CryMath/Cry_Math.h>
#include "DriverD3D.h"
#include "D3DDeferredShading.h"
#include "D3DPostProcess.h"

//#pragma optimize("", off)

//////////////////////////////////////////////////////////////////////////
//#define ENABLE_DETAILED_PROFILING

//////////////////////////////////////////////////////////////////////////
const static int VCVerticalShadowTexSize = 8;
const static float VCMinSphereRadius = 100000.0f;
const static float VCMaxSphereRadius = 10000000.0f;
const static float VCDropletDensity = 1e9f; // 10^8 to 10^9 is typical droplets density in clouds.
const static float VCDropletRadius = 0.000015f;// 2 to 15 micro meter is typical droplet size in clouds.
const static float VCSigmaE = PI * VCDropletRadius * VCDropletRadius;
const static float VCScatterCoefficient = VCDropletDensity * VCSigmaE;
const static Vec3 VCBaseNoiseScale = Vec3(0.00003125f, 0.00003125f, 0.00003125f);
const static float VCMinRaymarchStepNum = 8.0f;
const static float VCEdgeNoiseScale = 19.876521f;
const static int VCFrameCountCycle = 1024;
const static uint VCMaxCloudBlockerNum = 4;
const static uint VCMaxCloudBlockerSSNum = 1;

struct VCCloudRenderContext
{
	int  cloudVolumeTexId;
	int  screenWidth;
	int  screenHeight;
	int  screenDepth;
	bool stereoReproj;
	bool bRightEye;
#if defined(VOLUMETRIC_FOG_SHADOWS)
	bool renderFogShadow;
#endif
};

struct VCCloudShaderParam
{
#if defined(VOLUMETRIC_FOG_SHADOWS)
	Vec4  volFogShadowDarkening;
	Vec4  volFogShadowDarkeningSunAmb;
#endif
	Vec3  skylightRayleighInScatter;
	Vec3  atmosphericScattering;
	float cloudiness;
	float altitude;
	float thickness;
	float sunSingleScatter;
	float sunLowOrderScatter;
	float sunLowOrderScatterAnisotropy;
	float sunHighOrderScatter;
	float skyLightingFactor;
	float groundLightingFactor;
	Vec3  groundAlbedo;
	float multiScatterAttenuation;
	float multiScatterPreservation;
	float powderFactor;
	float sunIntensityForAtmospheric;
	float edgeNoiseErode;
	float edgeNoiseAbs;
	float edgeTurbulence;
	float edgeThreshold;
	float radius;
	float horizonHeight;
	Vec3  cloudNoiseScale;
	float maxViewableDistance;
	float maxRayMarchDistance;
	float maxRayMarchStep;
	float extinction;
	float cloudShadowTilingSize;
	Vec3  vInvTiling;
	Vec3  invBaseTexTiling;
	Vec3  edgeTurbulenceNoiseScale;
	float maxGlobalCloudDensity;
	float minRescaleCloudTexDensity;
	float maxRescaleCloudTexDensity;
	float additionalNoiseIntensity;
	float shadingLOD;
	Vec3  cloudOffset;
	float frameCount;
	float jitteredSamplingPosition;
};

static int VCGetDownscaleRatio()
{
	return clamp_tpl<int>(CRenderer::CV_r_VolumetricClouds, 1, 2); // half and quarter res are available.
}

// calculate Van der Corput sequence
static float VCGetVdC(uint index)
{
	index = (index << 16u) | (index >> 16u);
	index = ((index & 0x55555555u) << 1u) | ((index & 0xAAAAAAAAu) >> 1u);
	index = ((index & 0x33333333u) << 2u) | ((index & 0xCCCCCCCCu) >> 2u);
	index = ((index & 0x0F0F0F0Fu) << 4u) | ((index & 0xF0F0F0F0u) >> 4u);
	index = ((index & 0x00FF00FFu) << 8u) | ((index & 0xFF00FF00u) >> 8u);
	return float(index) / float(0xFFFFFFFFu);
}

static float VCGetCloudBlockerDecayDistance(float decayStart, float decayEnd)
{
	float decayDistance = (decayEnd - decayStart);
	decayDistance = (abs(decayDistance) < 1.0f) ? ((decayDistance < 0.0f) ? -1.0f : 1.0f) : decayDistance;
	return decayDistance;
}

static int VCGetCloudShadowTexSize()
{
	int size = clamp_tpl<int>(CRenderer::CV_r_VolumetricCloudsShadowResolution, 16, 512);
	size = size - (size % 16); // keep multiple of 16.
	return size;
}

static int VCGetCloudRaymarchStepNum()
{
	int num = clamp_tpl<int>(CRenderer::CV_r_VolumetricCloudsRaymarchStepNum, 16, 256);
	num = num - (num % 16); // keep multiple of 16.
	return num;
}

//////////////////////////////////////////////////////////////////////////
CVolumetricCloudManager::CVolumetricCloudManager()
	: m_enableVolumetricCloudsShadow(false)
	, m_cleared(MaxFrameNum)
	, m_tick(-1)
{
	for (int e = 0; e < MaxEyeNum; ++e)
	{
		m_nUpdateFrameID[e] = -1;

		for (int i = 0; i < MaxFrameNum; ++i)
		{
			m_viewProj[e][i].SetIdentity();
		}

		for (int i = 0; i < 2; ++i)
		{
			m_downscaledTex[e][i] = NULL;
			m_downscaledMinTex[e][i] = NULL;
		}
		m_prevScaledDepthBuffer[e] = NULL;
	}

	m_cloudDepthBuffer = NULL;
	m_downscaledTemp = NULL;
	m_downscaledLeftEye = NULL;
	m_cloudDensityTex = NULL;
	m_cloudShadowTex = NULL;

	m_blockerPosArray.Reserve(VCMaxCloudBlockerNum);
	m_blockerParamArray.Reserve(VCMaxCloudBlockerNum);
	for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
	{
		m_blockerArray[i].Reserve(VCMaxCloudBlockerNum);
	}

	m_blockerSSPosArray.Reserve(VCMaxCloudBlockerSSNum);
	m_blockerSSParamArray.Reserve(VCMaxCloudBlockerSSNum);
	for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
	{
		m_blockerSSArray[i].Reserve(VCMaxCloudBlockerSSNum);
	}
}

CVolumetricCloudManager::~CVolumetricCloudManager()
{
	for (int e = 0; e < MaxEyeNum; ++e)
	{
		for (int i = 0; i < 2; ++i)
		{
			SAFE_RELEASE_FORCE(m_downscaledTex[e][i]);
			SAFE_RELEASE_FORCE(m_downscaledMinTex[e][i]);
		}
		SAFE_RELEASE_FORCE(m_prevScaledDepthBuffer[e]);
	}
	SAFE_RELEASE_FORCE(m_cloudDepthBuffer);
	SAFE_RELEASE_FORCE(m_downscaledTemp);
	SAFE_RELEASE_FORCE(m_downscaledLeftEye);
	SAFE_RELEASE_FORCE(m_cloudDensityTex);
	SAFE_RELEASE_FORCE(m_cloudShadowTex);
}

void CVolumetricCloudManager::PushCloudBlocker(const Vec3& pos, const Vec3& param, int flag)
{
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
	assert(nThreadID == gcpRendD3D->m_pRT->GetThreadList());

	auto& array = (flag == 0) ? m_blockerArray[nThreadID] : m_blockerSSArray[nThreadID];
	uint MaxElementNum = (flag == 0) ? VCMaxCloudBlockerNum : VCMaxCloudBlockerSSNum;

	if (array.size() >= MaxElementNum)
	{
		// TODO: show skipped Cloud Blocker number on the screen.
		return;
	}

	SCloudBlocker temp;
	temp.pos = pos;
	temp.param = param;

	array.Add(temp);
}

bool CVolumetricCloudManager::IsRenderable() const
{
	//const int nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
	//const int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

	static ICVar* pClouds = gEnv->pConsole->GetCVar("e_Clouds");
	bool renderClouds = false;
	if (pClouds && (pClouds->GetIVal() != 0))
	{
		renderClouds = true;
	}

	return renderClouds && (CRenderer::CV_r_VolumetricClouds > 0);// && (nCurRecLevel <= 1);
}

Vec4 CVolumetricCloudManager::GetVolumetricCloudShadowParam(CD3D9Renderer* r, const Vec2& cloudOffset, const Vec2& vTiling) const
{
	if (!r)
	{
		return Vec4(0, 0, 0, 0);
	}

	const int shadowTexSize = VCGetCloudShadowTexSize();
	const Vec2 texSize((f32)shadowTexSize, (f32)shadowTexSize);
	const Vec2 vInvTiling(1.0f / vTiling.x, 1.0f / vTiling.y);
	const Vec2 voxelLength(vTiling.x / texSize.x, vTiling.y / texSize.y);
	const Vec2 invVoxelLength(1.0f / voxelLength.x, 1.0f / voxelLength.y);

	const CRenderCamera& rc = r->GetRCamera();
	const Vec3& camPos = rc.vOrigin;

	// shift world position along with camera movement, which is aligned to certain interval.
	Vec2 worldAlignmentOffset;
	worldAlignmentOffset.x = std::floor(camPos.x * invVoxelLength.x) * voxelLength.x;
	worldAlignmentOffset.y = std::floor(camPos.y * invVoxelLength.y) * voxelLength.y;

	// offset wind movement to align the position to certain interval.
	float intpart;
	worldAlignmentOffset.x -= std::modf(cloudOffset.x * invVoxelLength.x, &intpart) * voxelLength.x;
	worldAlignmentOffset.y -= std::modf(cloudOffset.y * invVoxelLength.y, &intpart) * voxelLength.y;

	return Vec4(worldAlignmentOffset.x, worldAlignmentOffset.y, vInvTiling.x, vInvTiling.y);
}

void CVolumetricCloudManager::BuildCloudBlockerList()
{
	// build blocker list for current frame.
	m_blockerPosArray.SetUse(0);
	m_blockerParamArray.SetUse(0);
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
	auto& array = m_blockerArray[nThreadID];
	uint numBlocker = array.Num();
	for (uint i = 0; i < numBlocker; ++i)
	{
		Vec3& pos = array[i].pos;
		m_blockerPosArray.push_back(Vec4(pos.x, pos.y, pos.z, (float)numBlocker));

		Vec3& param = array[i].param;
		const float decayStart = param.x;
		const float decayEnd = param.y;
		const float decayDistance = VCGetCloudBlockerDecayDistance(decayStart, decayEnd); // to avoid floating point exception.
		const float invDecayDist = 1.0f / decayDistance;
		const float decayInfluence = param.z;
		const float invDecayInfluence = 1.0f - decayInfluence;
		m_blockerParamArray.push_back(Vec4(invDecayDist, decayStart, decayInfluence, invDecayInfluence));
	}

	// reset blockers array for the next frame.
	array.SetUse(0);
}

void CVolumetricCloudManager::BuildCloudBlockerSSList()
{
	// build screen space blocker list for current frame.
	m_blockerSSPosArray.SetUse(0);
	m_blockerSSParamArray.SetUse(0);
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
	auto& array = m_blockerSSArray[nThreadID];

	if (array.Num() > 0)
	{
		Vec3& pos = array[0].pos;
		m_blockerSSPosArray.push_back(Vec4(pos.x, pos.y, pos.z, 0.0f));

		Vec3& param = array[0].param;
		const float decayIntensity = clamp_tpl(1.0f - param.z, 0.0f, 1.0f) * 10.0f;
		m_blockerSSParamArray.push_back(Vec4(param.y, decayIntensity, 0.0f, 0.0f));
	}

	// reset blockers array for the next frame.
	array.SetUse(0);
}

bool CVolumetricCloudManager::CreateResources()
{
	static const ETEX_Format shadowTexFormat = eTF_R11G11B10F;//eTF_R16G16B16A16F;
	static const int nRTFlags = FT_NOMIPS | FT_USAGE_UNORDERED_ACCESS;

	if (CTexture::s_ptexVolCloudShadow)
	{
		const int shadowTexSize = VCGetCloudShadowTexSize();
		const int w = shadowTexSize;
		const int h = shadowTexSize;
		const int d = VCVerticalShadowTexSize;

		int width = CTexture::s_ptexVolCloudShadow->GetWidth();
		int height = CTexture::s_ptexVolCloudShadow->GetHeight();

		if (w != width || h != height)
		{
			if (!CTexture::s_ptexVolCloudShadow->Create3DTexture(w, h, d, 1, nRTFlags, NULL, shadowTexFormat, shadowTexFormat))
			{
				CryFatalError("Couldn't allocate texture.");
			}
		}
	}

	bool resetReprojection = false;
	const int scale = VCGetDownscaleRatio();
	int scaledWidth = gRenDev->GetWidth() >> scale;
	int scaledHeight = gRenDev->GetHeight() >> scale;
	int depth = VCGetCloudRaymarchStepNum();
	const char* tname[MaxEyeNum][2] =
	{
		{
			"$VolCloudScaled0",
			"$VolCloudScaled1",
		},
		{
			"$VolCloudScaled0_R",
			"$VolCloudScaled1_R",
		},
	};
	const char* tnameMin[MaxEyeNum][2] =
	{
		{
			"$VolCloudScaledMin0",
			"$VolCloudScaledMin1",
		},
		{
			"$VolCloudScaledMin0_R",
			"$VolCloudScaledMin1_R",
		},
	};
	const char* tnamePrevDepth[MaxEyeNum] =
	{
		"$VolCloudPrevDepthScaled",
		"$VolCloudPrevDepthScaled_R",
	};
	bool bStereo = gRenDev->m_bDualStereoSupport;
	bool stereoReproj = bStereo && CRenderer::CV_r_VolumetricCloudsStereoReprojection != 0;

	for (int e = 0; e < MaxEyeNum; ++e)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (m_downscaledTex[e][i] &&
			    (scaledWidth != m_downscaledTex[e][i]->GetWidth() || scaledHeight != m_downscaledTex[e][i]->GetHeight()))
			{
				SAFE_RELEASE_FORCE(m_downscaledTex[e][i]);
			}

			if (!m_downscaledTex[e][i] && (e == 0 || (e == 1 && bStereo)))
			{
				int flags = FT_NOMIPS | FT_USAGE_RENDERTARGET;
				const ETEX_Format texFormat = eTF_R16G16B16A16F;
				m_downscaledTex[e][i] = CTexture::Create2DTexture(tname[e][i], scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

				resetReprojection = true;
			}
		}

		for (int i = 0; i < 2; ++i)
		{
			if (m_downscaledMinTex[e][i] &&
			    (scaledWidth != m_downscaledMinTex[e][i]->GetWidth() || scaledHeight != m_downscaledMinTex[e][i]->GetHeight()))
			{
				SAFE_RELEASE_FORCE(m_downscaledMinTex[e][i]);
			}

			if (!m_downscaledMinTex[e][i] && (e == 0 || (e == 1 && bStereo)))
			{
				int flags = FT_NOMIPS | FT_USAGE_RENDERTARGET;
				const ETEX_Format texFormat = eTF_R16G16B16A16F;
				m_downscaledMinTex[e][i] = CTexture::Create2DTexture(tnameMin[e][i], scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

				resetReprojection = true;
			}
		}

		{
			if (m_prevScaledDepthBuffer[e] &&
			    (scaledWidth != m_prevScaledDepthBuffer[e]->GetWidth() || scaledHeight != m_prevScaledDepthBuffer[e]->GetHeight()))
			{
				SAFE_RELEASE_FORCE(m_prevScaledDepthBuffer[e]);
			}

			if (!m_prevScaledDepthBuffer[e] && (e == 0 || (e == 1 && bStereo)))
			{
				int flags = FT_NOMIPS | FT_USAGE_RENDERTARGET;
				const ETEX_Format texFormat = eTF_R16G16F;
				m_prevScaledDepthBuffer[e] = CTexture::Create2DTexture(tnamePrevDepth[e], scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

				resetReprojection = true;
			}
		}
	}

	{
		if (m_cloudDepthBuffer &&
		    (scaledWidth != m_cloudDepthBuffer->GetWidth() || scaledHeight != m_cloudDepthBuffer->GetHeight()))
		{
			SAFE_RELEASE_FORCE(m_cloudDepthBuffer);
		}

		if (!m_cloudDepthBuffer)
		{
			int flags = FT_NOMIPS | FT_USAGE_UNORDERED_ACCESS;
			const ETEX_Format texFormat = eTF_R32F;
			m_cloudDepthBuffer = CTexture::Create2DTexture("$VolCloudDepthScaled", scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

			resetReprojection = true;
		}
	}

	{
		if (m_downscaledTemp &&
		    (scaledWidth != m_downscaledTemp->GetWidth() || scaledHeight != m_downscaledTemp->GetHeight()))
		{
			SAFE_RELEASE_FORCE(m_downscaledTemp);
		}

		if (!m_downscaledTemp)
		{
			int flags = FT_NOMIPS | FT_USAGE_UNORDERED_ACCESS;
			const ETEX_Format texFormat = eTF_R16G16B16A16F;
			m_downscaledTemp = CTexture::Create2DTexture("$VolCloudTemp", scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

			resetReprojection = true;
		}
	}

	{
		if (m_downscaledLeftEye &&
		    (scaledWidth != m_downscaledLeftEye->GetWidth() || scaledHeight != m_downscaledLeftEye->GetHeight()
		     || !stereoReproj))
		{
			SAFE_RELEASE_FORCE(m_downscaledLeftEye);
		}

		if (!m_downscaledLeftEye && stereoReproj)
		{
			int flags = FT_NOMIPS | FT_USAGE_RENDERTARGET;
			const ETEX_Format texFormat = eTF_R16G16B16A16F;
			m_downscaledLeftEye = CTexture::Create2DTexture("$VolCloudLeftEye", scaledWidth, scaledHeight, 1, flags, NULL, texFormat, texFormat);

			resetReprojection = true;
		}
	}

	if (CRenderer::CV_r_VolumetricCloudsPipeline != 0)
	{
		{
			if (m_cloudDensityTex &&
			    (scaledWidth != m_cloudDensityTex->GetWidth()
			     || scaledHeight != m_cloudDensityTex->GetHeight()
			     || depth != m_cloudDensityTex->GetDepth()))
			{
				SAFE_RELEASE_FORCE(m_cloudDensityTex);
			}

			if (!m_cloudDensityTex)
			{
				int flags = FT_NOMIPS | FT_USAGE_UNORDERED_ACCESS;
				const ETEX_Format texFormat = eTF_R16F;
				m_cloudDensityTex = CTexture::Create3DTexture("$VolCloudDensityTmp", scaledWidth, scaledHeight, depth, 1, flags, NULL, texFormat, texFormat);
			}
		}
		{
			if (m_cloudShadowTex &&
			    (scaledWidth != m_cloudShadowTex->GetWidth() || scaledHeight != m_cloudShadowTex->GetHeight() || depth != m_cloudShadowTex->GetDepth()))
			{
				SAFE_RELEASE_FORCE(m_cloudShadowTex);
			}

			if (!m_cloudShadowTex)
			{
				int flags = FT_NOMIPS | FT_USAGE_UNORDERED_ACCESS;
				const ETEX_Format texFormat = eTF_R11G11B10F;
				m_cloudShadowTex = CTexture::Create3DTexture("$VolCloudShadowTmp", scaledWidth, scaledHeight, depth, 1, flags, NULL, texFormat, texFormat);
			}
		}
	}
	else
	{
		SAFE_RELEASE_FORCE(m_cloudDensityTex);
		SAFE_RELEASE_FORCE(m_cloudShadowTex);
	}

	if (resetReprojection)
	{
		m_cleared = MaxFrameNum;
	}

	return AreTexturesValid();
}

bool CVolumetricCloudManager::AreTexturesValid() const
{
	bool bDownscaledRTs = CTexture::IsTextureExist(m_cloudDepthBuffer) && CTexture::IsTextureExist(m_downscaledTemp);
	const int eyeNum = gRenDev->m_bDualStereoSupport ? MaxEyeNum : 1;
	for (int e = 0; e < eyeNum; ++e)
	{
		for (int i = 0; i < 2; ++i)
		{
			bDownscaledRTs = bDownscaledRTs && CTexture::IsTextureExist(m_downscaledTex[e][i]);
			bDownscaledRTs = bDownscaledRTs && CTexture::IsTextureExist(m_downscaledMinTex[e][i]);
		}
		bDownscaledRTs = bDownscaledRTs && CTexture::IsTextureExist(m_prevScaledDepthBuffer[e]);
	}

	bool bStereo = gRenDev->m_bDualStereoSupport;
	bool stereoReproj = bStereo && CRenderer::CV_r_VolumetricCloudsStereoReprojection != 0;
	bDownscaledRTs = bDownscaledRTs && (!stereoReproj || CTexture::IsTextureExist(m_downscaledLeftEye));

	if (CRenderer::CV_r_VolumetricCloudsPipeline != 0)
	{
		bDownscaledRTs = bDownscaledRTs && CTexture::IsTextureExist(m_cloudDensityTex) && CTexture::IsTextureExist(m_cloudShadowTex);
	}

	return (CTexture::IsTextureExist(CTexture::s_ptexVolCloudShadow) && bDownscaledRTs);
}

void CVolumetricCloudManager::GenerateVolumeTextures()
{
	const int nCurrentFrameID = gcpRendD3D->GetFrameID(false);
	bool bLeftEye = nCurrentFrameID != m_nUpdateFrameID[gRenDev->m_CurRenderEye] && gRenDev->m_CurRenderEye == StereoEye::LEFT_EYE;
	bool bRightEye = nCurrentFrameID != m_nUpdateFrameID[gRenDev->m_CurRenderEye] && gRenDev->m_CurRenderEye == StereoEye::RIGHT_EYE;

	if (bLeftEye)
	{
		m_enableVolumetricCloudsShadow = false;
	}

	if (!IsRenderable())
		return;

	// Initialize the resources.
	if (!CreateResources())
		return;

	if (!AreTexturesValid())
		return;

	if (bLeftEye)
	{
		++m_tick;

		BuildCloudBlockerList();
		BuildCloudBlockerSSList();
	}

	// update previous view-projection matrix
	if (bLeftEye || bRightEye)
	{
		CD3D9Renderer* const __restrict rd = gcpRendD3D;
		Matrix44 mViewProj = rd->m_ViewMatrix * rd->m_ProjMatrix;
		Matrix44& mViewport = SD3DPostEffectsUtils::GetInstance().m_pScaleBias;

		m_viewProj[gRenDev->m_CurRenderEye][m_tick % MaxFrameNum] = mViewProj * mViewport;
	}

	// Only generate once per frame.
	if (bLeftEye)
	{
		//PROFILE_LABEL_SCOPE( "VOLUMETRIC_CLOUD_TEX_GEN" );

		GenerateCloudShadowVol();
	}

	m_nUpdateFrameID[gRenDev->m_CurRenderEye] = nCurrentFrameID;
}

void CVolumetricCloudManager::GenerateCloudShadowVol()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	CDeviceManager& dm = rd->m_DevMan;
	SCGParamsPF& PF = rd->m_cEF.m_PF[rp.m_nProcessThreadID];

	PROFILE_LABEL_SCOPE("VOLUMETRIC_CLOUD_SHADOWS_GEN");

	// Fetch the cloud parameters.
	Vec3 cloudGenParams;
	Vec3 cloudTurbulenceParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GEN_PARAMS, cloudGenParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE, cloudTurbulenceParams);

	Vec3 cloudGlobalNoiseScale;
	Vec3 cloudTurbulenceNoiseScale;
	Vec3 cloudTurbulenceNoiseParams;
	Vec3 cloudDensityParams;
	Vec3 cloudMiscParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GLOBAL_NOISE_SCALE, cloudGlobalNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_SCALE, cloudTurbulenceNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_PARAMS, cloudTurbulenceNoiseParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_DENSITY_PARAMS, cloudDensityParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_MISC_PARAM, cloudMiscParams);

	assert(CTexture::s_ptexVolumetricFogDensity != NULL);
	const int widthVolShadowTex = CTexture::s_ptexVolCloudShadow->GetWidth();
	const int heightVolShadowTex = CTexture::s_ptexVolCloudShadow->GetHeight();
	const int depthVolShadowTex = CTexture::s_ptexVolCloudShadow->GetDepth();

	// need to manually apply several shader flags
	uint64 nFlagsShaderRTSave = rp.m_FlagsShader_RT;
	rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4]);

	const int texId = rd->GetVolumetricCloudTextureId();
	CTexture* cloudVolumeTex = NULL;
	if (texId > 0)
	{
		cloudVolumeTex = CTexture::GetByID(texId);
		rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
	}

	// decay cloud density with cloud blocker entity.
	if (m_blockerPosArray.Num() > 0)
	{
		rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
	}

	// needs to reset due to a mismatch between real device states and cached states.
	CTexture::ResetTMUs();

	CShader* pShader = CShaderMan::s_ShaderClouds;
	static CCryNameTSCRC shaderName = "CloudShadowGen";
	bool result = GetUtils().ShBeginPass(pShader, shaderName, FEF_DONTSETSTATES);

	if (!result) //This could happen if shader is being compiled asynchronously
		return;

	// Set UAV
	ID3D11UnorderedAccessView* pUAVs[1] = {
		(ID3D11UnorderedAccessView*)CTexture::s_ptexVolCloudShadow->GetDeviceUAV(),
	};
	dm.BindUAV(CDeviceManager::TYPE_CS, pUAVs, NULL, 0, 1);

	// set cloud base texture.
	if (cloudVolumeTex)
	{
		cloudVolumeTex->Apply(15, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Compute);
	}

	const float cloudiness = cloudGenParams.x;
	const float altitude = cloudGenParams.y;
	const float thickness = max(1e-3f, cloudGenParams.z);
	const Vec3 cloudNoiseScale = VCBaseNoiseScale.CompMul(cloudGlobalNoiseScale);
	const float edgeNoiseErode = cloudTurbulenceNoiseParams.x;
	const float edgeNoiseAbs = cloudTurbulenceNoiseParams.y;
	const float edgeTurbulence = (edgeNoiseErode > 0.0f) ? 2.0f * cloudTurbulenceParams.x : 8.0f * cloudTurbulenceParams.x;
	const float edgeThreshold = cloudTurbulenceParams.y + 1e-4f;
	const float absorptionFactor = cloudTurbulenceParams.z; // 1 to 5 percent is absorbed in clouds at most.

	const float extinction = VCScatterCoefficient + (VCScatterCoefficient * absorptionFactor);

	const float cloudShadowTilingSize = cloudMiscParams.z;
	const Vec3 vTiling(cloudShadowTilingSize, cloudShadowTilingSize, thickness);

	const Vec3 texSize((f32)widthVolShadowTex, (f32)heightVolShadowTex, (f32)depthVolShadowTex);
	const Vec3 voxelLength(vTiling.x / texSize.x, vTiling.y / texSize.y, vTiling.z / texSize.z);

	const Vec3& baseTexTiling = PF.pVolCloudTilingSize;
	const Vec3 invBaseTexTiling(1.0f / baseTexTiling.x, 1.0f / baseTexTiling.y, 1.0f / baseTexTiling.z);

	const Vec3 edgeTurbulenceNoiseScale = VCBaseNoiseScale.CompMul(cloudTurbulenceNoiseScale * VCEdgeNoiseScale);

	const float maxGlobalCloudDensity = cloudDensityParams.x;
	const float minRescaleCloudTexDensity = cloudDensityParams.y;
	const float maxRescaleCloudTexDensity = cloudDensityParams.z;

	const float additionalNoiseIntensity = cloudMiscParams.x;

	const Vec3 cloudOffset(PF.pVolCloudTilingOffset.x, PF.pVolCloudTilingOffset.y, PF.pVolCloudTilingOffset.z - altitude);

	static CCryNameR shadowGenParamName("CloudShadowGenParams");
	Vec4 shadowGenParam(altitude, thickness, cloudiness, extinction);
	pShader->FXSetCSFloat(shadowGenParamName, &shadowGenParam, 1);

	static CCryNameR tilingParamName("CloudShadowTilingParams");
	Vec4 tilingParam(vTiling.x, vTiling.y, 0, 0);
	pShader->FXSetCSFloat(tilingParamName, &tilingParam, 1);

	static CCryNameR texParamName("CloudShadowTexParams");
	Vec4 texParam(voxelLength.x, voxelLength.y, voxelLength.z, 0);
	pShader->FXSetCSFloat(texParamName, &texParam, 1);

	static CCryNameR cloudShadowNoiseParamName("CloudShadowNoiseParams");
	Vec4 cloudShadowNoiseParam(cloudNoiseScale.x, cloudNoiseScale.y, cloudNoiseScale.z, 0);
	pShader->FXSetCSFloat(cloudShadowNoiseParamName, &cloudShadowNoiseParam, 1);

	static CCryNameR cloudShadowBaseScaleParamName("CloudShadowBaseScaleParams");
	Vec4 cloudShadowBaseScaleParam(invBaseTexTiling.x, invBaseTexTiling.y, invBaseTexTiling.z, 0);
	pShader->FXSetCSFloat(cloudShadowBaseScaleParamName, &cloudShadowBaseScaleParam, 1);

	static CCryNameR cloudShadowBaseOffsetParamName("CloudShadowBaseOffsetParams");
	Vec4 cloudShadowBaseOffsetParam(cloudOffset.x, cloudOffset.y, cloudOffset.z, 0);
	pShader->FXSetCSFloat(cloudShadowBaseOffsetParamName, &cloudShadowBaseOffsetParam, 1);

	static CCryNameR cloudShadowDensityParamName("CloudShadowDensityParams");
	Vec4 cloudShadowDensityParam(maxGlobalCloudDensity, minRescaleCloudTexDensity, maxRescaleCloudTexDensity, additionalNoiseIntensity);
	pShader->FXSetCSFloat(cloudShadowDensityParamName, &cloudShadowDensityParam, 1);

	static CCryNameR cloudShadowEdgeNoiseScaleParamName("CloudShadowEdgeNoiseScaleParams");
	Vec4 cloudShadowEdgeNoiseScaleParam(edgeTurbulenceNoiseScale.x, edgeTurbulenceNoiseScale.y, edgeTurbulenceNoiseScale.z, 0);
	pShader->FXSetCSFloat(cloudShadowEdgeNoiseScaleParamName, &cloudShadowEdgeNoiseScaleParam, 1);

	static CCryNameR cloudShadowEdgeTurbulenceParamName("CloudShadowEdgeTurbulenceParams");
	Vec4 cloudShadowEdgeTurbulenceParam(edgeTurbulence, edgeThreshold, edgeNoiseErode, edgeNoiseAbs);
	pShader->FXSetCSFloat(cloudShadowEdgeTurbulenceParamName, &cloudShadowEdgeTurbulenceParam, 1);

	static CCryNameR vcCloudBlockerPosName("vcCloudBlockerPos");
	pShader->FXSetCSFloat(vcCloudBlockerPosName, m_blockerPosArray.begin(), 4);

	static CCryNameR vcCloudBlockerParamName("vcCloudBlockerParam");
	pShader->FXSetCSFloat(vcCloudBlockerParamName, m_blockerParamArray.begin(), 4);

	// Commit shader params
	rd->FX_Commit();

	// Tile sizes defined in [numthreads()] in shader
	const uint32 nTileSizeX = 16;
	const uint32 nTileSizeY = 16;
	const uint32 nTileSizeZ = 1;

	uint32 dispatchSizeX = widthVolShadowTex / nTileSizeX + (widthVolShadowTex % nTileSizeX > 0 ? 1 : 0);
	uint32 dispatchSizeY = heightVolShadowTex / nTileSizeY + (heightVolShadowTex % nTileSizeY > 0 ? 1 : 0);
	uint32 dispatchSizeZ = depthVolShadowTex / nTileSizeZ + (depthVolShadowTex % nTileSizeZ > 0 ? 1 : 0);

	dm.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

	GetUtils().ShEndPass();

	rp.m_FlagsShader_RT = nFlagsShaderRTSave;

	ID3D11UnorderedAccessView* pUAVNull = NULL;
	dm.BindUAV(CDeviceManager::TYPE_CS, &pUAVNull, NULL, 0, 1);

	rd->FX_Commit();
	dm.CommitDeviceStates();

	m_enableVolumetricCloudsShadow = true;
}

void CVolumetricCloudManager::MakeCloudShaderParam(VCCloudRenderContext& context, VCCloudShaderParam& param)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	SCGParamsPF& PF = rd->m_cEF.m_PF[rp.m_nProcessThreadID];

	Vec3 cloudGenParams;
	Vec3 cloudScatteringLowParams;
	Vec3 cloudScatteringHighParams;
	Vec3 cloudGroundColorParams;
	Vec3 cloudScatteringMulti;
	Vec3 cloudWindAndAtmosphericParams;
	Vec3 cloudTurbulenceParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GEN_PARAMS, cloudGenParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_LOW, cloudScatteringLowParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_HIGH, cloudScatteringHighParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GROUND_COLOR, cloudGroundColorParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_MULTI, cloudScatteringMulti);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_WIND_ATMOSPHERIC, cloudWindAndAtmosphericParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE, cloudTurbulenceParams);

	Vec3 cloudEnvParams;
	Vec3 cloudGlobalNoiseScale;
	Vec3 cloudRenderParams;
	Vec3 cloudTurbulenceNoiseScale;
	Vec3 cloudTurbulenceNoiseParams;
	Vec3 cloudDensityParams;
	Vec3 cloudMiscParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_ENV_PARAMS, cloudEnvParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GLOBAL_NOISE_SCALE, cloudGlobalNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_RENDER_PARAMS, cloudRenderParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_SCALE, cloudTurbulenceNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_PARAMS, cloudTurbulenceNoiseParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_DENSITY_PARAMS, cloudDensityParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_MISC_PARAM, cloudMiscParams);

	context.cloudVolumeTexId = rd->GetVolumetricCloudTextureId();

	assert(m_cloudShadowTex != NULL);
	context.screenWidth = m_cloudShadowTex->GetWidth();
	context.screenHeight = m_cloudShadowTex->GetHeight();
	context.screenDepth = m_cloudShadowTex->GetDepth();

	bool bStereo = gRenDev->m_bDualStereoSupport;
	context.stereoReproj = bStereo && CRenderer::CV_r_VolumetricCloudsStereoReprojection != 0;
	context.bRightEye = gRenDev->m_CurRenderEye == StereoEye::RIGHT_EYE;

#if defined(VOLUMETRIC_FOG_SHADOWS)
	context.renderFogShadow = rd->m_bVolFogShadowsEnabled;
	if (context.renderFogShadow)
	{
		Vec3 volFogShadowDarkeningP;
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);
		param.volFogShadowDarkening = Vec4(volFogShadowDarkeningP, 0);

		const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
		const float bSun = 1.0f - aSun;
		const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
		const float bAmb = 1.0f - aAmb;
		param.volFogShadowDarkeningSunAmb = Vec4(aSun, bSun, aAmb, bAmb);
	}
#endif

	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKYLIGHT_RAYLEIGH_INSCATTER, param.skylightRayleighInScatter);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_ATMOSPHERIC_SCATTERING, param.atmosphericScattering);

	param.cloudiness = cloudGenParams.x;
	param.altitude = cloudGenParams.y;
	param.thickness = max(1e-3f, cloudGenParams.z);
	param.sunSingleScatter = cloudScatteringLowParams.x;
	param.sunLowOrderScatter = cloudScatteringLowParams.y;
	param.sunLowOrderScatterAnisotropy = cloudScatteringLowParams.z;
	param.sunHighOrderScatter = cloudScatteringHighParams.x;
	param.skyLightingFactor = cloudScatteringHighParams.y;
	param.groundLightingFactor = cloudScatteringHighParams.z;
	param.groundAlbedo = cloudGroundColorParams;
	param.multiScatterAttenuation = cloudScatteringMulti.x;
	param.multiScatterPreservation = cloudScatteringMulti.y;
	param.powderFactor = cloudScatteringMulti.z;
	param.sunIntensityForAtmospheric = cloudWindAndAtmosphericParams.z;
	param.edgeNoiseErode = cloudTurbulenceNoiseParams.x;
	param.edgeNoiseAbs = cloudTurbulenceNoiseParams.y;
	param.edgeTurbulence = (param.edgeNoiseErode > 0.0f) ? 2.0f * cloudTurbulenceParams.x : 8.0f * cloudTurbulenceParams.x;
	param.edgeThreshold = cloudTurbulenceParams.y + 1e-4f;

	param.radius = clamp_tpl<float>(cloudEnvParams.x, VCMinSphereRadius, VCMaxSphereRadius);
	param.horizonHeight = cloudEnvParams.z;
	param.cloudNoiseScale = VCBaseNoiseScale.CompMul(cloudGlobalNoiseScale);
	param.maxViewableDistance = cloudRenderParams.x;
	param.maxRayMarchDistance = cloudRenderParams.y;
	param.maxRayMarchStep = float(VCGetCloudRaymarchStepNum());

	float absorptionFactor = cloudTurbulenceParams.z; // 1 to 5 percent is absorbed in clouds at most.
	param.extinction = VCScatterCoefficient + (VCScatterCoefficient * absorptionFactor);

	param.cloudShadowTilingSize = cloudMiscParams.z;
	const Vec3 vTiling(param.cloudShadowTilingSize, param.cloudShadowTilingSize, param.thickness);
	param.vInvTiling = Vec3(1.0f / vTiling.x, 1.0f / vTiling.y, 1.0f / vTiling.z);

	const Vec3& baseTexTiling = PF.pVolCloudTilingSize;
	param.invBaseTexTiling = Vec3(1.0f / baseTexTiling.x, 1.0f / baseTexTiling.y, 1.0f / baseTexTiling.z);

	param.edgeTurbulenceNoiseScale = VCBaseNoiseScale.CompMul(cloudTurbulenceNoiseScale * VCEdgeNoiseScale);

	param.maxGlobalCloudDensity = cloudDensityParams.x;
	param.minRescaleCloudTexDensity = cloudDensityParams.y;
	param.maxRescaleCloudTexDensity = cloudDensityParams.z;

	param.additionalNoiseIntensity = cloudMiscParams.x;
	param.shadingLOD = cloudMiscParams.y;

	param.cloudOffset = Vec3(PF.pVolCloudTilingOffset.x, PF.pVolCloudTilingOffset.y, PF.pVolCloudTilingOffset.z - param.altitude);

	int32 iFrameCount = m_tick % VCFrameCountCycle;
	param.frameCount = float(iFrameCount);
	param.jitteredSamplingPosition = VCGetVdC(iFrameCount);
}

void CVolumetricCloudManager::SetShaderParameters(CShader& shader, const VCCloudShaderParam& p, const VCCloudRenderContext& context)
{
	static CCryNameR paramScreenSize("ScreenSize");
	float fScreenWidth = (float)context.screenWidth;
	float fScreenHeight = (float)context.screenHeight;
	Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
	shader.FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

	SD3DPostEffectsUtils::UpdateFrustumCorners();
	static CCryNameR paramFrustumTL("FrustumTL");
	Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
	shader.FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
	static CCryNameR paramFrustumTR("FrustumTR");
	Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
	shader.FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
	static CCryNameR paramFrustumBL("FrustumBL");
	Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
	shader.FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

#if defined(VOLUMETRIC_FOG_SHADOWS)
	if (context.renderFogShadow)
	{
		static CCryNameR volFogShadowDarkeningN("volFogShadowDarkening");
		shader.FXSetCSFloat(volFogShadowDarkeningN, &(p.volFogShadowDarkening), 1);

		static CCryNameR volFogShadowDarkeningSunAmbN("volFogShadowDarkeningSunAmb");
		shader.FXSetCSFloat(volFogShadowDarkeningSunAmbN, &(p.volFogShadowDarkeningSunAmb), 1);
	}
#endif

	if (context.stereoReproj && context.bRightEye)
	{
		static CCryNameR vcLeftViewProjMatrixParamName("vcLeftViewProjMatrix");
		Vec4* temp = (Vec4*)m_viewProj[StereoEye::LEFT_EYE][m_tick % MaxFrameNum].GetData();
		shader.FXSetCSFloat(vcLeftViewProjMatrixParamName, temp, 4);
	}

	static CCryNameR skylightRayleighInScatterParamName("vcSkylightRayleighInScatter");
	Vec4 skylightRayleighInScatterParam(p.skylightRayleighInScatter.x, p.skylightRayleighInScatter.y, p.skylightRayleighInScatter.z, p.skyLightingFactor);
	shader.FXSetCSFloat(skylightRayleighInScatterParamName, &skylightRayleighInScatterParam, 1);

	static CCryNameR cloudSunScatteringParamName("vcSunScatteringParams");
	Vec4 cloudSunScatteringParam(p.sunSingleScatter, p.sunLowOrderScatter, p.sunLowOrderScatterAnisotropy, p.sunHighOrderScatter);
	shader.FXSetCSFloat(cloudSunScatteringParamName, &cloudSunScatteringParam, 1);

	static CCryNameR cloudGroundLightingParamName("vcGroundLightingParams");
	Vec4 cloudGroundLightingParam(p.groundAlbedo.x, p.groundAlbedo.y, p.groundAlbedo.z, p.groundLightingFactor);
	shader.FXSetCSFloat(cloudGroundLightingParamName, &cloudGroundLightingParam, 1);

	static CCryNameR cloudRenderParamName("vcCloudRenderParams");
	Vec4 cloudRenderParam(VCScatterCoefficient, p.extinction, p.powderFactor, p.shadingLOD);
	shader.FXSetCSFloat(cloudRenderParamName, &cloudRenderParam, 1);

	static CCryNameR cloudGenParamName("vcCloudGenParams");
	Vec4 cloudGenParam(p.altitude, p.thickness, p.cloudiness, 0.0f /*float(scale + 1)*/);
	shader.FXSetCSFloat(cloudGenParamName, &cloudGenParam, 1);

	static CCryNameR tilingParamName("vcCloudTilingParams");
	Vec4 tilingParam(p.vInvTiling.x, p.vInvTiling.y, p.vInvTiling.z, p.radius);
	shader.FXSetCSFloat(tilingParamName, &tilingParam, 1);

	static CCryNameR multiScatteringParamName("vcMultiScatteringParams");
	Vec4 multiScatteringParam(p.multiScatterAttenuation, p.multiScatterPreservation, 0.0f, 0.0f);
	shader.FXSetCSFloat(multiScatteringParamName, &multiScatteringParam, 1);

	static CCryNameR cloudNoiseParamName("vcCloudNoiseParams");
	Vec4 cloudNoiseParam(p.cloudNoiseScale.x, p.cloudNoiseScale.y, p.cloudNoiseScale.z, p.frameCount);
	shader.FXSetCSFloat(cloudNoiseParamName, &cloudNoiseParam, 1);

	static CCryNameR cloudBaseScaleParamName("vcCloudBaseScaleParams");
	Vec4 cloudBaseScaleParam(p.invBaseTexTiling.x, p.invBaseTexTiling.y, p.invBaseTexTiling.z, p.jitteredSamplingPosition);
	shader.FXSetCSFloat(cloudBaseScaleParamName, &cloudBaseScaleParam, 1);

	static CCryNameR cloudBaseOffsetParamName("vcCloudBaseOffsetParams");
	Vec4 cloudBaseOffsetParam(p.cloudOffset.x, p.cloudOffset.y, p.cloudOffset.z, 0.0f);
	shader.FXSetCSFloat(cloudBaseOffsetParamName, &cloudBaseOffsetParam, 1);

	static CCryNameR densityParamName("vcDensityParams");
	Vec4 densityParam(p.maxGlobalCloudDensity, p.minRescaleCloudTexDensity, p.maxRescaleCloudTexDensity, p.additionalNoiseIntensity);
	shader.FXSetCSFloat(densityParamName, &densityParam, 1);

	static CCryNameR edgeNoiseScaleParamName("vcEdgeNoiseScaleParams");
	Vec4 edgeNoiseScaleParam(p.edgeTurbulenceNoiseScale.x, p.edgeTurbulenceNoiseScale.y, p.edgeTurbulenceNoiseScale.z, 0.0f);
	shader.FXSetCSFloat(edgeNoiseScaleParamName, &edgeNoiseScaleParam, 1);

	static CCryNameR edgeTurbulenceParamName("vcEdgeTurbulenceParams");
	Vec4 edgeTurbulenceParam(p.edgeTurbulence, p.edgeThreshold, p.edgeNoiseErode, p.edgeNoiseAbs);
	shader.FXSetCSFloat(edgeTurbulenceParamName, &edgeTurbulenceParam, 1);

	static CCryNameR cloudRaymarchParamName("vcCloudRaymarchParams");
	Vec4 cloudRaymarchParam(p.maxViewableDistance, p.maxRayMarchDistance, p.maxRayMarchStep, p.horizonHeight);
	shader.FXSetCSFloat(cloudRaymarchParamName, &cloudRaymarchParam, 1);

	static CCryNameR cloudAtmosphericParamName("vcCloudAtmosphericParams");
	Vec4 cloudAtmosphericParam(p.atmosphericScattering.x, p.atmosphericScattering.y, p.atmosphericScattering.z, p.sunIntensityForAtmospheric);
	shader.FXSetCSFloat(cloudAtmosphericParamName, &cloudAtmosphericParam, 1);

	static CCryNameR vcCloudBlockerPosName("vcCloudBlockerPos");
	shader.FXSetCSFloat(vcCloudBlockerPosName, m_blockerPosArray.begin(), 4);

	static CCryNameR vcCloudBlockerParamName("vcCloudBlockerParam");
	shader.FXSetCSFloat(vcCloudBlockerParamName, m_blockerParamArray.begin(), 4);
}

bool CVolumetricCloudManager::GenerateCloudDensityAndShadow()
{
	if (CRenderer::CV_r_VolumetricCloudsPipeline != 0)
	{
		CD3D9Renderer* const __restrict rd = gcpRendD3D;
		SRenderPipeline& rp(rd->m_RP);
		CDeviceManager& dm = rd->m_DevMan;
		const int nThreadID = rp.m_nProcessThreadID;
		const SThreadInfo& ti = rp.m_TI[nThreadID];
		SCGParamsPF& PF = rd->m_cEF.m_PF[nThreadID];
		uint64 nFlagsShaderRTSave = rp.m_FlagsShader_RT;

		VCCloudRenderContext context;
		VCCloudShaderParam cloudParam;
		MakeCloudShaderParam(context, cloudParam);

		// needs to reset due to a mismatch between real device states and cached states.
		CTexture::ResetTMUs();

		const int scale = VCGetDownscaleRatio();
		const uint downscaleIndex = max(0, scale - 1);
		CTexture* scaledZTarget = (downscaleIndex == 0) ? CTexture::s_ptexZTargetScaled : CTexture::s_ptexZTargetScaled2;

		// Inject clouds density and shadow to frustum aligned volume texture.
		{
#ifdef ENABLE_DETAILED_PROFILING
			PROFILE_LABEL_SCOPE("INJECT_CLOUD_SHADOWs");
#endif

			// need to manually apply several shader flags
			rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_FOG] | g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG] |
			                         g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]
			                         | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

			if (!(rp.m_PersFlags2 & RBPF2_NOSHADERFOG) && ti.m_FS.m_bEnable)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];

				if (rd->m_bVolumetricFogEnabled)
				{
					rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
				}
			}

#if defined(VOLUMETRIC_FOG_SHADOWS)
			if (context.renderFogShadow)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
			}
#endif

			// temporarily disabled until these features are implemented.
#if 0
			//Vec3 lCol(0, 0, 0);
			//gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, lCol);
			//bool bUseLightning(lCol.x > 1e-4f || lCol.y > 1e-4f || lCol.z > 1e-4f);
			//if(bUseLightning)
			//{
			//	rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
			//}

			if (sphericalCloudLayer)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
			}
#endif

			// enable reprojection from left eye to right eye.
			if (context.stereoReproj && context.bRightEye)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
			}

			// enable user specified cloud volume texture.
			if (context.cloudVolumeTexId > 0)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
			}

			// decay cloud density with cloud blocker entity.
			if (m_blockerPosArray.Num() > 0)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
			}

			// take fast path when clouds are always rendered in low shading mode, and viewable region is covered by cloud shadow map.
			if (cloudParam.shadingLOD >= 1.0f && cloudParam.cloudShadowTilingSize >= (cloudParam.maxViewableDistance * 2.0f))
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
			}

			CShader* pShader = CShaderMan::s_ShaderClouds;
			static CCryNameTSCRC shaderName = "InjectCloudDensityAndShadow";
			bool result = GetUtils().ShBeginPass(pShader, shaderName, FEF_DONTSETSTATES);

			if (!result) //This could happen if shader is being compiled asynchronously
			{
				rp.m_FlagsShader_RT = nFlagsShaderRTSave;
				return false;
			}

			// Set UAV
			ID3D11UnorderedAccessView* pUAVs[2] = {
				(ID3D11UnorderedAccessView*)m_cloudShadowTex->GetDeviceUAV(),
				(ID3D11UnorderedAccessView*)m_cloudDensityTex->GetDeviceUAV(),
			};
			dm.BindUAV(CDeviceManager::TYPE_CS, pUAVs, NULL, 0, 2);

			scaledZTarget->Apply(0, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Compute);

			if (context.stereoReproj && context.bRightEye)
			{
				m_prevScaledDepthBuffer[StereoEye::LEFT_EYE]->Apply(2, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Compute);
			}

			// set cloud base texture.
			if (context.cloudVolumeTexId > 0)
			{
				CTexture* cloudVolumeTex = CTexture::GetByID(context.cloudVolumeTexId);
				if (cloudVolumeTex)
				{
					cloudVolumeTex->Apply(15, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Compute);
				}
			}

			SetShaderParameters(*pShader, cloudParam, context);

			// Commit shader params
			rd->FX_Commit();

			// Tile sizes defined in [numthreads()] in shader
			const uint32 nTileSizeX = 8;
			const uint32 nTileSizeY = 8;
			const uint32 nTileSizeZ = 1;

			uint32 dispatchSizeX = context.screenWidth / nTileSizeX + (context.screenWidth % nTileSizeX > 0 ? 1 : 0);
			uint32 dispatchSizeY = context.screenHeight / nTileSizeY + (context.screenHeight % nTileSizeY > 0 ? 1 : 0);
			uint32 dispatchSizeZ = 1;

			dm.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

			GetUtils().ShEndPass();
		}

		rp.m_FlagsShader_RT = nFlagsShaderRTSave;

		ID3D11UnorderedAccessView* pUAVNull[2] = { NULL };
		dm.BindUAV(CDeviceManager::TYPE_CS, pUAVNull, NULL, 0, 2);

		CTexture::ResetTMUs();

		rd->FX_Commit();
		dm.CommitDeviceStates();
	}

	return true;
}

void CVolumetricCloudManager::RenderClouds()
{
	if (!IsRenderable())
		return;
	if (!AreTexturesValid())
		return;

	PROFILE_LABEL_SCOPE("VOLUMETRIC_CLOUDS");

	GenerateCloudDensityAndShadow();

	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	CDeviceManager& dm = rd->m_DevMan;
	const int nThreadID = rp.m_nProcessThreadID;
	const SThreadInfo& ti = rp.m_TI[nThreadID];
	SCGParamsPF& PF = rd->m_cEF.m_PF[nThreadID];

	Vec3 skylightRayleighInScatter;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKYLIGHT_RAYLEIGH_INSCATTER, skylightRayleighInScatter);

	Vec3 cloudGenParams;
	Vec3 cloudScatteringLowParams;
	Vec3 cloudScatteringHighParams;
	Vec3 cloudGroundColorParams;
	Vec3 cloudScatteringMulti;
	Vec3 cloudWindAndAtmosphericParams;
	Vec3 cloudTurbulenceParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GEN_PARAMS, cloudGenParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_LOW, cloudScatteringLowParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_HIGH, cloudScatteringHighParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GROUND_COLOR, cloudGroundColorParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_SCATTERING_MULTI, cloudScatteringMulti);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_WIND_ATMOSPHERIC, cloudWindAndAtmosphericParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE, cloudTurbulenceParams);

	Vec3 cloudEnvParams;
	Vec3 cloudGlobalNoiseScale;
	Vec3 cloudRenderParams;
	Vec3 cloudTurbulenceNoiseScale;
	Vec3 cloudTurbulenceNoiseParams;
	Vec3 cloudDensityParams;
	Vec3 cloudMiscParams;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_ENV_PARAMS, cloudEnvParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_GLOBAL_NOISE_SCALE, cloudGlobalNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_RENDER_PARAMS, cloudRenderParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_SCALE, cloudTurbulenceNoiseScale);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_TURBULENCE_NOISE_PARAMS, cloudTurbulenceNoiseParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_DENSITY_PARAMS, cloudDensityParams);
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_MISC_PARAM, cloudMiscParams);

	Vec3 atmosphericScattering;
	gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLCLOUD_ATMOSPHERIC_SCATTERING, atmosphericScattering);

	const bool sphericalCloudLayer = (cloudEnvParams.y > 0.0f);

	// Save previous states
	int vX, vY, vWidth, vHeight;
	rd->GetViewport(&vX, &vY, &vWidth, &vHeight);
	uint64 nFlagsShaderRTSave = rp.m_FlagsShader_RT;

	bool valid = true;
	CShader* pShader = CShaderMan::s_ShaderClouds;
	const int scale = VCGetDownscaleRatio();
	const uint downscaleIndex = max(0, scale - 1);
	bool bStereo = gRenDev->m_bDualStereoSupport;
	bool stereoReproj = bStereo && CRenderer::CV_r_VolumetricCloudsStereoReprojection != 0;
	bool bRightEye = gRenDev->m_CurRenderEye == StereoEye::RIGHT_EYE;

	CTexture* scaledTarget = CTexture::s_ptexHDRTargetScaledTmp[downscaleIndex]; // half or quarter resolution
	if (stereoReproj && gRenDev->m_CurRenderEye == StereoEye::LEFT_EYE)
	{
		scaledTarget = m_downscaledLeftEye; // left eye is re-projected to right eye.
	}

	CTexture* scaledZTarget = (downscaleIndex == 0) ? CTexture::s_ptexZTargetScaled : CTexture::s_ptexZTargetScaled2;

	{
		rd->FX_PushRenderTarget(0, scaledTarget, NULL);
		rd->FX_SetActiveRenderTargets();

		int targetWidth = scaledTarget->GetWidth();
		int targetHeight = scaledTarget->GetHeight();

		rd->RT_SetViewport(0, 0, targetWidth, targetHeight);
	}

	// render cloud
	{
#ifdef ENABLE_DETAILED_PROFILING
		PROFILE_LABEL_SCOPE("RENDER_CLOUDS");
#endif

		// need to manually apply several shader flags
		rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_FOG] | g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG] |
		                         g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]
		                         | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

		if (!(rp.m_PersFlags2 & RBPF2_NOSHADERFOG) && ti.m_FS.m_bEnable)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];

			if (rd->m_bVolumetricFogEnabled)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
			}
		}

#if defined(VOLUMETRIC_FOG_SHADOWS)
		static CCryNameR volFogShadowDarkeningN("volFogShadowDarkening");
		static CCryNameR volFogShadowDarkeningSunAmbN("volFogShadowDarkeningSunAmb");

		Vec4 volFogShadowDarkening(0, 0, 0, 0);
		Vec4 volFogShadowDarkeningSunAmb(0, 0, 0, 0);

		const bool renderFogShadow = rd->m_bVolFogShadowsEnabled;

		if (renderFogShadow)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

			Vec3 volFogShadowDarkeningP;
			gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);
			volFogShadowDarkening = Vec4(volFogShadowDarkeningP, 0);

			const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
			const float bSun = 1.0f - aSun;
			const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
			const float bAmb = 1.0f - aAmb;
			volFogShadowDarkeningSunAmb = Vec4(aSun, bSun, aAmb, bAmb);
		}
#endif

		// temporarily disabled until these features are implemented.
#if 0
		Vec3 lCol(0, 0, 0);
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, lCol);
		bool bUseLightning(lCol.x > 1e-4f || lCol.y > 1e-4f || lCol.z > 1e-4f);
		if (bUseLightning)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		}

		if (sphericalCloudLayer)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
		}
#endif

		// enable reprojection from left eye to right eye.
		if (stereoReproj && bRightEye)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		}

		const int texId = rd->GetVolumetricCloudTextureId();
		CTexture* cloudVolumeTex = NULL;
		if (texId > 0)
		{
			cloudVolumeTex = CTexture::GetByID(texId);
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
		}

		// decay cloud density with cloud blocker entity.
		if (m_blockerPosArray.Num() > 0)
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
		}

		// take fast path when clouds are always rendered in low shading mode, and viewable region is covered by cloud shadow map.
		const float maxViewableDistance = cloudRenderParams.x;
		const float shadingLOD = cloudMiscParams.y;
		const float cloudShadowTilingSize = cloudMiscParams.z;
		if (shadingLOD >= 1.0f && cloudShadowTilingSize >= (maxViewableDistance * 2.0f))
		{
			rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
		}

		static CCryNameTSCRC shaderName = "CloudsLayer";
		static CCryNameTSCRC shaderName2 = "RenderCloud";
		if (CRenderer::CV_r_VolumetricCloudsPipeline != 0)
		{
			pShader->FXSetTechnique(shaderName2);
		}
		else
		{
			pShader->FXSetTechnique(shaderName);
		}

		uint32 nPasses;
		pShader->FXBegin(&nPasses, FEF_DONTSETSTATES);
		valid &= pShader->FXBeginPass(0);

		{
			scaledZTarget->Apply(0, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);

			// Set UAV
			ID3D11UnorderedAccessView* pUAVs[2] = {
				(ID3D11UnorderedAccessView*)m_cloudDepthBuffer->GetDeviceUAV(),
				(ID3D11UnorderedAccessView*)m_downscaledTemp->GetDeviceUAV(),
			};
			dm.BindUAV(CDeviceManager::TYPE_PS, pUAVs, NULL, 1, 2);
		}

		if (stereoReproj && bRightEye)
		{
			m_downscaledLeftEye->Apply(1, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Pixel);
			if (CRenderer::CV_r_VolumetricCloudsPipeline == 0)
			{
				m_prevScaledDepthBuffer[StereoEye::LEFT_EYE]->Apply(2, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Pixel);
			}
		}

		// set cloud base texture.
		if (cloudVolumeTex)
		{
			cloudVolumeTex->Apply(15, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Pixel);
		}

#if defined(VOLUMETRIC_FOG_SHADOWS)
		if (renderFogShadow)
		{
			pShader->FXSetPSFloat(volFogShadowDarkeningN, &volFogShadowDarkening, 1);
			pShader->FXSetPSFloat(volFogShadowDarkeningSunAmbN, &volFogShadowDarkeningSunAmb, 1);
		}
#endif

		if (CRenderer::CV_r_VolumetricCloudsPipeline != 0)
		{
			m_cloudDensityTex->Apply(3, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Pixel);
			m_cloudShadowTex->Apply(4, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView, eHWSC_Pixel);
		}

		const float cloudiness = cloudGenParams.x;
		const float altitude = cloudGenParams.y;
		const float thickness = max(1e-3f, cloudGenParams.z);
		const float sunSingleScatter = cloudScatteringLowParams.x;
		const float sunLowOrderScatter = cloudScatteringLowParams.y;
		const float sunLowOrderScatterAnisotropy = cloudScatteringLowParams.z;
		const float sunHighOrderScatter = cloudScatteringHighParams.x;
		const float skyLightingFactor = cloudScatteringHighParams.y;
		const float groundLightingFactor = cloudScatteringHighParams.z;
		const Vec3& groundAlbedo = cloudGroundColorParams;
		const float multiScatterAttenuation = cloudScatteringMulti.x;
		const float multiScatterPreservation = cloudScatteringMulti.y;
		const float powderFactor = cloudScatteringMulti.z;
		const float sunIntensityForAtmospheric = cloudWindAndAtmosphericParams.z;
		const float edgeNoiseErode = cloudTurbulenceNoiseParams.x;
		const float edgeNoiseAbs = cloudTurbulenceNoiseParams.y;
		const float edgeTurbulence = (edgeNoiseErode > 0.0f) ? 2.0f * cloudTurbulenceParams.x : 8.0f * cloudTurbulenceParams.x;
		const float edgeThreshold = cloudTurbulenceParams.y + 1e-4f;
		const float absorptionFactor = cloudTurbulenceParams.z; // 1 to 5 percent is absorbed in clouds at most.

		const float radius = clamp_tpl<float>(cloudEnvParams.x, VCMinSphereRadius, VCMaxSphereRadius);
		const float horizonHeight = cloudEnvParams.z;
		const Vec3 cloudNoiseScale = VCBaseNoiseScale.CompMul(cloudGlobalNoiseScale);
		const float maxRayMarchDistance = cloudRenderParams.y;
		const float maxRayMarchStep = float(VCGetCloudRaymarchStepNum());

		float extinction = VCScatterCoefficient + (VCScatterCoefficient * absorptionFactor);

		const Vec3 vTiling(cloudShadowTilingSize, cloudShadowTilingSize, thickness);
		const Vec3 vInvTiling(1.0f / vTiling.x, 1.0f / vTiling.y, 1.0f / vTiling.z);

		const Vec3& baseTexTiling = PF.pVolCloudTilingSize;
		const Vec3 invBaseTexTiling(1.0f / baseTexTiling.x, 1.0f / baseTexTiling.y, 1.0f / baseTexTiling.z);

		const Vec3 edgeTurbulenceNoiseScale = VCBaseNoiseScale.CompMul(cloudTurbulenceNoiseScale * VCEdgeNoiseScale);

		const float maxGlobalCloudDensity = cloudDensityParams.x;
		const float minRescaleCloudTexDensity = cloudDensityParams.y;
		const float maxRescaleCloudTexDensity = cloudDensityParams.z;

		const float additionalNoiseIntensity = cloudMiscParams.x;

		const Vec3 cloudOffset(PF.pVolCloudTilingOffset.x, PF.pVolCloudTilingOffset.y, PF.pVolCloudTilingOffset.z - altitude);

		int32 iFrameCount = m_tick % VCFrameCountCycle;
		float frameCount = float(iFrameCount);
		float jitteredSamplingPosition = VCGetVdC(iFrameCount);

		static CCryNameR skylightRayleighInScatterParamName("vcSkylightRayleighInScatter");
		Vec4 skylightRayleighInScatterParam(skylightRayleighInScatter.x, skylightRayleighInScatter.y, skylightRayleighInScatter.z, skyLightingFactor);
		pShader->FXSetPSFloat(skylightRayleighInScatterParamName, &skylightRayleighInScatterParam, 1);

		static CCryNameR cloudSunScatteringParamName("vcSunScatteringParams");
		Vec4 cloudSunScatteringParam(sunSingleScatter, sunLowOrderScatter, sunLowOrderScatterAnisotropy, sunHighOrderScatter);
		pShader->FXSetPSFloat(cloudSunScatteringParamName, &cloudSunScatteringParam, 1);

		static CCryNameR cloudGroundLightingParamName("vcGroundLightingParams");
		Vec4 cloudGroundLightingParam(groundAlbedo.x, groundAlbedo.y, groundAlbedo.z, groundLightingFactor);
		pShader->FXSetPSFloat(cloudGroundLightingParamName, &cloudGroundLightingParam, 1);

		static CCryNameR cloudRenderParamName("vcCloudRenderParams");
		Vec4 cloudRenderParam(VCScatterCoefficient, extinction, powderFactor, shadingLOD);
		pShader->FXSetPSFloat(cloudRenderParamName, &cloudRenderParam, 1);

		static CCryNameR cloudGenParamName("vcCloudGenParams");
		Vec4 cloudGenParam(altitude, thickness, cloudiness, float(scale + 1));
		pShader->FXSetPSFloat(cloudGenParamName, &cloudGenParam, 1);

		static CCryNameR tilingParamName("vcCloudTilingParams");
		Vec4 tilingParam(vInvTiling.x, vInvTiling.y, vInvTiling.z, radius);
		pShader->FXSetPSFloat(tilingParamName, &tilingParam, 1);

		static CCryNameR multiScatteringParamName("vcMultiScatteringParams");
		Vec4 multiScatteringParam(multiScatterAttenuation, multiScatterPreservation, 0.0f, 0.0f);
		pShader->FXSetPSFloat(multiScatteringParamName, &multiScatteringParam, 1);

		static CCryNameR cloudNoiseParamName("vcCloudNoiseParams");
		Vec4 cloudNoiseParam(cloudNoiseScale.x, cloudNoiseScale.y, cloudNoiseScale.z, frameCount);
		pShader->FXSetPSFloat(cloudNoiseParamName, &cloudNoiseParam, 1);

		static CCryNameR cloudBaseScaleParamName("vcCloudBaseScaleParams");
		Vec4 cloudBaseScaleParam(invBaseTexTiling.x, invBaseTexTiling.y, invBaseTexTiling.z, jitteredSamplingPosition);
		pShader->FXSetPSFloat(cloudBaseScaleParamName, &cloudBaseScaleParam, 1);

		static CCryNameR cloudBaseOffsetParamName("vcCloudBaseOffsetParams");
		Vec4 cloudBaseOffsetParam(cloudOffset.x, cloudOffset.y, cloudOffset.z, 0);
		pShader->FXSetPSFloat(cloudBaseOffsetParamName, &cloudBaseOffsetParam, 1);

		static CCryNameR densityParamName("vcDensityParams");
		Vec4 densityParam(maxGlobalCloudDensity, minRescaleCloudTexDensity, maxRescaleCloudTexDensity, additionalNoiseIntensity);
		pShader->FXSetPSFloat(densityParamName, &densityParam, 1);

		static CCryNameR edgeNoiseScaleParamName("vcEdgeNoiseScaleParams");
		Vec4 edgeNoiseScaleParam(edgeTurbulenceNoiseScale.x, edgeTurbulenceNoiseScale.y, edgeTurbulenceNoiseScale.z, 0);
		pShader->FXSetPSFloat(edgeNoiseScaleParamName, &edgeNoiseScaleParam, 1);

		static CCryNameR edgeTurbulenceParamName("vcEdgeTurbulenceParams");
		Vec4 edgeTurbulenceParam(edgeTurbulence, edgeThreshold, edgeNoiseErode, edgeNoiseAbs);
		pShader->FXSetPSFloat(edgeTurbulenceParamName, &edgeTurbulenceParam, 1);

		static CCryNameR cloudRaymarchParamName("vcCloudRaymarchParams");
		Vec4 cloudRaymarchParam(maxViewableDistance, maxRayMarchDistance, maxRayMarchStep, horizonHeight);
		pShader->FXSetPSFloat(cloudRaymarchParamName, &cloudRaymarchParam, 1);

		static CCryNameR cloudAtmosphericParamName("vcCloudAtmosphericParams");
		Vec4 cloudAtmosphericParam(atmosphericScattering.x, atmosphericScattering.y, atmosphericScattering.z, sunIntensityForAtmospheric);
		pShader->FXSetPSFloat(cloudAtmosphericParamName, &cloudAtmosphericParam, 1);

		static CCryNameR vcCloudBlockerPosName("vcCloudBlockerPos");
		pShader->FXSetPSFloat(vcCloudBlockerPosName, m_blockerPosArray.begin(), 4);

		static CCryNameR vcCloudBlockerParamName("vcCloudBlockerParam");
		pShader->FXSetPSFloat(vcCloudBlockerParamName, m_blockerParamArray.begin(), 4);

		if (stereoReproj && bRightEye)
		{
			static CCryNameR vcLeftViewProjMatrixParamName("vcLeftViewProjMatrix");
			Vec4* temp = (Vec4*)m_viewProj[StereoEye::LEFT_EYE][m_tick % MaxFrameNum].GetData();
			pShader->FXSetPSFloat(vcLeftViewProjMatrixParamName, temp, 4);
		}

		rd->FX_SetState(GS_NODEPTHTEST);

		rd->FX_Commit();

		PostProcessUtils().DrawFullScreenTriWPOS(CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight(), 1.0f);

		pShader->FXEndPass();

		rp.m_FlagsShader_RT = nFlagsShaderRTSave;
	}

	{
		ID3D11UnorderedAccessView* pUAVNull[2] = { NULL };
		dm.BindUAV(CDeviceManager::TYPE_PS, pUAVNull, NULL, 1, 2);
		rd->FX_Commit();
		dm.CommitDeviceStates();

		int32 curEye = gRenDev->m_CurRenderEye;
		int32 tick = (m_tick / gcpRendD3D->GetActiveGPUCount()) & 0x1;
		CTexture* prevTex = tick ? m_downscaledTex[curEye][0] : m_downscaledTex[curEye][1];
		CTexture* currTex = tick ? m_downscaledTex[curEye][1] : m_downscaledTex[curEye][0];
		CTexture* prevMinTex = tick ? m_downscaledMinTex[curEye][0] : m_downscaledMinTex[curEye][1];
		CTexture* currMinTex = tick ? m_downscaledMinTex[curEye][1] : m_downscaledMinTex[curEye][0];
		CTexture* prevScaledDepth = m_prevScaledDepthBuffer[curEye];

		if (m_cleared > 0)
		{
			rd->GetDeviceContext().CopyResource(prevTex->GetDevTexture()->Get2DTexture(), currTex->GetDevTexture()->Get2DTexture());
			rd->GetDeviceContext().CopyResource(prevMinTex->GetDevTexture()->Get2DTexture(), currMinTex->GetDevTexture()->Get2DTexture());
			rd->GetDeviceContext().CopyResource(prevScaledDepth->GetDevTexture()->Get2DTexture(), scaledZTarget->GetDevTexture()->Get2DTexture());
		}

		// apply temporal re-projection.
		{
#ifdef ENABLE_DETAILED_PROFILING
			PROFILE_LABEL_SCOPE("REPROJECTION_CLOUDS_0");
#endif

			rd->FX_PopRenderTarget(0);
			rd->FX_PushRenderTarget(0, currTex, NULL);
			rd->FX_SetActiveRenderTargets();

			int targetWidth = currTex->GetWidth();
			int targetHeight = currTex->GetHeight();
			rd->RT_SetViewport(0, 0, targetWidth, targetHeight);

			rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1]);

			// activates using max-depth.
			rp.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SAMPLE0]);

			// enables less flicker temporal reprojection filter.
			if (CRenderer::CV_r_VolumetricCloudsTemporalReprojection == 1)
			{
				rp.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SAMPLE1]);
			}

			static CCryNameTSCRC shaderName = "ReprojectClouds";
			pShader->FXSetTechnique(shaderName);

			uint32 nPasses;
			pShader->FXBegin(&nPasses, FEF_DONTSETSTATES);
			valid &= pShader->FXBeginPass(0);

			scaledZTarget->Apply(0, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			prevScaledDepth->Apply(1, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			scaledTarget->Apply(2, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			prevTex->Apply(3, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			m_cloudDepthBuffer->Apply(4, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);

			static CCryNameR paramPrevMatrix("PrevViewProjMatrix");
			Vec4* temp = (Vec4*)m_viewProj[curEye][max(m_tick - (int32)rd->GetActiveGPUCount(), 0) % MaxFrameNum].GetData();
			pShader->FXSetPSFloat(paramPrevMatrix, temp, 4);

			rd->FX_SetState(GS_NODEPTHTEST);
			rd->FX_Commit();

			PostProcessUtils().DrawFullScreenTriWPOS(targetWidth, targetHeight, 1.0f);

			pShader->FXEndPass();
		}
		{
#ifdef ENABLE_DETAILED_PROFILING
			PROFILE_LABEL_SCOPE("REPROJECTION_CLOUDS_1");
#endif

			rd->FX_PopRenderTarget(0);
			rd->FX_PushRenderTarget(0, currMinTex, NULL);
			rd->FX_SetActiveRenderTargets();

			int targetWidth = currMinTex->GetWidth();
			int targetHeight = currMinTex->GetHeight();
			rd->RT_SetViewport(0, 0, targetWidth, targetHeight);

			rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1]);

			// enables less flicker temporal reprojection filter.
			if (CRenderer::CV_r_VolumetricCloudsTemporalReprojection == 1)
			{
				rp.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SAMPLE1]);
			}

			static CCryNameTSCRC shaderName = "ReprojectClouds";
			pShader->FXSetTechnique(shaderName);

			uint32 nPasses;
			pShader->FXBegin(&nPasses, FEF_DONTSETSTATES);
			valid &= pShader->FXBeginPass(0);

			scaledZTarget->Apply(0, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			prevScaledDepth->Apply(1, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			m_downscaledTemp->Apply(2, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			prevMinTex->Apply(3, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			m_cloudDepthBuffer->Apply(4, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);

			static CCryNameR paramPrevMatrix("PrevViewProjMatrix");
			Vec4* temp = (Vec4*)m_viewProj[curEye][max(m_tick - (int32)rd->GetActiveGPUCount(), 0) % MaxFrameNum].GetData();
			pShader->FXSetPSFloat(paramPrevMatrix, temp, 4);

			rd->FX_SetState(GS_NODEPTHTEST);
			rd->FX_Commit();

			PostProcessUtils().DrawFullScreenTriWPOS(targetWidth, targetHeight, 1.0f);

			pShader->FXEndPass();
		}

		// copy current downscaled depth for next frame's temporal re-projection
		rd->GetDeviceContext().CopyResource(prevScaledDepth->GetDevTexture()->Get2DTexture(), scaledZTarget->GetDevTexture()->Get2DTexture());

		if (valid)
		{
			m_cleared = (m_cleared > 0) ? (m_cleared - 1) : 0;
		}

		// restore render target.
		rd->FX_PopRenderTarget(0);
		rd->RT_SetViewport(vX, vY, vWidth, vHeight);
		rd->FX_SetActiveRenderTargets();

		// upscale and blend it with scene.
		{
#ifdef ENABLE_DETAILED_PROFILING
			PROFILE_LABEL_SCOPE("UPSCALE_CLOUDS");
#endif

			// need to manually apply several shader flags
			rp.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_FOG] | g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG] |
			                         g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4]);

			if (!(rp.m_PersFlags2 & RBPF2_NOSHADERFOG) && ti.m_FS.m_bEnable)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];

				if (rd->m_bVolumetricFogEnabled)
				{
					rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
				}
			}

#if defined(VOLUMETRIC_FOG_SHADOWS)
			static CCryNameR volFogShadowDarkeningN("volFogShadowDarkening");
			static CCryNameR volFogShadowDarkeningSunAmbN("volFogShadowDarkeningSunAmb");

			Vec4 volFogShadowDarkening(0, 0, 0, 0);
			Vec4 volFogShadowDarkeningSunAmb(0, 0, 0, 0);

			const bool renderFogShadow = rd->m_bVolFogShadowsEnabled;

			if (renderFogShadow)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

				Vec3 volFogShadowDarkeningP;
				gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);
				volFogShadowDarkening = Vec4(volFogShadowDarkeningP, 0);

				const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
				const float bSun = 1.0f - aSun;
				const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
				const float bAmb = 1.0f - aAmb;
				volFogShadowDarkeningSunAmb = Vec4(aSun, bSun, aAmb, bAmb);
			}
#endif

			// decay cloud density with cloud blocker entity on screen space.
			if (m_blockerSSPosArray.Num() > 0)
			{
				rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
			}

			static CCryNameTSCRC shaderName = "CloudsBlend";
			pShader->FXSetTechnique(shaderName);

			uint32 nPasses;
			pShader->FXBegin(&nPasses, FEF_DONTSETSTATES);
			pShader->FXBeginPass(0);

			CTexture::s_ptexZTarget->Apply(0, -1, EFTT_UNKNOWN, -2, rp.m_MSAAData.Type ? SResourceView::DefaultViewMS : SResourceView::DefaultView); // bind as msaa target (if valid)
			scaledZTarget->Apply(1, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			currTex->Apply(2, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			m_cloudDepthBuffer->Apply(3, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
			currMinTex->Apply(4, -1, EFTT_UNKNOWN, -2, SResourceView::DefaultView);

			const static int nTStatePointClamp = CTexture::GetTexState(STexState(FILTER_POINT, true));
			CTexture::SetSamplerState(nTStatePointClamp, 0, eHWSC_Pixel);

#if defined(VOLUMETRIC_FOG_SHADOWS)
			if (renderFogShadow)
			{
				pShader->FXSetPSFloat(volFogShadowDarkeningN, &volFogShadowDarkening, 1);
				pShader->FXSetPSFloat(volFogShadowDarkeningSunAmbN, &volFogShadowDarkeningSunAmb, 1);
			}
#endif

			float scaledWidth = (float)currTex->GetWidth();
			float scaledHeight = (float)currTex->GetHeight();
			static CCryNameR scaledScreenSizeParamName("ScaledScreenSize");
			Vec4 scaledScreenSizeParam(scaledWidth, scaledHeight, 1.0f / scaledWidth, 1.0f / scaledHeight);
			pShader->FXSetPSFloat(scaledScreenSizeParamName, &scaledScreenSizeParam, 1);

			static CCryNameR cloudBlockerPosName("CloudBlockerSSPos");
			pShader->FXSetPSFloat(cloudBlockerPosName, m_blockerSSPosArray.begin(), 1);

			static CCryNameR cloudBlockerParamName("CloudBlockerSSParam");
			pShader->FXSetPSFloat(cloudBlockerParamName, m_blockerSSParamArray.begin(), 1);

			// using GS_BLDST_SRCALPHA because GS_BLDST_ONEMINUSSRCALPHA causes banding artifact when alpha value is very low.
			rd->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_SRCALPHA);
			rd->FX_Commit();

			PostProcessUtils().DrawFullScreenTriWPOS(CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight(), 1.0f);

			pShader->FXEndPass();
		}
	}

	rp.m_FlagsShader_RT = nFlagsShaderRTSave;
}
