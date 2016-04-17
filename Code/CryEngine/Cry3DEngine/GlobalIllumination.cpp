// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id: GlobalIllumination.cpp,v 1.0 2008/08/4 12:14:13 AntonKaplanyan Exp wwwrun $
   $DateTime$
   Description:  Routine for rendering and managing of global illumination
              approach based on reflective shadow maps and light propagation volumes
   -------------------------------------------------------------------------
   History:
   - 4:8:2008   12:14 : Created by Anton Kaplanyan
   - 26:1:2011  11:18 : Refactored by Anton Kaplanyan
*************************************************************************/

#include "StdAfx.h"

#include "GlobalIllumination.h"
#include "LightEntity.h"
#include "LPVRenderNode.h"
#include <CryRenderer/RenderElements/CRELightPropagationVolume.h>

#define fSunDistance (7000.)

CGlobalIlluminationManager::CGlobalIlluminationManager() : m_bEnabled(false), m_fMaxDistance(0.f), m_fCascadesRatio(0.f), m_fOffset(0.f), m_GlossyReflections(false), m_SecondaryOcclusion(false), m_nUpdateFrameId(-1)
{
}

void CGlobalIlluminationManager::Update(const CCamera& rCamera)
{
	// Check the valid range of CVars
#ifndef CONSOLE_CONST_CVAR_MODE
	Cry3DEngineBase::GetCVars()->e_GIOffset = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIOffset, 0.f, 1.f);
	Cry3DEngineBase::GetCVars()->e_GIBlendRatio = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIBlendRatio, 0.1f, 2.f);
	Cry3DEngineBase::GetCVars()->e_GIPropagationAmp = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIPropagationAmp, 1.f, 5.f);
	Cry3DEngineBase::GetCVars()->e_GICache = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GICache, 0, 30);
	Cry3DEngineBase::GetCVars()->e_GINumCascades = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GINumCascades, 1, 6);
	Cry3DEngineBase::GetCVars()->e_GIMaxDistance = max(Cry3DEngineBase::GetCVars()->e_GIMaxDistance, 10.f);
	Cry3DEngineBase::GetCVars()->e_GIIterations = min((int32)CRELightPropagationVolume::nMaxGridSize, max(Cry3DEngineBase::GetCVars()->e_GIIterations, 0));
#endif

	// update local vars from console ones
	const bool bGIEnabled = Cry3DEngineBase::GetCVars()->e_GI != 0 && (GetFloatCVar(e_GIAmount) * gEnv->p3DEngine->GetGIAmount() > .01f);
	const bool bGlossyReflections = Cry3DEngineBase::GetCVars()->e_GIGlossyReflections != 0;
	const bool bSecondaryOcclusion = Cry3DEngineBase::GetCVars()->e_GISecondaryOcclusion != 0;

	// check if it's disabled
	if (bGIEnabled == m_bEnabled && bGlossyReflections == m_GlossyReflections &&
	    bSecondaryOcclusion == m_SecondaryOcclusion &&
	    Cry3DEngineBase::GetCVars()->e_GINumCascades == m_Cascades.size()
#ifndef CONSOLE_CONST_CVAR_MODE
	    && GetFloatCVar(e_GIMaxDistance) == m_fMaxDistance
	    && GetFloatCVar(e_GICascadesRatio) == m_fCascadesRatio
	    && GetFloatCVar(e_GIOffset) == m_fOffset
#endif
	    )
		return;

	m_fMaxDistance = GetFloatCVar(e_GIMaxDistance);
	m_fCascadesRatio = GetFloatCVar(e_GICascadesRatio);
	m_fOffset = GetFloatCVar(e_GIOffset);
	m_GlossyReflections = bGlossyReflections;
	m_SecondaryOcclusion = bSecondaryOcclusion;
	Cleanup();

	if (bGIEnabled)
	{
		float fGICascadeSize = GetFloatCVar(e_GIMaxDistance);

		// create cascades
		for (int i = 0; i < Cry3DEngineBase::GetCVars()->e_GINumCascades; ++i)
		{
			CLPVCascadePtr pCascade(new CLPVCascade());
			m_Cascades.push_back(pCascade);
			pCascade->Init(fGICascadeSize, GetFloatCVar(e_GIOffset), rCamera);
			fGICascadeSize /= GetFloatCVar(e_GICascadesRatio);
		}

		m_bEnabled = true;
	}
}

