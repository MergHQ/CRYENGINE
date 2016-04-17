// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractMeshElement.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"
#include <CryRenderer/VertexFormats.h>

void AbstractMeshElement::ApplyVert()
{
	if (!GetVertCount())
		return;

	TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(GetVertBufData(), GetVertCount(), 0);
	gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
}

void AbstractMeshElement::ApplyIndices()
{
	if (!GetIndexCount())
		return;

	TempDynIB16::CreateFillAndBind(GetIndexBufData(), GetIndexCount());
}

void AbstractMeshElement::ApplyMesh()
{
	ApplyVert();
	ApplyIndices();
}

void AbstractMeshElement::DrawMeshTriList()
{
	int nVertexBufferCount = GetVertCount();
	int nIndexBufferCount = GetIndexCount();
	if (nVertexBufferCount <= 0 || nIndexBufferCount <= 0)
		return;
	gcpRendD3D->FX_Commit();
	gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nVertexBufferCount, 0, nIndexBufferCount);
}

void AbstractMeshElement::DrawMeshWireframe()
{
	int nVertexBufferCount = GetVertCount();
	int nIndexBufferCount = GetIndexCount();
	if (nVertexBufferCount <= 0 || nIndexBufferCount <= 0)
		return;

	const int32 nState = gRenDev->m_RP.m_CurState;
	gcpRendD3D->FX_SetState(nState | GS_WIREFRAME);

	gcpRendD3D->FX_Commit();
	gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nVertexBufferCount, 0, nIndexBufferCount);

	gcpRendD3D->FX_SetState(nState);
}
