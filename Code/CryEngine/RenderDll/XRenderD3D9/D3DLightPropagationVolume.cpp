// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id: D3DLightPropagationVolume.cpp,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
   $DateTime$
   Description:  Routine for rendering and managing of Light Propagation Volumes
   -------------------------------------------------------------------------
   History:
   - 19:5:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#include "StdAfx.h"

#include <CryCore/CryCustomTypes.h>
#include <Cry3DEngine/I3DEngine.h>
#include <CryEntitySystem/IEntityRenderState.h>
#include "DriverD3D.h"
#include "D3DDeferredShading.h"
#include "D3DLightPropagationVolume.h"
#include "D3DPostProcess.h"
#include <CryRenderer/RenderElements/CRELightPropagationVolume.h>
#include "D3DPostProcess.h"

#ifdef _DEBUG
	#undef CHK
	#define CHK(x) assert(x)
#else
	#define CHK(x) x
#endif

// System shader name: "LightPropagationVolumes.cfx"
#define LPV_SHADER_NAME "LightPropagationVolumes"

// apply pass flags
#define RT_APPLY_SPECULAR     g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_STICKED_TO_CAMERA  g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_VISAREA_CLIP_ID    g_HWSR_MaskBit[HWSR_SAMPLE2]
#define RT_USE_OCCLUSION      g_HWSR_MaskBit[HWSR_SAMPLE3]
#define RT_USE_VOLUME_TEXTURE g_HWSR_MaskBit[HWSR_SAMPLE4]

// postinjection pass flags
#define RT_NEGATIVE_LIGHT g_HWSR_MaskBit[HWSR_SAMPLE0]

// debug apply flags
#define RT_DEBUG_VISUALIZATION g_HWSR_MaskBit[HWSR_DEBUG1]

// define platform-independent RT formats
#define RSM_FLUX_FMT    eTF_R8G8B8A8
#define RSM_NORMALS_FMT eTF_R8G8B8A8

// define platform-dependent RT formats
// PC: Use ARGB16F format in order to preserve HDR range
#define LPV_GI_FMT          eTF_R16G16B16A16F
#define LPV_HDR_VOLUME_FMT  eTF_R16G16B16A16F
#define LPV_OCCL_VOLUME_FMT eTF_R16G16B16A16F
#define RSM_DEPTH_FMT       eTF_R32F

#define m_RenderSettings    m_Settings[rd->m_RP.m_nProcessThreadID]

CLightPropagationVolumesManager* CLightPropagationVolumesManager::s_pInstance = NULL;

CLightPropagationVolumesManager::CLightPropagationVolumesManager() : m_enabled(false)
{
	m_pPropagateVB = NULL;
	m_pPostinjectVB = NULL;
	m_pApplyVB = NULL;
	m_VTFVertexDeclaration = NULL;
	m_pRSMInjPointsVB = NULL;

	m_pCurrentGIVolume = NULL;
	m_pTempVolRTs[0] = m_pTempVolRTs[1] = m_pTempVolRTs[2] = NULL;

	m_nNextFreeId = -1;

	// shader tech names
	m_techPropagateTechName = "LPVPropagate";
	m_techCollectTechName = "LPVCollect";
	m_techApplyTechName = "LPVApply";
	m_techInjectRSM = "LPVInjectRSM";
	m_techPostinjectLight = "LPVPostinjectLight";

	// shader constants' names
	m_semLightPositionSemantic = "g_lightPosition";
	m_semLightColorSemantic = "g_lightColor";
	m_semCameraMatrix = "g_mCamera";
	m_semCameraFrustrumLB = "g_vFrustrumLB";
	m_semCameraFrustrumLT = "g_vFrustrumLT";
	m_semCameraFrustrumRB = "g_vFrustrumRB";
	m_semCameraFrustrumRT = "g_vFrustrumRT";
	m_semVisAreaParams = "g_vVisAreasParams";
}

CLightPropagationVolumesManager::~CLightPropagationVolumesManager()
{
	Cleanup();

	s_pInstance = NULL;

	// destroy VBs
	SAFE_DELETE(m_pPropagateVB);
	SAFE_DELETE(m_pPostinjectVB);
	SAFE_DELETE(m_pApplyVB);
	SAFE_DELETE(m_pRSMInjPointsVB);

	// destroying vd
	SAFE_RELEASE(m_VTFVertexDeclaration);
}

bool CLightPropagationVolumesManager::IsEnabled() const
{
	static ICVar* pGI = iConsole->GetCVar("e_gi");
	return m_enabled && pGI->GetIVal() != 0;
}

bool CLightPropagationVolumesManager::IsRenderable()
{
	if (!m_grids.empty())
		Toggle(CRenderer::CV_r_LightPropagationVolumes != 0);
	else
		Toggle(false);

	// Check if there any LPV which needs to be rendered
	bool bIsRenderable = false;
	LPVSet::const_iterator itEnd = m_grids.end();
	for (LPVSet::iterator it = m_grids.begin(); it != itEnd; ++it)
	{
		if ((*it)->IsRenderable())
		{
			bIsRenderable = true;
			break;
		}
	}

	return bIsRenderable;
}

void CLightPropagationVolumesManager::RenderGI()
{
	if (!m_enabled || m_setGIVolumes.empty())
		return;

	PROFILE_LABEL_SCOPE("GI_RENDER");

	// render GI volume
	LPVSet::const_iterator itEnd = m_setGIVolumes.end();
	for (LPVSet::iterator it = m_setGIVolumes.begin(); it != itEnd; ++it)
		(*it)->DeferredApply();
}

void CLightPropagationVolumesManager::Render()
{
	if (!m_enabled || m_grids.empty())
		return;

	PROFILE_LABEL_SCOPE("LPV_RENDER");

	// render all LPVs with light sources
	LPVSet::const_iterator itEnd = m_grids.end();
	for (LPVSet::iterator it = m_grids.begin(); it != itEnd; ++it)
	{
		// skip rendering of GI volume
		if ((*it)->GetFlags() & CRELightPropagationVolume::efGIVolume)
			continue;

		(*it)->DeferredApply();
	}
}

