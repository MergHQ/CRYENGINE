// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CoreEnvModules.h"

#include <Schematyc/Env/IEnvRegistrar.h>
#include <Schematyc/Env/Elements/EnvModule.h>

namespace Schematyc
{
const SGUID g_coreModuleGUID = "5b78293e-0f99-42ae-9a51-8fd0dac5ff1a"_schematyc_guid;

void RegisterCoreEnvModules(IEnvRegistrar& registrar)
{
	registrar.RootScope().Register(SCHEMATYC_MAKE_ENV_MODULE(g_coreModuleGUID, "Core"));
}
} // Schematyc
