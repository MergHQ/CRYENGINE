// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   CREPostProcess.h : Post processing RenderElement common data

   Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

   =============================================================================*/

#ifndef __CREPOSTPROCESS_H__
#define __CREPOSTPROCESS_H__

class CREPostProcess : public CRenderElement
{
	friend class CD3D9Renderer;

public:

	CREPostProcess();
	virtual ~CREPostProcess();

	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	// Use for setting numeric values, vec4 (colors, position, vectors, wtv), strings
	virtual int   mfSetParameter(const char* pszParam, float fValue, bool bForceValue = false) const;
	virtual int   mfSetParameterVec4(const char* pszParam, const Vec4& pValue, bool bForceValue = false) const;
	virtual int   mfSetParameterString(const char* pszParam, const char* pszArg) const;

	virtual void  mfGetParameter(const char* pszParam, float& fValue) const;
	virtual void  mfGetParameterVec4(const char* pszParam, Vec4& pValue) const;
	virtual void  mfGetParameterString(const char* pszParam, const char*& pszArg) const;

	virtual int32 mfGetPostEffectID(const char* pPostEffectName) const;

	//! Reset all post-processing effects.
	virtual void Reset(bool bOnSpecChange = false);
	virtual void mfReset()
	{
		Reset();
	}

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif
