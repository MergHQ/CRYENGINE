// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

#include <CryRenderer/RenderElements/CREPrismObject.h>
#include <CryEntitySystem/IEntityRenderState.h> // <> required for Interfuscator
#include "XRenderD3D9/DriverD3D.h"

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

bool CREPrismObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd(gcpRendD3D);

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (!nPasses)
		return false;

	ef->FXBeginPass(0);

	// commit all render changes
	//	rd->FX_Commit();

	CryStackAllocWithSizeVector(SVF_P3F_C4B_T2F, 4, pScreenQuad, CDeviceBufferManager::AlignBufferSizeForStreaming);

	pScreenQuad[0] = { Vec3(0, 0, 0), { { 0 } }, Vec2(0, 0) };
	pScreenQuad[1] = { Vec3(0, 1, 0), { { 0 } }, Vec2(0, 1) };
	pScreenQuad[2] = { Vec3(1, 0, 0), { { 0 } }, Vec2(1, 0) };
	pScreenQuad[3] = { Vec3(1, 1, 0), { { 0 } }, Vec2(1, 1) };

	pScreenQuad[0].xyz = Vec3(0, 0, 0);
	pScreenQuad[1].xyz = Vec3(0, 1, 0);
	pScreenQuad[2].xyz = Vec3(1, 0, 0);
	pScreenQuad[3].xyz = Vec3(1, 1, 0);

	CVertexBuffer strip(pScreenQuad, eVF_P3F_C4B_T2F);
	gcpRendD3D->DrawPrimitivesInternal(&strip, 4, eptTriangleStrip);

	// count rendered polygons
	//	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += 2;
	//	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;

	ef->FXEndPass();
	ef->FXEnd();

	return true;
}

#endif // EXCLUDE_DOCUMENTATION_PURPOSE
