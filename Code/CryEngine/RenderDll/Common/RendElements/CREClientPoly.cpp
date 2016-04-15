// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   CREClientPoly.cpp : implementation of 3D Client polygons RE.

   Revision history:
* Created by Honitch Andrey

   =============================================================================*/

#include "StdAfx.h"

//===============================================================

CRendElementBase* CREClientPoly::mfCopyConstruct(void)
{
	CREClientPoly* cp = new CREClientPoly;
	*cp = *this;
	return cp;
}

void CREClientPoly::mfPrepare(bool bCheckOverflow)
{
	CRenderer* rd = gRenDev;
	CShader* ef = rd->m_RP.m_pShader;
	int i, n;

	if (rd->m_RP.m_CurVFormat == eVF_P3S_C4B_T2S)
		rd->m_RP.m_CurVFormat = eVF_P3F_C4B_T2F;

	rd->FX_StartMerging();
	CREClientPoly::mRS.NumRendPolys++;

	int savev = rd->m_RP.m_RendNumVerts;
	int savei = rd->m_RP.m_RendNumIndices;

	int nThreadID = rd->m_RP.m_nProcessThreadID;

	int nVerts = 0;
	int nInds = 0;
	if (bCheckOverflow)
		rd->FX_CheckOverflow(m_sNumVerts, m_sNumIndices, this, &nVerts, &nInds);

	if (m_nOffsInd >= (int)(rd->m_RP.m_SysIndexPool[nThreadID].size()))
	{
		assert(0);
		return;
	}

	uint16* pSrcInds = &rd->m_RP.m_SysIndexPool[nThreadID][m_nOffsInd];
	n = rd->m_RP.m_RendNumVerts;
	uint16* dinds = &rd->m_RP.m_RendIndices[gRenDev->m_RP.m_RendNumIndices];
	for (i = 0; i < nInds; i++, dinds++, pSrcInds++)
	{
		*dinds = *pSrcInds + n;
	}
	rd->m_RP.m_RendNumIndices += i;

	UVertStreamPtr ptr = rd->m_RP.m_NextStreamPtr;
	byte* OffsTC, * OffsColor;
	SVF_P3F_C4B_T2F* pSrc = (SVF_P3F_C4B_T2F*)&rd->m_RP.m_SysVertexPool[nThreadID][m_nOffsVert];
	switch (rd->m_RP.m_CurVFormat)
	{
	case eVF_P3F_C4B_T2F:
		OffsTC = rd->m_RP.m_StreamOffsetTC + ptr.PtrB;
		OffsColor = rd->m_RP.m_StreamOffsetColor + ptr.PtrB;
		for (i = 0; i < nVerts; i++, ptr.PtrB += rd->m_RP.m_StreamStride, OffsTC += rd->m_RP.m_StreamStride, OffsColor += rd->m_RP.m_StreamStride)
		{
			*(float*)(ptr.PtrB + 0) = pSrc[i].xyz[0];
			*(float*)(ptr.PtrB + 4) = pSrc[i].xyz[1];
			*(float*)(ptr.PtrB + 8) = pSrc[i].xyz[2];
			*(float*)(OffsTC) = pSrc[i].st[0];
			*(float*)(OffsTC + 4) = pSrc[i].st[1];
			*(uint32*)OffsColor = pSrc[i].color.dcolor;
		}
		break;
	default:
		assert(false);
		break;
	}
	rd->m_RP.m_NextStreamPtr = ptr;

	if (m_nOffsTang >= 0)
	{
		UVertStreamPtr ptrTang = rd->m_RP.m_NextStreamPtrTang;
		SPipTangents* pTangents = (SPipTangents*)&rd->m_RP.m_SysVertexPool[nThreadID][m_nOffsTang];
		for (i = 0; i < nVerts; i++, ptrTang.PtrB += sizeof(SPipTangents))
		{
			*(SPipTangents*)(ptrTang.PtrB) = pTangents[i];
		}
		rd->m_RP.m_NextStreamPtrTang = ptrTang;
	}

	rd->m_RP.m_RendNumVerts += nVerts;

	CREClientPoly::mRS.NumVerts += rd->m_RP.m_RendNumVerts - savev;
	CREClientPoly::mRS.NumIndices += rd->m_RP.m_RendNumIndices - savei;
}

//=======================================================================

SClientPolyStat CREClientPoly::mRS;

void CREClientPoly::mfPrintStat()
{
	/*  char str[1024];

	 * gpCurPrX = 4;
	   cry_sprintf(str, "Num Indices: %i\n", mRS.NumIndices);
	   gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

	 * gpCurPrX = 4;
	   cry_sprintf(str, "Num Verts: %i\n", mRS.NumVerts);
	   gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

	 * gpCurPrX = 4;
	   cry_sprintf(str, "Num Render Client Polys: %i\n", mRS.NumRendPolys);
	   gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

	 * gpCurPrX = 4;
	   cry_sprintf(str, "Num Occluded Client Polys: %i\n", mRS.NumOccPolys);
	   gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

	 * gpCurPrX = 4;
	   gRenDev->mfPrintString ("\nClient Polygons status info:\n", PS_TRANSPARENT | PS_UP);*/
}
