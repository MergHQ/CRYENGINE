// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRulesStandardVictoryConditionsTeam.h"

class CGameRules;

class CGameRulesObjectiveVictoryConditionsTeam : public CGameRulesStandardVictoryConditionsTeam
{
public:

	// IGameRulesVictoryConditionsModule
	virtual void	Update(float frameTime);
	// ~IGameRulesVictoryConditionsModule

protected:

	bool CheckObjectives();
};
