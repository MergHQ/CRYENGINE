// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

//=========================================================================================

#include "../CryFont/FBitmap.h"

bool CD3D9Renderer::FontUpdateTexture(int nTexId, int nX, int nY, int USize, int VSize, const byte* pSrcData)
{
	CTexture* tp = CTexture::GetByID(nTexId);
	assert(tp);
	if (!tp)
		return false;

	// Always non-blocking
	tp->UpdateTextureRegion((const byte*)pSrcData, nX, nY, 0, USize, VSize, 1, eTF_A8);

	return true;
}

bool CD3D9Renderer::FontUploadTexture(class CFBitmap* pBmp, ETEX_Format eSrcFormat)
{
	if (!pBmp)
		return false;

	CRY_ASSERT(eSrcFormat == eTF_R8G8B8A8);
	unsigned int* pDstData = nullptr;
	pBmp->Get32Bpp(&pDstData);

	char szName[128];
	cry_sprintf(szName, "$AutoFont_%d", m_TexGenID++);

	// Indicate non-blocking
	const int iFlags = FT_TEX_FONT | FT_TAKEOVER_DATA_POINTER | FT_DONT_STREAM | FT_DONT_RELEASE;
	CTexture* tp = CTexture::GetOrCreate2DTexture(szName, pBmp->GetWidth(), pBmp->GetHeight(), 1, iFlags, (const byte*)pDstData, eSrcFormat);

	pBmp->SetRenderData((void*)tp);

	return true;
}

int CD3D9Renderer::FontCreateTexture(int Width, int Height, const byte* pSrcData, ETEX_Format eSrcFormat, bool genMips)
{
	if (!pSrcData)
		return -1;

	CRY_ASSERT(eSrcFormat == eTF_A8 || eSrcFormat == eTF_R8G8B8A8);
	byte* pDstData = new byte[Width * Height * (eSrcFormat == eTF_A8 ? 1 : 4)];
	if (!pDstData)
		return false;

	memcpy(pDstData, pSrcData, Width * Height * (eSrcFormat == eTF_A8 ? 1 : 4));

	char szName[128];
	cry_sprintf(szName, "$AutoFont_%d", m_TexGenID++);

	// Indicate non-blocking
	const int iFlags = FT_TEX_FONT | FT_TAKEOVER_DATA_POINTER | FT_DONT_STREAM | FT_DONT_RELEASE | (genMips * FT_FORCE_MIPS);
	CTexture* tp = CTexture::GetOrCreate2DTexture(szName, Width, Height, 1, iFlags, (const byte*)pDstData, eSrcFormat);

	return tp->GetID();
}

void CD3D9Renderer::FontReleaseTexture(class CFBitmap* pBmp)
{
	if (!pBmp)
	{
		return;
	}

	CTexture* tp = (CTexture*)pBmp->GetRenderData();

	SAFE_RELEASE(tp);
}
