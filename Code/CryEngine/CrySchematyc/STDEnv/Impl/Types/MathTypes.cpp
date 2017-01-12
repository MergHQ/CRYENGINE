// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <Schematyc/Types/MathTypes.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{

void RegisterMathTypes(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_mathModuleGUID);
	{
		scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(float));
		scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(Vec2));
		scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(Vec3));
		scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(CRotation));
		scope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(CTransform));
	}
}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterMathTypes)
