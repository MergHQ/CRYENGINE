// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameRulesSystem.h>
#include "GameRulesTypes.h"

class IGameRulesScoringModule
{
public:
	virtual ~IGameRulesScoringModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual TGameRulesScoreInt GetPlayerPointsByType(EGRST pointsType) const = 0;
	virtual TGameRulesScoreInt GetPlayerXPByType(EGRST pointsType) const = 0;
	virtual TGameRulesScoreInt GetTeamPointsByType(EGRST pointsType) const = 0;
	virtual void	DoScoringForDeath(IActor *pActor, EntityId shooterId, int damage, int material, int hit_type) = 0;
	virtual void	OnPlayerScoringEvent(EntityId playerId, EGRST type) = 0;
	virtual void  OnPlayerScoringEventWithInfo(EntityId playerId, SGameRulesScoreInfo* scoreInfo) = 0;
	virtual void  OnPlayerScoringEventToAllTeamWithInfo(const int teamId, SGameRulesScoreInfo* scoreInfo) = 0;
	virtual void	OnTeamScoringEvent(int teamId, EGRST pointsType) = 0;
	virtual void	SvResetTeamScore(int teamId) = 0;
	virtual int		GetStartTeamScore() = 0;
	virtual int		GetMaxTeamScore() = 0;
	virtual void  SetAttackingTeamLost() = 0;
	virtual bool	AttackingTeamWonAllRounds() = 0;
	virtual TGameRulesScoreInt GetDeathScoringModifier() = 0;
	virtual void SvSetDeathScoringModifier(TGameRulesScoreInt inModifier) = 0;
};