void CLightPropagationVolumesManager::Toggle(const bool enable)
{
	if (m_enabled == enable)
		return;

	Cleanup();

	m_enabled = enable;

	// Re-initialize all render resources: VBs and RT pools
	if (enable)
	{
		CD3D9Renderer* const __restrict rd = gcpRendD3D;

		// create VB for propagation
		if (!m_pPropagateVB)
		{
			std::vector<SVF_P3F_T3F> vecPropagateVBPointer;
			vecPropagateVBPointer.resize(CRELightPropagationVolume::nMaxGridSize * 6);
			for (uint32 iQuad = 0; iQuad < CRELightPropagationVolume::nMaxGridSize; ++iQuad)
			{
				vecPropagateVBPointer[iQuad * 6 + 0].p = Vec3(1, 1, (float)iQuad);
				vecPropagateVBPointer[iQuad * 6 + 1].p = Vec3(1, 0, (float)iQuad);
				vecPropagateVBPointer[iQuad * 6 + 2].p = Vec3(0, 1, (float)iQuad);

				vecPropagateVBPointer[iQuad * 6 + 3].p = Vec3(0, 1, (float)iQuad);
				vecPropagateVBPointer[iQuad * 6 + 4].p = Vec3(1, 0, (float)iQuad);
				vecPropagateVBPointer[iQuad * 6 + 5].p = Vec3(0, 0, (float)iQuad);
			}
			m_pPropagateVB = rd->m_DevBufMan.CreateVBuffer(vecPropagateVBPointer.size(), eVF_P3F_T3F, "LPVPropagate");
			rd->m_DevBufMan.UpdateVBuffer(m_pPropagateVB, &vecPropagateVBPointer.front(), vecPropagateVBPointer.size());
		}

		// create VB for post injected lights
		if (!m_pPostinjectVB)
		{
			std::vector<SVF_P3F_T3F> vecPostinjectVBPointer;
			vecPostinjectVBPointer.resize(CRELightPropagationVolume::nMaxGridSize * 6);
			for (int iQuad = 0; iQuad < CRELightPropagationVolume::nMaxGridSize; ++iQuad)
			{
				vecPostinjectVBPointer[iQuad * 6 + 0].p = Vec3(1, 1, (float)iQuad);
				vecPostinjectVBPointer[iQuad * 6 + 1].p = Vec3(1, -1, (float)iQuad);
				vecPostinjectVBPointer[iQuad * 6 + 2].p = Vec3(-1, 1, (float)iQuad);

				vecPostinjectVBPointer[iQuad * 6 + 3].p = Vec3(1, -1, (float)iQuad);
				vecPostinjectVBPointer[iQuad * 6 + 4].p = Vec3(-1, 1, (float)iQuad);
				vecPostinjectVBPointer[iQuad * 6 + 5].p = Vec3(-1, -1, (float)iQuad);
			}
			m_pPostinjectVB = rd->m_DevBufMan.CreateVBuffer(vecPostinjectVBPointer.size(), eVF_P3F_T3F, "LPVPostinject");
			rd->m_DevBufMan.UpdateVBuffer(m_pPostinjectVB, &vecPostinjectVBPointer.front(), vecPostinjectVBPointer.size());
		}

		// create VB for deferred apply(unit cube)
		if (!m_pApplyVB)
		{
			const SVF_P3F_T3F vbApplyCube[] = {
				// front & back
				{ Vec3(1, 0, 0), Vec3(0, 0, 0) }, { Vec3(0, 0, 0), Vec3(0, 0, 0) }, { Vec3(0, 0, 1), Vec3(0, 0, 0) },
				{ Vec3(0, 0, 1), Vec3(0, 0, 0) }, { Vec3(1, 0, 1), Vec3(0, 0, 0) }, { Vec3(1, 0, 0), Vec3(0, 0, 0) },
				{ Vec3(1, 1, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 1), Vec3(0, 0, 0) }, { Vec3(0, 1, 0), Vec3(0, 0, 0) },
				{ Vec3(1, 1, 0), Vec3(0, 0, 0) }, { Vec3(1, 1, 1), Vec3(0, 0, 0) }, { Vec3(0, 1, 1), Vec3(0, 0, 0) },
				// left & right
				{ Vec3(0, 0, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 1), Vec3(0, 0, 0) }, { Vec3(0, 0, 1), Vec3(0, 0, 0) },
				{ Vec3(0, 0, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 1), Vec3(0, 0, 0) },
				{ Vec3(1, 0, 0), Vec3(0, 0, 0) }, { Vec3(1, 0, 1), Vec3(0, 0, 0) }, { Vec3(1, 1, 1), Vec3(0, 0, 0) },
				{ Vec3(1, 0, 0), Vec3(0, 0, 0) }, { Vec3(1, 1, 1), Vec3(0, 0, 0) }, { Vec3(1, 1, 0), Vec3(0, 0, 0) },
				// top & bottom
				{ Vec3(1, 0, 1), Vec3(0, 0, 0) }, { Vec3(0, 0, 1), Vec3(0, 0, 0) }, { Vec3(0, 1, 1), Vec3(0, 0, 0) },
				{ Vec3(0, 1, 1), Vec3(0, 0, 0) }, { Vec3(1, 1, 1), Vec3(0, 0, 0) }, { Vec3(1, 0, 1), Vec3(0, 0, 0) },
				{ Vec3(1, 0, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 0), Vec3(0, 0, 0) }, { Vec3(0, 0, 0), Vec3(0, 0, 0) },
				{ Vec3(1, 0, 0), Vec3(0, 0, 0) }, { Vec3(1, 1, 0), Vec3(0, 0, 0) }, { Vec3(0, 1, 0), Vec3(0, 0, 0) }
			};

			m_pApplyVB = rd->m_DevBufMan.CreateVBuffer(CRY_ARRAY_COUNT(vbApplyCube), eVF_P3F_T3F, "LPVApply");
			rd->m_DevBufMan.UpdateVBuffer(m_pApplyVB, (void*)&vbApplyCube[0], CRY_ARRAY_COUNT(vbApplyCube));
		}

		// create VB for color shadow maps rendering(array of points)
		if (!m_pRSMInjPointsVB)
		{
			std::vector<SVF_T2F> vecRSMData;
			vecRSMData.resize(CRELightPropagationVolume::espMaxInjectRSMSize * CRELightPropagationVolume::espMaxInjectRSMSize);
			for (uint32 iVertex = 0; iVertex < vecRSMData.size(); ++iVertex)
				vecRSMData[iVertex].st = Vec2((float)iVertex, cry_random(0.0f, 1.0f));
			m_pRSMInjPointsVB = rd->m_DevBufMan.CreateVBuffer(vecRSMData.size(), eVF_T2F, "RSMInjectPoints");

			rd->m_DevBufMan.UpdateVBuffer(m_pRSMInjPointsVB, &vecRSMData.front(), vecRSMData.size());
		}

		if (!m_poolRT.GetTarget()[0])
		{
			assert(m_poolRT.GetTarget()[1] == NULL && m_poolRT.GetTarget()[2] == NULL);

			// create RT pool
			CTexture* pRT0[3] = { NULL, NULL, NULL }, * pRT1[3] = { NULL, NULL, NULL };
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Red_Pool0", pRT0[0], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_R);
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Green_Pool0", pRT0[1], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_G);
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Blue_Pool0", pRT0[2], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_B);
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Red_Pool1", pRT1[0], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_R);
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Green_Pool1", pRT1[1], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_G);
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Blue_Pool1", pRT1[2], CRELightPropagationVolume::nMaxGridSize * CRELightPropagationVolume::nMaxGridSize, CRELightPropagationVolume::nMaxGridSize, Clr_Transparent, true, false, LPV_GI_FMT, TO_LPV_B);

			if (pRT0[0] == NULL || pRT0[1] == NULL || pRT0[2] == NULL ||
			    pRT1[0] == NULL || pRT1[1] == NULL || pRT1[2] == NULL)
			{
				assert(0);
				iLog->Log("Failed to initialize Light Propagation Volume: Insufficient video memory");
				return;
			}

			// clear targets
			for (int i = 0; i < 3; ++i)
			{
				gcpRendD3D->FX_ClearTarget(pRT0[i], Clr_Transparent);
				gcpRendD3D->FX_ClearTarget(pRT1[i], Clr_Transparent);
			}

			m_poolRT.SetRTs(pRT0, pRT1);
		}
	}
}

void CLightPropagationVolumesManager::Cleanup()
{
	m_pCurrentGIVolume = NULL;
	m_enabled = false;

	m_CachedRTs.Cleanup();

	m_RSM.Release();

	// destroy RT pool
	m_poolRT.SetRTs(NULL, NULL);
}

void CLightPropagationVolumesManager::RegisterLPV(CRELightPropagationVolume* p)
{
	p->m_nId = CryInterlockedIncrement(&m_nNextFreeId); // issue the new ID
	LPVSet::const_iterator it(m_grids.find(p));
	assert(it == m_grids.end() && "CLightPropagationVolumesManager::RegisterLPV() -- Object already registered!");
	m_grids.insert(p);    // Add new LPV to the grid list
}