void CGlobalIlluminationManager::UpdatePosition(const SRenderingPassInfo& passInfo)
{
	const CCamera& rCamera = passInfo.GetCamera();

	Update(rCamera);      // Check if GI is enabled

	if (!m_bEnabled)
		return;

	const bool bActive = GetFloatCVar(e_GIAmount) * gEnv->p3DEngine->GetGIAmount() > .001f;

	const int nNextFrameID = passInfo.GetMainFrameID() + 1;

	const int nRSMCacheFrames = Cry3DEngineBase::GetCVars()->e_GICache;
	bool bUpdate = true;

	// Compute if we need RSM update (GI cache)
	if (nRSMCacheFrames > 0)
		bUpdate = nNextFrameID - m_nUpdateFrameId >= nRSMCacheFrames;

	for (size_t i = 0; i < m_Cascades.size(); ++i)
	{
		CLPVCascadePtr& pCascade = m_Cascades[i];

		// Preupdate event
		pCascade->GeneralUpdate(bActive);

		// cache for RSM
		if (bUpdate)
		{
			// update light propagation volume
			pCascade->UpdateLPV(rCamera, bActive);
			// update shadow frustum position
			pCascade->UpdateRSMFrustrum(rCamera, bActive);
			m_nUpdateFrameId = nNextFrameID;
		}

		// Postupdate event
		CLPVCascade* pNestedCascade = NULL;

		if (i < m_Cascades.size() - 1)
		{
			pNestedCascade = m_Cascades[i + 1];
		}

		pCascade->PostUpdate(bActive, bUpdate, pNestedCascade);
	}
}

CGlobalIlluminationManager::~CGlobalIlluminationManager()
{
	Cleanup();
}

void CGlobalIlluminationManager::Cleanup()
{
	m_bEnabled = false;
	m_nUpdateFrameId = -100;
	m_Cascades.clear();
}

void CGlobalIlluminationManager::OffsetPosition(const Vec3& delta)
{
#ifdef SEG_WORLD
	for (size_t i = 0; i < m_Cascades.size(); ++i)
	{
		CLPVCascadePtr& pCascade = m_Cascades[i];
		pCascade->OffsetPosition(delta);
	}
#endif
}
///////////////////////////////////////////////////////////////////////////
CLPVCascade::CLPVCascade() : CMultiThreadRefCount()
{
	m_pLPV = NULL;
	m_pReflectiveShadowMap = NULL;
	m_vcGridPos = Vec3(0, 0, 0);
	m_fDistance = 20;
	m_fOffsetDistance = 10;
	m_bIsActive = false;
	m_bGenerateRSM = false;
}

CLPVCascade::~CLPVCascade()
{
	SAFE_DELETE(m_pLPV);
	if (m_pReflectiveShadowMap)
	{
		m_pReflectiveShadowMap->Release(true);
		m_pReflectiveShadowMap = NULL;
	}
}

void CLPVCascade::Init(float fRadius, float fOffset, const CCamera& rCamera)
{
	// update local vars from console ones
	m_fDistance = fRadius;
	m_fOffsetDistance = m_fDistance * fOffset;

	const bool bGlossyReflections = Cry3DEngineBase::GetCVars()->e_GIGlossyReflections != 0;
	const bool bSecondaryOcclusion = Cry3DEngineBase::GetCVars()->e_GISecondaryOcclusion != 0;

	const Vec3 camPos = rCamera.GetPosition() + m_fOffsetDistance * rCamera.GetViewdir();

	// create RSM light
	{
		m_pReflectiveShadowMap = (CLightEntity*)Cry3DEngineBase::Get3DEngine()->CreateLightSource();
		// set up default light properties
		CDLight DynLight;
		DynLight.m_Flags = DLF_PROJECT | DLF_CASTSHADOW_MAPS | DLF_REFLECTIVE_SHADOWMAP;
		DynLight.m_sName = "ColoredShadowMap";
		DynLight.SetLightColor(Cry3DEngineBase::Get3DEngine()->GetSunColor());
		DynLight.SetPosition(camPos + Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized() * fSunDistance);
		const float fMaxBoxDistance = m_fDistance * sqrtf(3.f);
		DynLight.m_fRadius = (float)fSunDistance + fMaxBoxDistance;
		//DynLight.m_fBaseRadius = fRadius;	// this variable will be used for lod selection during shaodow map gen
		DynLight.m_fLightFrustumAngle = RAD2DEG_R(atan2(f64(m_fDistance) * .25, fSunDistance));
		DynLight.m_fProjectorNearPlane = max(1.f, (float)fSunDistance - fMaxBoxDistance - 1000.f);    // -1000 means we have occludes even 1km before
		m_pReflectiveShadowMap->SetRndFlags(ERF_OUTDOORONLY, false);
		m_pReflectiveShadowMap->SetRndFlags(ERF_REGISTER_BY_POSITION, true);
		m_pReflectiveShadowMap->SetLightProperties(DynLight);
		m_pReflectiveShadowMap->m_fWSMaxViewDist = 100000.f;
	}

	// create Light Propagation Volume
	{
		m_pLPV = (CLPVRenderNode*)Cry3DEngineBase::Get3DEngine()->CreateRenderNode(eERType_LightPropagationVolume);
		m_pLPV->EnableSpecular(bGlossyReflections);
		m_pLPV->SetRndFlags(ERF_RENDER_ALWAYS, true);
		if (m_pLPV->m_pRE)
		{
			m_pLPV->m_pRE->SetNewFlags(CRELightPropagationVolume::efGIVolume);
			m_pLPV->m_pRE->AttachLightSource(m_pReflectiveShadowMap);

			if (bSecondaryOcclusion)
			{
				m_pLPV->m_pRE->SetNewFlags(m_pLPV->m_pRE->GetFlags() | CRELightPropagationVolume::efHasOcclusion);
			}
		}
		else
			assert(0);

		m_pLPV->SetDensity(CRELightPropagationVolume::nMaxGridSize / m_fDistance);
		m_pLPV->SetMatrix(Matrix34::CreateScale(Vec3(m_fDistance)));
	}
}

