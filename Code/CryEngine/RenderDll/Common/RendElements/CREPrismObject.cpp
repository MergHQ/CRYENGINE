// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

	#include <CryRenderer/RenderElements/CREPrismObject.h>
	#include <CryEntitySystem/IEntityRenderState.h> // <> required for Interfuscator

CREPrismObject::CREPrismObject()
	: CRendElementBase(), m_center(0, 0, 0)
{
	mfSetType(eDATA_PrismObject);
	mfUpdateFlags(FCEF_TRANSFORM);
}

void CREPrismObject::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

#endif // EXCLUDE_DOCUMENTATION_PURPOSE