void CLightPropagationVolumesManager::UnregisterLPV(CRELightPropagationVolume* p)
{
	LPVSet::iterator it(m_grids.find(p));
	assert(it != m_grids.end() && "CLightPropagationVolumesManager::UnregisterLPV() -- Object not registered or previously removed!");
	if (it != m_grids.end())
	{
		m_grids.erase(it);  // Remove LPV from the grid list
	}
	if (p->GetFlags() & CRELightPropagationVolume::efGIVolume)
	{
		it = m_setGIVolumes.find(p);
		assert(it != m_setGIVolumes.end());
		if (it != m_setGIVolumes.end())
			m_setGIVolumes.erase(it); // Remove LPV from the GI grid list
	}
}

void CLightPropagationVolumesManager::UpdateReflectiveShadowmapSize(SReflectiveShadowMap& rRSM, int nWidth, int nHeight)
{
	// If RSM is not initialized and/or was resized => reallocate RTs for it
	if (!rRSM.pNormalsRT || rRSM.pNormalsRT->GetWidth() != nWidth || rRSM.pNormalsRT->GetHeight() != nHeight)
	{
		rRSM.Release();
		uint32 nFlags = 0;

		// if we have r2vb available, use it by default
		union
		{
			ITexture** ppITex;
			CTexture** ppTex;
		} u;
		u.ppITex = &rRSM.pDepthRT;
		SD3DPostEffectsUtils::CreateRenderTarget("$RSMDepthMap", *u.ppTex, nWidth, nHeight, Clr_Unknown, false, false, RSM_DEPTH_FMT, TO_RSM_DEPTH, nFlags);

		u.ppITex = &rRSM.pNormalsRT;
		SD3DPostEffectsUtils::CreateRenderTarget("$RSMNormalMap", *u.ppTex, nWidth, nHeight, Clr_Unknown, false, false, RSM_NORMALS_FMT, TO_RSM_NORMAL, nFlags);

		u.ppITex = &rRSM.pFluxRT;
		SD3DPostEffectsUtils::CreateRenderTarget("$RSMColorMap", *u.ppTex, nWidth, nHeight, Clr_Unknown, false, false, RSM_FLUX_FMT, TO_RSM_COLOR, nFlags);
	}
}

void CLightPropagationVolumesManager::PingPongRTs::SetRTs(CTexture* pRT0[3], CTexture* pRT1[3])
{
	SAFE_RELEASE(m_pRT[0][0]);
	SAFE_RELEASE(m_pRT[0][1]);
	SAFE_RELEASE(m_pRT[0][2]);
	SAFE_RELEASE(m_pRT[1][0]);
	SAFE_RELEASE(m_pRT[1][1]);
	SAFE_RELEASE(m_pRT[1][2]);
	if (pRT0)
	{
		m_pRT[0][0] = pRT0[0];
		m_pRT[0][1] = pRT0[1];
		m_pRT[0][2] = pRT0[2];
	}
	if (pRT1)
	{
		m_pRT[1][0] = pRT1[0];
		m_pRT[1][1] = pRT1[1];
		m_pRT[1][2] = pRT1[2];
	}
}
/////////////////////////////////////////////////////////////////////////////
void CRELightPropagationVolume::InsertLight(const CDLight& light)
{
	if (!LPVManager.IsEnabled())
		return;

	const int& nThreadID = gRenDev->m_RP.m_nFillThreadID;

	m_lightsToPostinject[nThreadID][0].push_back(light);  // add light to render
}

void CRELightPropagationVolume::Evaluate()
{
	m_bIsRenderable = false;

	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	static ICVar* pGICache = iConsole->GetCVar("e_GICache");
	assert(pGICache);

	const int nCurrentFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

	const bool bCahchedValue = ((m_nGridFlags & efGIVolume) != 0) && (pGICache->GetIVal() > 0) &&
	                           ((nCurrentFrameID - m_nUpdateFrameID) < pGICache->GetIVal());

	LPVManager.m_pCurrentGIVolume = this;

	rd->m_cEF.mfRefreshSystemShader(LPV_SHADER_NAME, rd->m_cEF.s_ShaderLPV);

	if (LPVManager.IsEnabled())
	{
		if (!m_bIsUpToDate)
			UpdateRenderParameters();

		PROFILE_LABEL_SCOPE("LPV_EVALUATE");

		if (m_bNeedPropagate)
		{
			Propagate();  // propagate radiance
			m_bIsRenderable = true;
		}

		if (!bCahchedValue)
			m_bIsRenderable |= Postinject();

		// update frame ID
		if (m_bIsRenderable)
			m_nUpdateFrameID = nCurrentFrameID;

		// for RSM caching
		if (!m_bIsRenderable)
			m_bIsRenderable = bCahchedValue;

		Lights& lights = m_lightsToPostinject[rd->m_RP.m_nProcessThreadID][0];
		lights.clear();
	}

	LPVManager.m_pCurrentGIVolume = NULL;
}

