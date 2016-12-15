// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

typedef CDelegate<void (IEnvRegistrar&)> EnvPackageCallback;

struct IEnvPackage
{
	virtual ~IEnvPackage() {}

	virtual SGUID              GetGUID() const = 0;
	virtual const char*        GetName() const = 0;
	virtual EnvPackageCallback GetCallback() const = 0;
};
} // Schematyc
