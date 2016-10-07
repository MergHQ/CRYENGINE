// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoException.h>
#include <mono/metadata/object.h>

class CMonoException : public IMonoException
{
public:
	CMonoException(MonoException* pException);

	// IMonoException
	virtual void Throw() override;
	// ~IMonoException

protected:
	MonoException* m_pException;
};