void CRELightPropagationVolume::InjectReflectiveShadowMap(SReflectiveShadowMap& rRSM)
{
	if (!LPVManager.IsEnabled())
		return;

	if (!m_bIsUpToDate)
		UpdateRenderParameters();

	if (!rRSM.pDepthRT || !rRSM.pNormalsRT || !rRSM.pFluxRT)
	{
		assert(0);
		return;
	}

	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	LPVManager.m_pCurrentGIVolume = this;

	assert(m_pRT[0]);
	assert(rRSM.pDepthRT->GetWidth() <= CRELightPropagationVolume::espMaxInjectRSMSize && rRSM.pDepthRT->GetHeight() <= CRELightPropagationVolume::espMaxInjectRSMSize);

	PROFILE_LABEL_PUSH("LPV_INJECTION");

	// reset all flags
	const uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;

	if (m_bNeedClear)
	{
		m_bNeedClear = false;

		rd->FX_ClearTarget((CTexture*)m_pRT[0], Clr_Transparent);
		rd->FX_ClearTarget((CTexture*)m_pRT[1], Clr_Transparent);
		rd->FX_ClearTarget((CTexture*)m_pRT[2], Clr_Transparent);
	}

	CHK(rd->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
	CHK(rd->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
	CHK(rd->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));
	rd->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

	// save original RTs
	CTexture* pOriginalRSM = CTexture::s_ptexRSMFlux;

	// set up RTs
	CTexture::s_ptexRSMFlux = (CTexture*)rRSM.pFluxRT;

	CShader* pShader = rd->m_cEF.s_ShaderLPV;

	SD3DPostEffectsUtils::ShBeginPass(pShader, LPVManager.m_techInjectRSM, FEF_DONTSETSTATES);

	rd->SetCullMode(R_CULL_NONE);

	// calc light dir
	Matrix44A mxInvLight = rRSM.mxLightViewProj;
	mxInvLight.Invert();
	mxInvLight.Transpose();
	const Vec4 v0 = mxInvLight * Vec4(0, 0, 0, 1);
	const Vec4 v1 = mxInvLight * Vec4(0, 0, 1, 1);
	const Vec4 vDirToLight = Vec4((Vec3(v0.x, v0.y, v0.z) / v0.w - Vec3(v1.x, v1.y, v1.z) / v1.w).GetNormalized(), 1.f);
	Vec4 vDirToLightGridSpace = Vec4(m_RenderSettings.m_mat.TransformVector(Vec3(vDirToLight.x, vDirToLight.y, vDirToLight.z)).GetNormalized(), 0);
	vDirToLightGridSpace.x *= m_RenderSettings.m_invGridDimensions.x;
	vDirToLightGridSpace.y *= m_RenderSettings.m_invGridDimensions.y;
	vDirToLightGridSpace.z *= m_RenderSettings.m_invGridDimensions.z;

	// set up injection matrix
	Matrix44A mxInjectionMatrix = m_RenderSettings.m_mat * mxInvLight;
	static CCryNameR sInjectionMatrix("g_injectionMatrix");
	pShader->FXSetVSFloat(sInjectionMatrix, (Vec4*)mxInjectionMatrix.GetData(), 4);
	// set up constants
	static CCryNameR sLightDirGridSpace("g_dirToLightGridSpace");
	pShader->FXSetVSFloat(sLightDirGridSpace, &vDirToLightGridSpace, 1);
	static CCryNameR sLightDir("g_dirToLight");
	pShader->FXSetPSFloat(sLightDir, &vDirToLight, 1);
	static CCryNameR sSMSize("g_smSize");
	const Vec4 nSize((float)rRSM.pDepthRT->GetWidth(), (float)rRSM.pDepthRT->GetHeight(), 1.f / (float)rRSM.pDepthRT->GetWidth(), 1.f / (float)rRSM.pDepthRT->GetHeight());
	pShader->FXSetVSFloat(sSMSize, &nSize, 1);
	static CCryNameR sSMInvPixelSize("g_smInvPixelSize");

	// TODO: take into account light matrix rotation and perspective
	const float fMaxGridExtend = max(m_RenderSettings.m_gridDimensions.x, max(m_RenderSettings.m_gridDimensions.y, m_RenderSettings.m_gridDimensions.z));
	const float fTexelArea = fMaxGridExtend * fMaxGridExtend
	                         / (rRSM.pDepthRT->GetWidth() * rRSM.pDepthRT->GetHeight());
	const Vec4 nInvPixelSize(fTexelArea, 0, 0, 0);
	pShader->FXSetPSFloat(sSMInvPixelSize, &nInvPixelSize, 1);

	_injectWithVTF(rRSM);

	SD3DPostEffectsUtils::ShEndPass();
	CHK(rd->FX_PopRenderTarget(2));
	CHK(rd->FX_PopRenderTarget(1));
	CHK(rd->FX_PopRenderTarget(0));

	// set up the flag to propagate
	m_bNeedPropagate = true;

	rd->m_RP.m_FlagsShader_RT = nOldFlags;

	// restore original RTs
	CTexture::s_ptexRSMFlux = pOriginalRSM;

	PROFILE_LABEL_POP("LPV_INJECTION");

	// Inject the same point cloud from current RSM into the occlusion volume as occluders
	if (GetFlags() & efHasOcclusion)
		InjectOcclusionFromRSM(rRSM, false);

	// Reset current GI volume (for shader binding)
	LPVManager.m_pCurrentGIVolume = NULL;
}

void CRELightPropagationVolume::InjectOcclusionFromRSM(SReflectiveShadowMap& rCSM, bool bCamera)
{
	if (!(GetFlags() & efHasOcclusion))
		return;
	assert(m_pOcclusionTexture);
	assert(rCSM.pDepthRT);
	PROFILE_LABEL_SCOPE("LPV_OCCL_GEN");

	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	// reset all flags
	const uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT &= ~(RT_STICKED_TO_CAMERA);

	CTexture* pOcclTex = (CTexture*)m_pOcclusionTexture;

	const int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
	const bool bNeedClear = pOcclTex->m_nUpdateFrameID != nFrameID;

	// clear that
	if (bNeedClear)
	{
		rd->FX_ClearTarget(pOcclTex, Clr_Transparent);
	}

	CHK(rd->FX_PushRenderTarget(0, pOcclTex, NULL));

	if (bCamera)
		rd->m_RP.m_FlagsShader_RT |= RT_STICKED_TO_CAMERA;

	CShader* pShader = rd->m_cEF.s_ShaderLPV;

	static CCryNameTSCRC techInjectOcclusionMap("LPVInjectOcclusion");
	SD3DPostEffectsUtils::ShBeginPass(pShader, techInjectOcclusionMap, FEF_DONTSETSTATES);

	rd->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

	rd->SetCullMode(R_CULL_NONE);

	if (bCamera)
	{
		SD3DPostEffectsUtils::UpdateFrustumCorners();
		const Vec4 vLB(SD3DPostEffectsUtils::m_vLB, 0);
		const Vec4 vLT(SD3DPostEffectsUtils::m_vLT, 0);
		const Vec4 vRB(SD3DPostEffectsUtils::m_vRB, 0);
		const Vec4 vRT(SD3DPostEffectsUtils::m_vRT, 0);
		pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumLB, &vLB, 1);
		pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumLT, &vLT, 1);
		pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumRB, &vRB, 1);
		pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumRT, &vRT, 1);
	}

	// set up injection matrix
	Matrix44A mxInjectionMatrix;
	if (bCamera)
	{
		mxInjectionMatrix = m_RenderSettings.m_mat;
	}
	else
	{
		// calc light unprojection mx
		Matrix44A mxInvLight = rCSM.mxLightViewProj;
		mxInvLight.Invert();
		mxInvLight.Transpose();
		mxInjectionMatrix = m_RenderSettings.m_mat * mxInvLight;
	}

	static CCryNameR sInjectionMatrix("g_injectionMatrix");
	pShader->FXSetVSFloat(sInjectionMatrix, (Vec4*)mxInjectionMatrix.GetData(), 4);
	static CCryNameR sSMSize("g_smSize");
	const Vec4 nSize((float)rCSM.pDepthRT->GetWidth(), (float)rCSM.pDepthRT->GetHeight(), 1.f / (float)rCSM.pDepthRT->GetWidth(), 1.f / (float)rCSM.pDepthRT->GetHeight());
	pShader->FXSetVSFloat(sSMSize, &nSize, 1);
	static CCryNameR sSMInvPixelSize("g_smInvPixelSize");

	const float fMaxGridExtend = max(m_RenderSettings.m_gridDimensions.x, max(m_RenderSettings.m_gridDimensions.y, m_RenderSettings.m_gridDimensions.z));
	const float fTexelArea = fMaxGridExtend * fMaxGridExtend / (rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight());
	const Vec4 nInvPixelSize(fTexelArea, 0, 0, 0);
	pShader->FXSetPSFloat(sSMInvPixelSize, &nInvPixelSize, 1);

	_injectWithVTF(rCSM);

	SD3DPostEffectsUtils::ShEndPass();
	CHK(rd->FX_PopRenderTarget(0));

	rd->m_RP.m_FlagsShader_RT = nOldFlags;
}

