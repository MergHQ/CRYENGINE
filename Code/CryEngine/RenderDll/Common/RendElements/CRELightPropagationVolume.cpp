// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryRenderer/RenderElements/CRELightPropagationVolume.h>
#include <CryEntitySystem/IEntityRenderState.h> // <> required for Interfuscator

#include "../../XRenderD3D9/D3DLightPropagationVolume.h"

#include "../../XRenderD3D9/DriverD3D.h"

//////////////////////////////////////////////////////////////////////////
CRELightPropagationVolume::CRELightPropagationVolume()
	: CRendElementBase()
	, m_bIsRenderable(false)
	, m_bIsUpToDate(false)
	, m_bNeedPropagate(false)
	, m_bNeedClear(true)
	, m_bHasSpecular(false)
	, m_pParent(NULL)
	, m_nId(-1)
	, m_nGridFlags(0)
	, m_fDistance(100.f)
	, m_fAmount(0.f)
	, m_pAttachedLightSource(NULL)
	, m_nUpdateFrameID(-1)
{
	mfSetType(eDATA_LightPropagationVolume);
	mfUpdateFlags(FCEF_TRANSFORM);
	m_pRT[0] = NULL;
	m_pRT[1] = NULL;
	m_pRT[2] = NULL;

	m_pVolumeTextures[0] = NULL;
	m_pVolumeTextures[1] = NULL;
	m_pVolumeTextures[2] = NULL;

	m_pOcclusionTexture = NULL;

	LPVManager.RegisterLPV(this);

}

CRELightPropagationVolume::~CRELightPropagationVolume()
{
	Cleanup();

	LPVManager.UnregisterLPV(this);

}

void CRELightPropagationVolume::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

void CRELightPropagationVolume::Cleanup()
{

	// Release cached RTs
	if (m_pRT[0] == LPVManager.GetCachedRTs().GetRT(0) && LPVManager.GetCachedRTs().GetRT(0) && LPVManager.GetCachedRTs().IsAcquired())
		LPVManager.GetCachedRTs().Release();

	if (m_pVolumeTextures[0] == m_pRT[0]) m_pVolumeTextures[0] = NULL;
	if (m_pVolumeTextures[1] == m_pRT[1]) m_pVolumeTextures[1] = NULL;
	if (m_pVolumeTextures[2] == m_pRT[2]) m_pVolumeTextures[2] = NULL;
	SAFE_RELEASE(m_pRT[0]);
	SAFE_RELEASE(m_pRT[1]);
	SAFE_RELEASE(m_pRT[2]);

	SAFE_RELEASE(m_pVolumeTextures[0]);
	SAFE_RELEASE(m_pVolumeTextures[1]);
	SAFE_RELEASE(m_pVolumeTextures[2]);

	SAFE_RELEASE(m_pOcclusionTexture);
}

void CRELightPropagationVolume::EnableSpecular(const bool bEnabled)
{
	m_bHasSpecular = bEnabled;
}

void SReflectiveShadowMap::Release()
{
	//STATIC UNINITIALISATION. This can be called after gEnv->pRenderer has been released
	if (gEnv && gEnv->pRenderer)
	{
		SAFE_RELEASE(pDepthRT);
		SAFE_RELEASE(pNormalsRT);
		SAFE_RELEASE(pFluxRT);
	}
}

CRELightPropagationVolume::Settings::Settings()
{
	m_pos = Vec3(0, 0, 0);
	m_bbox = AABB(AABB::RESET);
	m_mat.SetIdentity();
	m_matInv.SetIdentity();
	m_nGridWidth = 0;
	m_nGridHeight = 0;
	m_nGridDepth = 0;
	m_nNumIterations = 0;
	m_gridDimensions = Vec4(0, 0, 0, 0);
	m_invGridDimensions = Vec4(0, 0, 0, 0);
}
