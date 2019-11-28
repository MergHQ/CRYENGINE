// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameRulesSystem.h>

class IGameRulesAssistScoringModule
{
public:
	virtual ~IGameRulesAssistScoringModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;

	virtual void	RegisterAssistTarget(EntityId targetId) = 0;
	virtual void	UnregisterAssistTarget(EntityId targetId) = 0;
	virtual void  OnEntityHit(const HitInfo &info, const float tagDuration=-1.f) = 0;
//virtual void  ClAwardAssistKillPoints(EntityId victimId) = 0;
//virtual void	SvDoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type) = 0;
	virtual EntityId	SvGetMostRecentAttacker(EntityId targetId) = 0;
};