void CRELightPropagationVolume::_injectWithVTF(SReflectiveShadowMap& rCSM)
{
	HRESULT h = S_OK;

	// save texture stage info. Vertex samplers will trample pixel sampler state otherwise
	STexStageInfo pPrevTexState0 = CTexture::s_TexStages[0];
	STexStageInfo pPrevTexState1 = CTexture::s_TexStages[1];

	// set up vertex RT
	((CTexture*)rCSM.pDepthRT)->SetVertexTexture(true);
	SD3DPostEffectsUtils::SetTexture((CTexture*)rCSM.pDepthRT, 0, FILTER_POINT, TADDR_BORDER);
	((CTexture*)rCSM.pNormalsRT)->SetVertexTexture(true);
	SD3DPostEffectsUtils::SetTexture((CTexture*)rCSM.pNormalsRT, 1, FILTER_POINT, TADDR_BORDER);

	// set vb
	CVertexBuffer* vb = LPVManager.m_pRSMInjPointsVB;
	//h = gcpRendD3D->FX_SetVertexDeclaration(0, vb->m_eVF);
	assert(vb->m_eVF == eVF_T2F);
	if (!LPVManager.m_VTFVertexDeclaration)
	{
		gcpRendD3D->FX_Commit();
		D3D11_INPUT_ELEMENT_DESC vdElems[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		// in case we still don't have shader we're just still compiling it
		if (!CHWShader_D3D::s_pCurInstVS || !CHWShader_D3D::s_pCurInstVS->m_pShaderData)
			return;
		h = gcpRendD3D->GetDevice().CreateInputLayout(vdElems, 1, CHWShader_D3D::s_pCurInstVS->m_pShaderData, CHWShader_D3D::s_pCurInstVS->m_nDataSize, &LPVManager.m_VTFVertexDeclaration);
		assert(SUCCEEDED(h));
	}
	gcpRendD3D->m_DevMan.BindVtxDecl(LPVManager.m_VTFVertexDeclaration);
	assert(LPVManager.m_VTFVertexDeclaration);
	gcpRendD3D->m_pLastVDeclaration = LPVManager.m_VTFVertexDeclaration;

	if (LPVManager.m_VTFVertexDeclaration)
	{
		int nPrimitives = rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight();
		size_t nOffset = 0;
		D3DVertexBuffer* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(vb->m_VS.m_BufferHdl, &nOffset);
		assert(pRawVB);
		h = gcpRendD3D->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh::m_cSizeVF[vb->m_eVF]);
		assert(SUCCEEDED(h));

		// draw
		gcpRendD3D->FX_Commit();
		gcpRendD3D->FX_DrawPrimitive(eptPointList, 0, nPrimitives);
	}
	//else
	//	assert(0);

	((CTexture*)rCSM.pDepthRT)->SetVertexTexture(false);
	((CTexture*)rCSM.pNormalsRT)->SetVertexTexture(false);

	// restore sampler info
	CTexture::s_TexStages[0] = pPrevTexState0;
	CTexture::s_TexStages[1] = pPrevTexState1;
}

void CRELightPropagationVolume::_injectOcclusionFromCamera()
{
	PROFILE_LABEL_SCOPE("LPV_CAMERA_OCCLUSION");

	// Allocate RTs for downscaled G-Buffers
	SDynTexture* pDownscNBuffer = new SDynTexture(CRELightPropagationVolume::espMaxInjectRSMSize, CRELightPropagationVolume::espMaxInjectRSMSize, CTexture::s_ptexSceneNormalsMap->GetTextureDstFormat(), eTT_2D, FT_STATE_CLAMP, "TempDownscNBuff");
	SDynTexture* pDownscDBuffer = new SDynTexture(CRELightPropagationVolume::espMaxInjectRSMSize, CRELightPropagationVolume::espMaxInjectRSMSize, CTexture::s_ptexZTarget->GetTextureDstFormat(), eTT_2D, FT_STATE_CLAMP, "TempDownscDBuff");

	if (pDownscNBuffer && pDownscDBuffer)
	{
		pDownscNBuffer->Update(CRELightPropagationVolume::espMaxInjectRSMSize, CRELightPropagationVolume::espMaxInjectRSMSize);
		pDownscDBuffer->Update(CRELightPropagationVolume::espMaxInjectRSMSize, CRELightPropagationVolume::espMaxInjectRSMSize);

		if (pDownscNBuffer->m_pTexture && pDownscDBuffer->m_pTexture)
		{
			// Downscale G-Buffers with min filter
			PostProcessUtils().StretchRect(CTexture::s_ptexSceneNormalsMap, pDownscNBuffer->m_pTexture);
			PostProcessUtils().StretchRect(CTexture::s_ptexZTarget, pDownscDBuffer->m_pTexture);

			// Use a dummy RSM to inject the occlusion data from downsampled G-Buffer
			SReflectiveShadowMap cameraRSM;
			cameraRSM.pDepthRT = pDownscDBuffer->m_pTexture;
			cameraRSM.pNormalsRT = pDownscNBuffer->m_pTexture;

			InjectOcclusionFromRSM(cameraRSM, true);
		}
	}

	SAFE_DELETE(pDownscNBuffer);
	SAFE_DELETE(pDownscDBuffer);
}

void CRELightPropagationVolume::Propagate()
{
	m_bNeedPropagate = false;

	if (m_bHasSpecular)
		return;

	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	static ICVar* pGIPropagationAmp = iConsole->GetCVar("e_GIPropagationAmp");
	assert(pGIPropagationAmp);

	// inject occlusion from camera
	if (GetFlags() & efHasOcclusion)
		_injectOcclusionFromCamera();

	PROFILE_LABEL_SCOPE("LPV_PROPAGATION");

	// prepare renderer to injection
	assert(m_pRT[0]);

	rd->SetCullMode(R_CULL_NONE);

	uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;

	const Vec3 vCellLength = Vec3(m_RenderSettings.m_matInv.TransformVector(Vec3(m_RenderSettings.m_invGridDimensions.x, 0, 0)).GetLength(),
	                              m_RenderSettings.m_matInv.TransformVector(Vec3(0, m_RenderSettings.m_invGridDimensions.y, 0)).GetLength(),
	                              m_RenderSettings.m_matInv.TransformVector(Vec3(0, 0, m_RenderSettings.m_invGridDimensions.z)).GetLength());

	CTexture* pOriginalRTs[3] = { CTexture::s_ptexLPV_RTs[0], CTexture::s_ptexLPV_RTs[1], CTexture::s_ptexLPV_RTs[2] };
	CTexture* pOriginalColorMap = CTexture::s_ptexRSMFlux;
	CTexture* pOriginalNormalMap = CTexture::s_ptexRSMNormals;
	CTexture* pOriginalDepthMap = CTexture::s_ptexRSMDepth;

	//	Solve equation in the 3D texture
	for (int iIter = 1; iIter < m_RenderSettings.m_nNumIterations; ++iIter)
	{
		CTexture** rtPrev = iIter == 1 ? m_pRTex : LPVManager.m_poolRT.GetTarget();
		LPVManager.m_poolRT.Swap();
		CTexture** rt = LPVManager.m_poolRT.GetTarget();

		if (iIter == 1)
		{
			rd->FX_ClearTarget(rt[0], Clr_Transparent);
			rd->FX_ClearTarget(rt[1], Clr_Transparent);
			rd->FX_ClearTarget(rt[2], Clr_Transparent);
		}

		// iterations
		CHK(rd->FX_PushRenderTarget(0, rt[0], NULL));
		CHK(rd->FX_PushRenderTarget(1, rt[1], NULL));
		CHK(rd->FX_PushRenderTarget(2, rt[2], NULL));
		rd->FX_SetState(GS_NODEPTHTEST);

		// set textures
		CTexture::s_ptexLPV_RTs[0] = rtPrev[0];
		CTexture::s_ptexLPV_RTs[1] = rtPrev[1];
		CTexture::s_ptexLPV_RTs[2] = rtPrev[2];

		nOldFlags = rd->m_RP.m_FlagsShader_RT;

		// set RT flags
		if (GetFlags() & efHasOcclusion && iIter > 1)
		{
			assert(m_pOcclusionTexture);
			rd->m_RP.m_FlagsShader_RT |= RT_USE_OCCLUSION;

			// set up RTs
			CTexture::s_ptexRSMFlux = (CTexture*)m_pOcclusionTexture;
		}

		CShader* pShader = rd->m_cEF.s_ShaderLPV;

		SD3DPostEffectsUtils::ShBeginPass(pShader, LPVManager.m_techPropagateTechName, FEF_DONTSETSTATES);

		// calculate dx/dy/dz
		Vec4 vPropagationAmp_Iteration(pGIPropagationAmp->GetFVal(), float(iIter), 0, 0);
		static CCryNameR sPropagationAmp_Iteration("g_PropagationAmp_Iteration");
		pShader->FXSetPSFloat(sPropagationAmp_Iteration, &vPropagationAmp_Iteration, 1);

		rd->FX_SetState(GS_NODEPTHTEST);
		if (SUCCEEDED(rd->FX_SetVertexDeclaration(0, LPVManager.m_pPropagateVB->m_eVF)))
		{
			size_t nOffset = 0;
			D3DVertexBuffer* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(LPVManager.m_pPropagateVB->m_VS.m_BufferHdl, &nOffset);
			assert(pRawVB);
			HRESULT h = rd->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh::m_cSizeVF[LPVManager.m_pPropagateVB->m_eVF]);
			assert(SUCCEEDED(h));
			rd->FX_Commit();
			rd->FX_DrawPrimitive(eptTriangleList, 0, m_RenderSettings.m_nGridDepth * 6);
		}
		SD3DPostEffectsUtils::ShEndPass();
		CHK(rd->FX_PopRenderTarget(2));
		CHK(rd->FX_PopRenderTarget(1));
		CHK(rd->FX_PopRenderTarget(0));

		// collect data
		{
			CHK(rd->FX_PushRenderTarget(0, m_pRTex[0], NULL));
			CHK(rd->FX_PushRenderTarget(1, m_pRTex[1], NULL));
			CHK(rd->FX_PushRenderTarget(2, m_pRTex[2], NULL));
		}

		gcpRendD3D->FX_SetActiveRenderTargets();

		// set textures
		CTexture::s_ptexLPV_RTs[0] = rt[0];
		CTexture::s_ptexLPV_RTs[1] = rt[1];
		CTexture::s_ptexLPV_RTs[2] = rt[2];

		rd->m_RP.m_FlagsShader_RT = nOldFlags;

		int nStates = GS_NODEPTHTEST;
		nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;
		SD3DPostEffectsUtils::ShBeginPass(pShader, LPVManager.m_techCollectTechName, FEF_DONTSETSTATES);
		rd->FX_SetState(nStates);

		if (SUCCEEDED(rd->FX_SetVertexDeclaration(0, LPVManager.m_pPropagateVB->m_eVF)))
		{
			size_t nOffset = 0;
			D3DVertexBuffer* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(LPVManager.m_pPropagateVB->m_VS.m_BufferHdl, &nOffset);
			assert(pRawVB);
			HRESULT h = rd->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh::m_cSizeVF[LPVManager.m_pPropagateVB->m_eVF]);
			assert(SUCCEEDED(h));
			rd->FX_Commit();
			rd->FX_DrawPrimitive(eptTriangleList, 0, m_RenderSettings.m_nGridDepth * 6);
		}

		SD3DPostEffectsUtils::ShEndPass();

		// if it's the last one, resolve into the volume texture
		if (iIter == m_RenderSettings.m_nNumIterations - 1)
			ResolveToVolumeTexture();

		CHK(rd->FX_PopRenderTarget(2));
		CHK(rd->FX_PopRenderTarget(1));
		CHK(rd->FX_PopRenderTarget(0));

		rd->m_RP.m_FlagsShader_RT = nOldFlags;
	}

	rd->m_RP.m_FlagsShader_RT = nOldFlags;

	// restore RTs
	CTexture::s_ptexLPV_RTs[0] = pOriginalRTs[0];
	CTexture::s_ptexLPV_RTs[1] = pOriginalRTs[1];
	CTexture::s_ptexLPV_RTs[2] = pOriginalRTs[2];
	CTexture::s_ptexRSMFlux = pOriginalColorMap;
	CTexture::s_ptexRSMNormals = pOriginalNormalMap;
	CTexture::s_ptexRSMDepth = pOriginalDepthMap;
}

