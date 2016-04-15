// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

#ifndef _CREPRISMOBJECT_
	#define _CREPRISMOBJECT_

	#pragma once

class CREPrismObject : public CRendElementBase
{
public:
	CREPrismObject();

	virtual ~CREPrismObject() {}
	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	Vec3 m_center;
};

#endif // #ifndef _CREPRISMOBJECT_

#endif // EXCLUDE_DOCUMENTATION_PURPOSE
