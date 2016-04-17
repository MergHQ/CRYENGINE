// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// constructor/destructor
CREHDRProcess::CREHDRProcess()
{
	// setup screen process renderer type
	mfSetType(eDATA_HDRProcess);
	mfUpdateFlags(FCEF_TRANSFORM);
}

CREHDRProcess::~CREHDRProcess()
{
};

// prepare screen processing
void CREHDRProcess::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);

	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

void CREHDRProcess::mfReset()
{
}

void CREHDRProcess::mfActivate(int iProcess)
{
}
