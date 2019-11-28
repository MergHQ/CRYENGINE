// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRules.h"

class IGameRulesModuleRMIListener
{
public:
	virtual ~IGameRulesModuleRMIListener() {}

	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params) = 0;
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params) = 0;
	virtual void OnEntityWithTimeRMI(CGameRules::SModuleRMIEntityTimeParams params) = 0;

	virtual void OnSvClientActionRMI(CGameRules::SModuleRMISvClientActionParams params, EntityId fromEid) = 0;
};
