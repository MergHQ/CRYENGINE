// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SGameRulesPlayerStat;

class IGameRulesPlayerStatsListener
{
public:
	virtual ~IGameRulesPlayerStatsListener() {}

	virtual void ClPlayerStatsNetSerializeReadDeath(const SGameRulesPlayerStat* s, uint16 prevDeathsThisRound, uint8 prevFlags) = 0;

};
