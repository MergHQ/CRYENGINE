// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   CREPostProcess.cpp : Post processing RenderElement

   Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

   =============================================================================*/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CREPostProcess::CREPostProcess()
{
	mfSetType(eDATA_PostProcess);
	mfUpdateFlags(FCEF_TRANSFORM);
}

CREPostProcess::~CREPostProcess()
{
}

void CREPostProcess::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;

	if (CRenderer::CV_r_PostProcessReset)
	{
		CRenderer::CV_r_PostProcessReset = 0;
		mfReset();
	}
}

void CREPostProcess::mfReset()
{
	if (PostEffectMgr())
		PostEffectMgr()->Reset();
}

