// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   AlembicConvertor.h
//  Created:     20/7/2012 by Axel Gneiting
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "IConvertor.h"

class ICryXML;

class AlembicConvertor : public IConvertor
{
public:
	AlembicConvertor(ICryXML *pXMLParser, IPakSystem* pPakSystem);
	virtual ~AlembicConvertor();

	// IConvertor methods.
	virtual void Release();
	virtual void DeInit() {}
	virtual ICompiler *CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual const char *GetExt(int index) const;

private:
	int m_refCount;
	ICryXML *m_pXMLParser;
	IPakSystem *m_pPakSystem;
};