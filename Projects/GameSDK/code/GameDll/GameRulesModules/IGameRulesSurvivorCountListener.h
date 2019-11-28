// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IGameRulesSurvivorCountListener
{
public:
	virtual ~IGameRulesSurvivorCountListener() {}

	virtual void SvSurvivorCountRefresh(int count, const EntityId survivors[], int numKills) = 0;
};
