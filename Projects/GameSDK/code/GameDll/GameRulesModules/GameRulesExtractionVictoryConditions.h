// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRulesStandardVictoryConditionsTeam.h"

class CGameRules;

class CGameRulesExtractionVictoryConditions : public CGameRulesStandardVictoryConditionsTeam
{
public:

	// IGameRulesVictoryConditionsModule
	virtual void	Update(float frameTime);
	// ~IGameRulesVictoryConditionsModule

	virtual void  TeamCompletedAnObjective(int teamId);

protected:

	void					UpdateResolutionData(int primaryTeam);
	void					CheckObjectives();
	virtual void	TimeLimitExpired();
};
