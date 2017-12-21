// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#include <StdAfx.h>
#include "DepthTexture.h"

SDepthTexture::~SDepthTexture()
{
}

void SDepthTexture::Release(bool bReleaseTexture)
{
	if (bReleaseTexture && pTexture)
		pTexture->Release();

	pTarget = nullptr;
	pSurface = nullptr;
	pTexture = nullptr;
}
