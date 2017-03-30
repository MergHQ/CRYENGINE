// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoException.h"

#include "MonoRuntime.h"

CMonoException::CMonoException(MonoException* pException)
	: m_pException(pException)
{
	CRY_ASSERT(m_pException);
}

void CMonoException::Throw()
{
	GetMonoRuntime()->HandleException((MonoObject*)m_pException);

	//mono_raise_exception(m_pException);
}