// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>
#include "GameRulesModules/GameRulesModulesRegistration.h"

class IGameRulesModulesManager
{
public:
	virtual ~IGameRulesModulesManager() {}

// Register as a factory for each module type
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) DECLARE_GAMEOBJECT_FACTORY(type)
	GAMERULES_MODULE_TYPES_LIST
#undef GAMERULES_MODULE_LIST_FUNC

	virtual void Init() = 0;

	virtual int GetRulesCount() = 0;

	virtual const char* GetRules(int index) = 0;
};