bool CRELightPropagationVolume::Postinject()
{
	Lights& lights = m_lightsToPostinject[gcpRendD3D->m_RP.m_nProcessThreadID][0];
	if (lights.empty())
		return false;

	assert(m_pRT[0]);

	if (m_bNeedClear)
	{
		m_bNeedClear = false;

		gcpRendD3D->FX_ClearTarget((CTexture*)m_pRT[0], Clr_Transparent);
		gcpRendD3D->FX_ClearTarget((CTexture*)m_pRT[1], Clr_Transparent);
		gcpRendD3D->FX_ClearTarget((CTexture*)m_pRT[2], Clr_Transparent);
	}

	CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));

	assert(LPVManager.m_pPostinjectVB);

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	// bind main vb
	CVertexBuffer* vb = LPVManager.m_pPostinjectVB;
	size_t nOffset = 0;
	D3DVertexBuffer* pVB = gcpRendD3D->m_DevBufMan.GetD3DVB(vb->m_VS.m_BufferHdl, &nOffset);
	assert(pVB);
	HRESULT h = gcpRendD3D->FX_SetVStream(0, pVB, nOffset, CRenderMesh::m_cSizeVF[vb->m_eVF]);  // R32F
	assert(SUCCEEDED(h));

	CShader* pShader = gcpRendD3D->m_cEF.s_ShaderLPV;

	// inject all light sources
	for (int32 iLight = 0; iLight < lights.size(); ++iLight)
	{
		int nStates = GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE;
		gcpRendD3D->FX_SetState(nStates);
		SD3DPostEffectsUtils::ShBeginPass(pShader, LPVManager.m_techPostinjectLight, FEF_DONTSETSTATES);
		PostnjectLight(lights[iLight]);
		SD3DPostEffectsUtils::ShEndPass();
	}

	// if it's the last one, resolve
	ResolveToVolumeTexture();

	CHK(gcpRendD3D->FX_PopRenderTarget(2));
	CHK(gcpRendD3D->FX_PopRenderTarget(1));
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	lights.clear();

	return true;
}

void CRELightPropagationVolume::PostnjectLight(const CDLight& rLight)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	// calc bounding in grid space to get number of slices to render
	const Vec3 vRadius = Vec3(rLight.m_fRadius, rLight.m_fRadius, rLight.m_fRadius);
	AABB lightBox(rLight.m_Origin - vRadius, rLight.m_Origin + vRadius);
	Matrix34 mx(m_RenderSettings.m_mat);
	lightBox.SetTransformedAABB(mx, lightBox);
	int nEndSlice = max(0, min((int)CRELightPropagationVolume::nMaxGridSize, (int)ceilf(lightBox.max.z * m_RenderSettings.m_gridDimensions.z)));
	int nStartSlice = max(0, min((int)CRELightPropagationVolume::nMaxGridSize, (int)floorf(lightBox.min.z * m_RenderSettings.m_gridDimensions.z)));
	nEndSlice = max(nEndSlice, nStartSlice);
	int nSlices = min((int)CRELightPropagationVolume::nMaxGridSize, nEndSlice - nStartSlice + 1);

	CShader* pShader = rd->m_cEF.s_ShaderLPV;

	const Vec4 vcLightPos(rLight.m_Origin, rLight.m_fRadius);
	Vec4 cLightCol = rLight.m_Color.toVec4();
	cLightCol.w = (float)nSlices; // store num slices in the alpha channel of color

	// Apply LBuffers range rescale
	cLightCol.x *= rd->m_fAdaptedSceneScaleLBuffer;
	cLightCol.y *= rd->m_fAdaptedSceneScaleLBuffer;
	cLightCol.z *= rd->m_fAdaptedSceneScaleLBuffer;

	pShader->FXSetVSFloat(LPVManager.m_semLightPositionSemantic, &vcLightPos, 1);
	pShader->FXSetVSFloat(LPVManager.m_semLightColorSemantic, &cLightCol, 1);

	// draw
	rd->FX_Commit();
	rd->FX_DrawPrimitive(eptTriangleList, 0, nSlices * 6);
}

