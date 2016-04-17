// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/RendElement.h>

void CREOcclusionQuery::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);

	mfSetType(eDATA_OcclusionQuery);
	mfUpdateFlags(FCEF_TRANSFORM);
	//  m_matWSInv.SetIdentity();

	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_FirstVertex = 0;
	gRenDev->m_RP.m_RendNumVerts = 4;

	if (m_pRMBox && m_pRMBox->m_Chunks.size())
	{
		CRenderChunk* pChunk = &m_pRMBox->m_Chunks[0];
		if (pChunk)
		{
			gRenDev->m_RP.m_RendNumIndices = pChunk->nNumIndices;
			gRenDev->m_RP.m_FirstVertex = 0;
			gRenDev->m_RP.m_RendNumVerts = pChunk->nNumVerts;
		}
	}
}
