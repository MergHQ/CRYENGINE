// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "STDModules.h"

#include "AutoRegister.h"

namespace Schematyc
{
const SGUID g_stdModuleGUID = "a929e72e-d3a0-46a0-949b-17d22162ec33"_schematyc_guid;
const SGUID g_logModuleGUID = "a2cbae18-2114-4c0f-8fc0-58988affca7e"_schematyc_guid;
const SGUID g_mathModuleGUID = "f2800127-ed71-4d1e-a178-87aac8ee74ed"_schematyc_guid;
const SGUID g_resourceModuleGUID = "4dbeaab8-e902-4db9-b930-09228a172d89"_schematyc_guid;

void RegisterSTDModules(IEnvRegistrar& registrar)
{
	registrar.RootScope().Register(SCHEMATYC_MAKE_ENV_MODULE(g_stdModuleGUID, "Standard"));
	{
		CEnvRegistrationScope scope = registrar.Scope(g_stdModuleGUID);
		scope.Register(SCHEMATYC_MAKE_ENV_MODULE(g_logModuleGUID, "Log"));
		scope.Register(SCHEMATYC_MAKE_ENV_MODULE(g_mathModuleGUID, "Math"));
		scope.Register(SCHEMATYC_MAKE_ENV_MODULE(g_resourceModuleGUID, "Resource"));
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterSTDModules)
