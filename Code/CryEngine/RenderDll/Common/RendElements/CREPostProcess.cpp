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

void CREPostProcess::Reset(bool bOnSpecChange)
{
	if (PostEffectMgr())
		PostEffectMgr()->Reset(bOnSpecChange);
}

int CREPostProcess::mfSetParameter(const char* pszParam, float fValue, bool bForceValue) const
{
	assert((pszParam) && "mfSetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return 0;
	}

	pParam->SetParam(fValue, bForceValue);

	return 1;
}

void CREPostProcess::mfGetParameter(const char* pszParam, float& fValue) const
{
	assert((pszParam) && "mfGetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return;
	}

	fValue = pParam->GetParam();
}

int CREPostProcess::mfSetParameterVec4(const char* pszParam, const Vec4& pValue, bool bForceValue) const
{
	assert((pszParam) && "mfSetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return 0;
	}

	pParam->SetParamVec4(pValue, bForceValue);

	return 1;
}

void CREPostProcess::mfGetParameterVec4(const char* pszParam, Vec4& pValue) const
{
	assert((pszParam) && "mfGetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return;
	}

	pValue = pParam->GetParamVec4();
}

int CREPostProcess::mfSetParameterString(const char* pszParam, const char* pszArg) const
{
	assert((pszParam || pszArg) && "mfSetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return 0;
	}

	pParam->SetParamString(pszArg);
	return 1;
}

void CREPostProcess::mfGetParameterString(const char* pszParam, const char*& pszArg) const
{
	assert((pszParam) && "mfGetParameter: null parameter");

	CEffectParam* pParam = PostEffectMgr()->GetByName(pszParam);
	if (!pParam)
	{
		return;
	}

	pszArg = pParam->GetParamString();
}

int32 CREPostProcess::mfGetPostEffectID(const char* pPostEffectName) const
{
	assert(pPostEffectName && "mfGetParameter: null parameter");

	return PostEffectMgr()->GetEffectID(pPostEffectName);
}
