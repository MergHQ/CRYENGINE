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
	virtual void mfReset();

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif
