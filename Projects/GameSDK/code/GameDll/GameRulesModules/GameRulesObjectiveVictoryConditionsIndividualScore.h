// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRulesObjectiveVictoryConditionsTeam.h"

class CGameRules;

class CGameRulesObjectiveVictoryConditionsIndividualScore : public CGameRulesObjectiveVictoryConditionsTeam
{
public:
	virtual void	OnEndGame(int teamId, EGameOverReason reason, ESVC_DrawResolution winningResolution=ESVC_DrawResolution_invalid, EntityId killedEntity=0, EntityId shooterEntity=0);
};
