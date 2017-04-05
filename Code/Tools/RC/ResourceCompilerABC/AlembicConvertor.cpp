// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   AlembicConvertor.cpp
//  Created:     20/7/2012 by Axel Gneiting
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AlembicConvertor.h"
#include "AlembicCompiler.h"

AlembicConvertor::AlembicConvertor(ICryXML *pXMLParser, IPakSystem* pPakSystem)
	: m_refCount(1), m_pXMLParser(pXMLParser), m_pPakSystem(pPakSystem)
{
}

AlembicConvertor::~AlembicConvertor()
{
}

void AlembicConvertor::Release()
{
	if (--m_refCount <= 0)
	{
		delete this;
	}
}

ICompiler *AlembicConvertor::CreateCompiler()
{
	return new AlembicCompiler(m_pXMLParser);
}

bool AlembicConvertor::SupportsMultithreading() const
{
	return false;
}

const char *AlembicConvertor::GetExt(int index) const
{
	switch (index)
	{
	case 0:
		return "abc";
	default:
		return 0;
	}
}