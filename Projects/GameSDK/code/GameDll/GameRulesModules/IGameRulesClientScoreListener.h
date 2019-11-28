// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameRulesTypes.h"

class IGameRulesClientScoreListener
{
public:
	virtual ~IGameRulesClientScoreListener() {}

	virtual void ClientScoreEvent(EGameRulesScoreType type, int points, EXPReason inReason, int currentTeamScore) = 0;
};
