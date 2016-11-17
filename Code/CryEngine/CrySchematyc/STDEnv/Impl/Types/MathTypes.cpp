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
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(float, "Float");
		pDataType->SetAuthor("Paul Slinger");
		pDataType->SetDescription("32bit floating point number");
		pDataType->SetDefaultValue(0.0f);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(Vec2, "Vector2");
		pDataType->SetAuthor("Paul Slinger");
		pDataType->SetDescription("2d vector");
		pDataType->SetDefaultValue(ZERO);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(Vec3, "Vector3");
		pDataType->SetAuthor("Paul Slinger");
		pDataType->SetDescription("3d vector");
		pDataType->SetDefaultValue(ZERO);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(CRotation, "Rotation");
		pDataType->SetAuthor("Paul Slinger");
		pDataType->SetDescription("Quaternion rotation");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(CTransform, "Transform");
		pDataType->SetAuthor("Paul Slinger");
		pDataType->SetDescription("Transform");
		scope.Register(pDataType);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterMathTypes)
