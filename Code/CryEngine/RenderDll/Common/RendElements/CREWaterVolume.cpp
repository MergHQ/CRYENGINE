// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREWaterVolume.h>

CREWaterVolume::CREWaterVolume()
	: CRendElementBase()
	, m_pParams(0)
	, m_pOceanParams(0)
	, m_drawWaterSurface(false)
	, m_drawFastPath(false)
{
	mfSetType(eDATA_WaterVolume);
	mfUpdateFlags(FCEF_TRANSFORM);
}

CREWaterVolume::~CREWaterVolume()
{
}

void CREWaterVolume::mfGetPlane(Plane& pl)
{
	pl = m_pParams->m_fogPlane;
	pl.d = -pl.d;
}

void CREWaterVolume::mfCenter(Vec3& vCenter, CRenderObject* pObj)
{
	vCenter = m_pParams->m_center;
	if (pObj)
		vCenter += pObj->GetTranslation();
}

void CREWaterVolume::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}
