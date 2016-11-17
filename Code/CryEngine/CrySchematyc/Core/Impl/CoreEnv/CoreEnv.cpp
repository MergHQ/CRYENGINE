// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CoreEnv.h"

#include <Schematyc/Env/IEnvRegistrar.h>

#include "CoreEnvModules.h"
#include "CoreEnvSignals.h"

namespace Schematyc
{
void RegisterCoreEnvPackage(IEnvRegistrar& registrar)
{
	RegisterCoreEnvModules(registrar);
	RegisterCoreEnvSignals(registrar);
}
} // Schematyc
