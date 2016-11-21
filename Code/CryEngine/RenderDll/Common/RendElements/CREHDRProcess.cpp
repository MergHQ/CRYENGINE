// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "XRenderD3D9/DriverD3D.h"

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

bool CREHDRProcess::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd = gcpRendD3D;
	assert(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR || rd->m_RP.m_CurState & GS_WIREFRAME);
	if (!(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR))
		return false;

	rd->FX_HDRPostProcessing();
	return true;
}