void CLPVCascade::UpdateLPV(const CCamera& camera, const bool bActive)
{
	const Vec3 camPos = camera.GetPosition() + m_fOffsetDistance * camera.GetViewdir();

	assert(m_pLPV);
	// snap 3D grid pos
	{
		Vec3 gridCellSize = m_pLPV->GetBBox().GetSize();
		gridCellSize.x /= m_pLPV->m_gridDimensions.x;
		gridCellSize.y /= m_pLPV->m_gridDimensions.y;
		gridCellSize.z /= m_pLPV->m_gridDimensions.z;
		const Vec3 deltaPos = m_vcGridPos - camPos;
		if (fabsf(deltaPos.x) > gridCellSize.x || fabsf(deltaPos.y) > gridCellSize.y || fabsf(deltaPos.z) > gridCellSize.z)
		{
			m_vcGridPos = camPos;
			m_vcGridPos.x -= fmodf(camPos.x, gridCellSize.x);
			m_vcGridPos.y -= fmodf(camPos.y, gridCellSize.y);
			m_vcGridPos.z -= fmodf(camPos.z, gridCellSize.z);
		}
	}

	const Vec3 gridCenter = m_pLPV->m_matInv.TransformVector(Vec3(.5f, .5f, .5f));
	m_pLPV->m_matInv.SetTranslation(m_vcGridPos - gridCenter);
	Matrix34 mxNew = m_pLPV->m_matInv.GetInverted();
	mxNew.m03 -= .5f;
	mxNew.m13 -= .5f;
	mxNew.m23 -= .5f;
	mxNew.Invert();
	m_pLPV->UpdateMetrics(mxNew, true);
}

