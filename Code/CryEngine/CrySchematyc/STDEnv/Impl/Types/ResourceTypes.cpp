// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <Schematyc/Types/ResourceTypes.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
void RegisterResourceTypes(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_resourceModuleGUID);
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(GeomFileName, "GeomFileName");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Geometry file name");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(CharacterFileName, "CharacterFileName");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Character file name");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(ParticleEffectName, "ParticleEffectName");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Particle effect name");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(DialogName, "DialogName");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Dialog name");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(ForceFeedbackId, "ForceFeedbackId");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Force feedback identifier");
		scope.Register(pDataType);
	}
	{
		auto pDataType = SCHEMATYC_MAKE_ENV_DATA_TYPE(ActionMapName, "ActionMapName");
		pDataType->SetAuthor(g_szCrytek);
		pDataType->SetDescription("Action map name");
		scope.Register(pDataType);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterResourceTypes)
