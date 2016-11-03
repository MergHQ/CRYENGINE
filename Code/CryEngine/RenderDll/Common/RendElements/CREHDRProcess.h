// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __CREHDRPROCESS_H__
#define __CREHDRPROCESS_H__

// screen processing render element
class CREHDRProcess : public CRenderElement
{
	friend class CD3D9Renderer;
	friend class CGLRenderer;

public:

	// constructor/destructor
	CREHDRProcess();

	virtual ~CREHDRProcess();

	// prepare screen processing
	virtual void mfPrepare(bool bCheckOverflow);
	// render screen processing
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	// begin screen processing
	virtual void mfActivate(int iProcess);
	// reset
	virtual void mfReset(void);

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif
