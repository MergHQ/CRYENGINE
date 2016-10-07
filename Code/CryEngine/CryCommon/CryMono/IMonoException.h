// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IMonoException
{
	virtual ~IMonoException() {}

	virtual void Throw() = 0;
};