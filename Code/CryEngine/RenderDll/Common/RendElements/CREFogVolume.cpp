// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREFogVolume.h>
#include <CryEntitySystem/IEntityRenderState.h> // <> required for Interfuscator

CREFogVolume::CREFogVolume()
	: CRendElementBase()
	, m_center(0.0f, 0.0f, 0.0f)
	, m_viewerInsideVolume(0)
	, m_stencilRef(0)
	, m_reserved(0)
	, m_localAABB(Vec3(-1, -1, -1), Vec3(1, 1, 1))
	, m_matWSInv()
	, m_fogColor(1, 1, 1, 1)
	, m_globalDensity(1)
	, m_softEdgesLerp(1, 0)
	, m_heightFallOffDirScaled(0, 0, 1)
	, m_heightFallOffBasePoint(0, 0, 0)
	, m_eyePosInWS(0, 0, 0)
	, m_eyePosInOS(0, 0, 0)
	, m_rampParams(0, 0, 0)
	, m_windOffset(0, 0, 0)
	, m_noiseScale(0)
	, m_noiseFreq(1, 1, 1)
	, m_noiseOffset(0)
	, m_noiseElapsedTime(0)
	, m_emission(0, 0, 0)
{
	mfSetType(eDATA_FogVolume);
	mfUpdateFlags(FCEF_TRANSFORM);

	m_matWSInv.SetIdentity();
}

CREFogVolume::~CREFogVolume()
{
}

void CREFogVolume::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}
