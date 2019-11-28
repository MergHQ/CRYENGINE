// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityBasicTypes.h>

class IGameRulesTeamChangedListener
{
public:
	virtual ~IGameRulesTeamChangedListener() {}

	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId) = 0;
};
