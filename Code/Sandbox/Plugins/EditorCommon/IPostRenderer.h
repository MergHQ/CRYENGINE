// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IPostRenderer
{
public:
	IPostRenderer() : m_refCount(0){}

	virtual void OnPostRender() const = 0;

	void         AddRef()  { m_refCount++; }
	void         Release() { if (--m_refCount <= 0) delete this; }

protected:
	virtual ~IPostRenderer(){}

	int m_refCount;
};
