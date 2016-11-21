// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <Schematyc/Types/BasicTypes.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
void RegisterBasicTypes(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_stdModuleGUID);
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(bool, "Bool");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Boolean");
		pDataType->SetDefaultValue(false);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(int32, "Int32");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Signed 32bit integer");
		pDataType->SetFlags(EEnvDataTypeFlags::Switchable);
		pDataType->SetDefaultValue(0);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(uint32, "UInt32");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Unsigned 32bit integer");
		pDataType->SetFlags(EEnvDataTypeFlags::Switchable);
		pDataType->SetDefaultValue(0);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(CSharedString, "String");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("String");
		pDataType->SetFlags(EEnvDataTypeFlags::Switchable);
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(ObjectId, "ObjectId");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Object id");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(ExplicitEntityId, "EntityId");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Entity id");
		scope.Register(pDataType);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterBasicTypes)
