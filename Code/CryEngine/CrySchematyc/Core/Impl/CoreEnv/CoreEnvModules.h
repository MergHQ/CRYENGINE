// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Utils/GUID.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

extern const SGUID g_coreModuleGUID;

void RegisterCoreEnvModules(IEnvRegistrar& registrar);
} // Schematyc