void CRELightPropagationVolume::DeferredApply()
{
	if (!m_bIsRenderable)
		return;

	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	const int nThreadID = rd->m_RP.m_nProcessThreadID;

	LPVManager.m_pCurrentGIVolume = this;

	// DEBUG
	{
		//rd->GetIRenderAuxGeom()->DrawAABB(m_pParent->GetBBox(), false, Col_White, eBBD_Faceted);
		//float density = m_pParent->GetDensity();
		//Vec3 size = m_pParent->GetBBox().GetSize() * density;
		//Vec3 origSize = size;
		//Vec3 offset = size;
		//size.x = floor_tpl(size.x);
		//size.y = floor_tpl(size.y);
		//size.z = floor_tpl(size.z);
		//offset -= size;
		//offset *= .5f;
		//for(float i=offset.x;i<origSize.x;++i)
		//	for(float j=offset.y;j<origSize.y;++j)
		//		for(float k=offset.z;k<origSize.z;++k)
		//		{
		//			Vec3 pos = Vec3(i/origSize.x, j/origSize.y, k/origSize.z);
		//			pos = m_RenderSettings.m_matInv.TransformPoint(pos);
		//			Vec3 siz = Vec3(.5f, .5f, .5f);
		//			AABB bb(pos - siz, pos + siz);
		//			rd->GetIRenderAuxGeom()->DrawAABB(bb, false, Col_White, eBBD_Faceted);
		//			//rd->GetIRenderAuxGeom()->DrawAABB(Vec3(i/origSize.x-.5f, j/origSize.y-.5f, k/origSize.z-.5f), Col_CornflowerBlue, 10);
		//		}
	}

	PROFILE_LABEL_SCOPE("LPV_APPLY");

	assert(m_pRT[0]);

	const uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT &= ~(RT_USE_VOLUME_TEXTURE | RT_APPLY_SPECULAR | RT_VISAREA_CLIP_ID | RT_STICKED_TO_CAMERA | RT_DEBUG_VISUALIZATION);

	const Settings& rRendSettings = m_RenderSettings;
	const Vec3 gridCenter = rRendSettings.m_matInv.TransformPoint(Vec3(.5f, .5f, .5f));

	// setup textures
	LPVManager.SetGIVolumes(this);

	if (m_pVolumeTextures[0] && m_pVolumeTextures[0] != m_pRT[0])
		rd->m_RP.m_FlagsShader_RT |= RT_USE_VOLUME_TEXTURE;

	if (m_nGridFlags & efGIVolume)
		rd->m_RP.m_FlagsShader_RT |= RT_STICKED_TO_CAMERA;

	if (CRenderer::CV_r_LightPropagationVolumes > 1)
		rd->m_RP.m_FlagsShader_RT |= RT_DEBUG_VISUALIZATION;
	else if (m_bHasSpecular)
		rd->m_RP.m_FlagsShader_RT |= RT_APPLY_SPECULAR;

	const Vec4* pVisAreaParams = NULL;
	uint32 nVisAreaCount = 0;
	CDeferredShading::Instance().GetClipVolumeParams(pVisAreaParams, nVisAreaCount);

	if (nVisAreaCount > 0)
		rd->m_RP.m_FlagsShader_RT |= RT_VISAREA_CLIP_ID;

	CShader* pShader = rd->m_cEF.s_ShaderLPV;

	if (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
	{
		const bool bReverseDepth = (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
		const Vec4 pDepthBounds = CDeferredShading::Instance().GetLightDepthBounds(rRendSettings.m_pos, m_fDistance, bReverseDepth);
		rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true); // skip sky only for GI
	}

	// We cheating here for performance - usually GI subtle, maybe not much of issue
	//#ifdef SUPPORTS_MSAA
	//  const bool bUseStencil = (rd->m_RP.m_CurState & GS_STENCIL) != 0;
	//  const int32 nNumPassesMSAA = (rd->FX_GetMSAAMode() == 1)? 2: 1;
	//  rd->FX_MSAASampleFreqStencilSetup( (!bUseStencil)?  MSAA_STENCILMASK_RESET_BIT: 0 );
	//  for(int32 nPass =0; nPass<nNumPassesMSAA;++nPass)
	//  {
	//    if (nPass==1)
	//    {
	//      PROFILE_LABEL_PUSH( "LPV_APPLY_SAMPLEFREQPASS" );
	//      rd->FX_MSAASampleFreqStencilSetup(MSAA_SAMPLEFREQ_PASS| (bUseStencil?MSAA_STENCILMASK_RESET_BIT:0));
	//    }
	//#endif

	SD3DPostEffectsUtils::ShBeginPass(pShader, LPVManager.m_techApplyTechName, FEF_DONTSETSTATES);

	rd->SetCullMode(R_CULL_BACK);

	int nStates = GS_NODEPTHTEST;
	// preserve current stencil state for GI in order to support clip volumes
	if (m_nGridFlags & efGIVolume)
		nStates |= (rd->m_RP.m_CurState & GS_STENCIL);
	nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;   // usual additive blending for Light Propagation Volumes

	//#ifdef SUPPORTS_MSAA
	//    if( !bUseStencil || nPass==1 )
	//    {
	//      // For GI stencil batching strategy is:
	//      //    - no stencil used for GI ? reset mask to 0 in tagged regions at start, use for all passes
	//      //    - stencil used for GI ? reset mask to 0 in tagged regions for sample freq pass
	//      bool bStFuncEq = true;
	//      int32 nPrevStencilRef = rd->m_nStencilMaskRef;
	//
	//      if( !bUseStencil )
	//      {
	//        rd->m_nStencilMaskRef = 0;
	//        bStFuncEq = (nPass == 0)? false : true;
	//      }
	//
	//      rd->FX_StencilTestCurRef(true, false, bStFuncEq);
	//      rd->m_nStencilMaskRef = nPrevStencilRef;
	//      nStates |= GS_STENCIL;
	//    }
	// #endif

	rd->FX_SetState(nStates);

	// set frustum corners
	SD3DPostEffectsUtils::UpdateFrustumCorners();
	const Vec4 vLB(SD3DPostEffectsUtils::m_vLB, 0);
	const Vec4 vLT(SD3DPostEffectsUtils::m_vLT, 0);
	const Vec4 vRB(SD3DPostEffectsUtils::m_vRB, 0);
	const Vec4 vRT(SD3DPostEffectsUtils::m_vRT, 0);
	pShader->FXSetVSFloat(LPVManager.m_semCameraMatrix, (Vec4*) rd->m_CameraProjMatrix.GetData(), 4);
	pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumLB, &vLB, 1);
	pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumLT, &vLT, 1);
	pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumRB, &vRB, 1);
	pShader->FXSetVSFloat(LPVManager.m_semCameraFrustrumRT, &vRT, 1);

	SD3DPostEffectsUtils::SetTexture(CTexture::s_ptexBackBuffer, 6, FILTER_POINT);

	if (nVisAreaCount)
	{
		pShader->FXSetPSFloat(LPVManager.m_semVisAreaParams, pVisAreaParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nVisAreaCount + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));
		SD3DPostEffectsUtils::SetTexture(CDeferredShading::Instance().GetResolvedStencilRT(), 7, FILTER_POINT);
	}

	if (SUCCEEDED(rd->FX_SetVertexDeclaration(0, LPVManager.m_pApplyVB->m_eVF)))
	{
		size_t nOffset = 0;
		D3DVertexBuffer* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(LPVManager.m_pApplyVB->m_VS.m_BufferHdl, &nOffset);
		assert(pRawVB);
		HRESULT h = rd->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh::m_cSizeVF[LPVManager.m_pApplyVB->m_eVF]);
		assert(SUCCEEDED(h));
		rd->FX_Commit();
		rd->FX_DrawPrimitive(eptTriangleList, 0, 36);
	}

	SD3DPostEffectsUtils::ShEndPass();
	//#ifdef SUPPORTS_MSAA
	//    if (nPass == 1)
	//    {
	//      rd->FX_MSAASampleFreqStencilSetup();
	//      PROFILE_LABEL_POP( "LPV_APPLY_SAMPLEFREQPASS" );
	//    }
	//  }
	//#endif
	// set up the clear-once flag
	m_bNeedClear = true;
	// reset the renderable flag
	m_bIsRenderable = false;

	rd->m_RP.m_FlagsShader_RT = nOldFlags;

	// restore RTs
	LPVManager.UnsetGIVolumes();

	if (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
		rd->SetDepthBoundTest(0.f, 1.f, false);

	LPVManager.m_pCurrentGIVolume = NULL;
}

