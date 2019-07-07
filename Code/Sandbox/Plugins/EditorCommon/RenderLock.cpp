// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "RenderLock.h"

#include <CryCore/Assert/CryAssert.h>
#include <CryThreading/CryAtomics.h>

int CScopedRenderLock::s_renderLock = 0;

CScopedRenderLock::CScopedRenderLock()
{
	int res = CryInterlockedIncrement(alias_cast<volatile int*>(&s_renderLock));
	assert(res > 0);
	m_bOwnsLock = (res == 1);
}

CScopedRenderLock::~CScopedRenderLock()
{
	int res = CryInterlockedDecrement(alias_cast<volatile int*>(&s_renderLock));
	assert(res >= 0);
}

void CScopedRenderLock::Lock()
{
	int res = CryInterlockedIncrement(alias_cast<volatile int*>(&s_renderLock));
	assert(res > 0);
}

void CScopedRenderLock::Unlock()
{
	int res = CryInterlockedDecrement(alias_cast<volatile int*>(&s_renderLock));
	assert(res >= 0);
}


CScopedRenderLock::operator bool()
{
	return m_bOwnsLock;
}
