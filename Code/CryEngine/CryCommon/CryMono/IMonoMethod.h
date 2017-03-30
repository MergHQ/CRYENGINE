// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IMonoObject.h"

struct IMonoMethod
{
	virtual ~IMonoMethod() {}

	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject, void** pParameters, bool &bEncounteredException) const = 0;
	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject = nullptr, void** pParameters = nullptr) const = 0;

	inline std::shared_ptr<IMonoObject> InvokeStatic(void** pParameters) const { return Invoke(nullptr, pParameters); }
};