void CLPVCascade::UpdateRSMFrustrum(const CCamera& camera, const bool bActive)
{
	assert(m_pReflectiveShadowMap);

	const Vec3 camPos = camera.GetPosition() + m_fOffsetDistance * camera.GetViewdir();
	m_pReflectiveShadowMap->m_light.SetPosition(camPos + Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized() * fSunDistance);

	const Matrix33 mxSunRot = Matrix33::CreateRotationVDir(-Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized());

	// align view matrix within shadow map texel size
	Vec3 vNewLightOrigin = m_pReflectiveShadowMap->m_light.m_Origin;
	ShadowMapFrustum* pFrust = m_pReflectiveShadowMap->GetShadowFrustum(0);
	if (pFrust) // otherwise we're still not initialized
	{
		assert(pFrust->nTexSize > 0);

		// calculate shadow map pixel size (minimum snapping step)
		const float fKatetSize = float(fSunDistance * tan(DEG2RAD_R(m_pReflectiveShadowMap->m_light.m_fLightFrustumAngle)));
		const int nFinalRSMSize = min((uint32)CRELightPropagationVolume::espMaxInjectRSMSize, (uint32)pFrust->nTexSize);
		float fSnapXY = max(0.00001f, fKatetSize * 2.f / nFinalRSMSize); //texture size should be valid already

		// get rot matrix and transform to projector space (rotate towards z and flip)
		Matrix33 mxNewRot = mxSunRot;
		const Vec3 xaxis = mxNewRot.GetColumn0();
		const Vec3 yaxis = -mxNewRot.GetColumn1();
		const Vec3 zaxis = mxNewRot.GetColumn2();
		mxNewRot.SetRow(0, -xaxis);
		mxNewRot.SetRow(1, -zaxis);
		mxNewRot.SetRow(2, -yaxis);

		// transform (rotate) frustum origin into its own space
		assert(mxNewRot.IsOrthonormal());
		Vec3 vViewSpaceOrigin = mxNewRot.TransformVector(m_pReflectiveShadowMap->m_light.m_Origin);
		// snap it to shadow map pixel
		vViewSpaceOrigin.x -= fmodf(vViewSpaceOrigin.x, fSnapXY);
		vViewSpaceOrigin.y -= fmodf(vViewSpaceOrigin.y, fSnapXY);
		// transform it back to world space
		mxNewRot.Transpose();
		vNewLightOrigin = mxNewRot.TransformVector(vViewSpaceOrigin);
	}

	// set up new snapped matrix into light
	Matrix34 mxNewLight = Matrix34(mxSunRot, vNewLightOrigin);
	const Vec3 diry = mxNewLight.GetColumn(0);
	const Vec3 dirx = mxNewLight.GetColumn(1);
	mxNewLight.SetColumn(0, dirx);
	mxNewLight.SetColumn(1, diry);
	m_pReflectiveShadowMap->m_light.SetMatrix(mxNewLight);
	m_pReflectiveShadowMap->SetMatrix(mxNewLight);
	m_pReflectiveShadowMap->m_light.SetPosition(vNewLightOrigin);
	m_pReflectiveShadowMap->m_light.SetLightColor(Cry3DEngineBase::Get3DEngine()->GetSunColor());

	// update bbox and objects tree
	const float fBoxRadius = m_fDistance * sqrtf(3.f);
	m_pReflectiveShadowMap->SetBBox(AABB(camPos - Vec3(fBoxRadius, fBoxRadius, fBoxRadius),
	                                     camPos + Vec3(fBoxRadius, fBoxRadius, fBoxRadius)));
}

void CLPVCascade::GeneralUpdate(const bool bActive)
{

}

void CLPVCascade::PostUpdate(const bool bActive, const bool bGenerateRSM, const CLPVCascade* pNestedVolume)
{
	// post-update render parameters
	Matrix34 mxOld;
	m_pLPV->GetMatrix(mxOld);
	m_pLPV->UpdateMetrics(mxOld, false);
	if (m_pLPV->m_pRE)
	{
		m_pLPV->m_pRE->m_fDistance = m_fDistance;
		m_pLPV->m_pRE->m_fAmount += (GetFloatCVar(e_GIAmount) - m_pLPV->m_pRE->m_fAmount) * Cry3DEngineBase::GetTimer()->GetFrameTime();
		m_pLPV->m_pRE->Invalidate();
	}

	// add/remove RSM frustum
	if (bGenerateRSM != m_bGenerateRSM)
	{
		if (bGenerateRSM)
			m_pReflectiveShadowMap->m_light.m_Flags &= ~DLF_DISABLED;
		else
			m_pReflectiveShadowMap->m_light.m_Flags |= DLF_DISABLED;
		m_bGenerateRSM = bGenerateRSM;
	}

	// add/remove entities
	if (bActive != m_bIsActive)
	{
		if (bActive)
		{
			m_pReflectiveShadowMap->m_light.m_Flags &= ~DLF_DISABLED;
			m_pReflectiveShadowMap->m_light.m_Flags |= DLF_CASTSHADOW_MAPS;
			Cry3DEngineBase::Get3DEngine()->RegisterEntity(m_pReflectiveShadowMap);
			Cry3DEngineBase::Get3DEngine()->RegisterEntity(m_pLPV);
		}
		else
		{
			m_pReflectiveShadowMap->m_light.m_Flags |= DLF_DISABLED;
			m_pReflectiveShadowMap->m_light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
			Cry3DEngineBase::Get3DEngine()->UnRegisterEntityAsJob(m_pReflectiveShadowMap);
			Cry3DEngineBase::Get3DEngine()->UnRegisterEntityAsJob(m_pLPV);
		}
		m_bIsActive = bActive;
	}
}

void CLPVCascade::OffsetPosition(const Vec3& delta)
{
#ifdef SEG_WORLD
	m_pLPV->SetMatrix(Matrix34::CreateScale(Vec3(m_fDistance)));
	Cry3DEngineBase::Get3DEngine()->UnRegisterEntityAsJob(m_pReflectiveShadowMap);
#endif
}
