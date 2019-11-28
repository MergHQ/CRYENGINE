// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRulesStandardTwoTeams.h"
#include <CryString/CryFixedString.h>

class CGameRulesGladiatorTeams : public CGameRulesStandardTwoTeams
{
public:
	virtual int		GetAutoAssignTeamId(EntityId playerId) { return 0; }		// Defer team selection for objectives module
	virtual void	RequestChangeTeam(EntityId playerId, int teamId, bool onlyIfUnassigned) {};
};