void CRELightPropagationVolume::ResolveToVolumeTexture()
{
#ifdef USE_VOLUME_TEXTURE
	if (!m_pVolumeTextures[0] || m_pVolumeTextures[0] == m_pRT[0])
		return;
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	PROFILE_LABEL_SCOPE("LPV_RESOLVE_3D");
	assert(m_RenderSettings.m_nGridWidth % 32 == 0 && m_RenderSettings.m_nGridHeight % 32 == 0 && m_RenderSettings.m_nGridDepth % 32 == 0);
#endif
}

void CRELightPropagationVolume::UpdateRenderParameters()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	// load shader
	rd->m_cEF.mfRefreshSystemShader(LPV_SHADER_NAME, rd->m_cEF.s_ShaderLPV);

	// update RT if necessary
	if (!m_pRT[0] || (m_pRT[0]->GetWidth() != m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth || m_pRT[0]->GetHeight() != m_RenderSettings.m_nGridHeight))
	{
		Cleanup();

		ETEX_Format eVolumeFmt = (m_nGridFlags & efGIVolume) ? LPV_GI_FMT : LPV_HDR_VOLUME_FMT;

		CLightPropagationVolumesManager::CCachedRTs& rCachedRTs = LPVManager.GetCachedRTs();
		if (!rCachedRTs.GetRT(0) || rCachedRTs.IsAcquired() ||
		    rCachedRTs.GetRT(0)->GetWidth() != m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth ||
		    rCachedRTs.GetRT(0)->GetHeight() != m_RenderSettings.m_nGridHeight)
		{
			char str[256];
			bool bRTAllocated = true;
			cry_sprintf(str, "$LPV_Red_RT%d", m_nId);
			bRTAllocated &= SD3DPostEffectsUtils::CreateRenderTarget(str, m_pRTex[0], m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, Clr_Transparent, true, false, eVolumeFmt, TO_LPV_R);
			cry_sprintf(str, "$LPV_Green_RT%d", m_nId);
			bRTAllocated &= SD3DPostEffectsUtils::CreateRenderTarget(str, m_pRTex[1], m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, Clr_Transparent, true, false, eVolumeFmt, TO_LPV_G);
			cry_sprintf(str, "$LPV_Blue_RT%d", m_nId);
			bRTAllocated &= SD3DPostEffectsUtils::CreateRenderTarget(str, m_pRTex[2], m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, Clr_Transparent, true, false, eVolumeFmt, TO_LPV_B);

			if (!bRTAllocated || m_pRTex[0] == NULL || m_pRTex[1] == NULL || m_pRTex[2] == NULL)
			{
				SAFE_RELEASE(m_pRTex[0]);
				SAFE_RELEASE(m_pRTex[1]);
				SAFE_RELEASE(m_pRTex[2]);
				assert(0);
				LogWarning("Failed to initialize Light Propagation Volume: Insufficient video memory");
				return;
			}

			// set up cached RTs if it's the first time
			if (!LPVManager.GetCachedRTs().GetRT(0))
			{
				LPVManager.GetCachedRTs().SetCachedRTs(m_pRTex[0], m_pRTex[1], m_pRTex[2]);
				if (!LPVManager.GetCachedRTs().IsAcquired())
					LPVManager.GetCachedRTs().Acquire();
			}
		}
		else
		{
			LPVManager.GetCachedRTs().Acquire();
			m_pRT[0] = rCachedRTs.GetRT(0);
			m_pRT[0]->AddRef();
			m_pRT[1] = rCachedRTs.GetRT(1);
			m_pRT[1]->AddRef();
			m_pRT[2] = rCachedRTs.GetRT(2);
			m_pRT[2]->AddRef();
		}

		m_pVolumeTextures[0] = m_pRT[0];
		m_pVolumeTextures[1] = m_pRT[1];
		m_pVolumeTextures[2] = m_pRT[2];

		// create RT for secondary occlusion
		if (GetFlags() & efHasOcclusion)
		{
			SD3DPostEffectsUtils::CreateRenderTarget("$LPV_Occlusion", m_pOcclusionTex, m_RenderSettings.m_nGridWidth * m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, Clr_Transparent, true, false, LPV_OCCL_VOLUME_FMT, TO_RSM_COLOR);
			assert(m_pOcclusionTexture);
			rd->FX_ClearTarget(m_pOcclusionTex);
		}

#ifdef USE_VOLUME_TEXTURE
		if (m_RenderSettings.m_nGridWidth % 32 == 0 && m_RenderSettings.m_nGridHeight % 32 == 0 && m_RenderSettings.m_nGridDepth % 32 == 0)
		{
			static const uint32 flags = FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS;
			char str[256];
			cry_sprintf(str, "$LPV_Red_RT%dVol", m_nId);
			m_pVolumeTextures[0] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags, NULL,
			                                                 eVolumeFmt, eVolumeFmt);
			cry_sprintf(str, "$LPV_Green_RT%dVol", m_nId);
			m_pVolumeTextures[1] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags, NULL,
			                                                 eVolumeFmt, eVolumeFmt);
			cry_sprintf(str, "$LPV_Blue_RT%dVol", m_nId);
			m_pVolumeTextures[2] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags, NULL,
			                                                 eVolumeFmt, eVolumeFmt);
			assert(m_pVolumeTextures[0] && m_pVolumeTextures[1] && m_pVolumeTextures[2]);
			m_pVolumeTexs[0]->SetCustomID(TO_LPV_R);
			m_pVolumeTexs[1]->SetCustomID(TO_LPV_G);
			m_pVolumeTexs[2]->SetCustomID(TO_LPV_B);
		}
#endif
	}

	if (GetFlags() & efGIVolume)
	{
		LPVManager.m_setGIVolumes.insert(this);
	}

	m_bIsUpToDate = true;
}

CRELightPropagationVolume::Settings* CRELightPropagationVolume::GetFillSettings()
{
	ASSERT_IS_MAIN_THREAD(gcpRendD3D->m_pRT);
	return &m_Settings[gcpRendD3D->m_RP.m_nFillThreadID];
}

void CRELightPropagationVolume::DuplicateFillSettingToOtherSettings()
{
	for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
		if (i != gcpRendD3D->m_RP.m_nFillThreadID)
			memcpy(&m_Settings[i], &m_Settings[gcpRendD3D->m_RP.m_nFillThreadID], sizeof(Settings));
}

const CRELightPropagationVolume::Settings& CRELightPropagationVolume::GetRenderSettings() const
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	ASSERT_IS_RENDER_THREAD(rd->m_pRT);
	return m_RenderSettings;
}

bool CRELightPropagationVolume::mfPreDraw(SShaderPass* sl)
{
	return true;
}
