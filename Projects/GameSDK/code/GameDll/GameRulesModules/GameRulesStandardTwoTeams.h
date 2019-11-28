// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IGameRulesTeamsModule.h"
#include <CryString/CryFixedString.h>

class CGameRules;

class CGameRulesStandardTwoTeams : public IGameRulesTeamsModule
{
public:
	CGameRulesStandardTwoTeams();
	virtual ~CGameRulesStandardTwoTeams();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();

	virtual void RequestChangeTeam(EntityId playerId, int teamId, bool onlyIfUnassigned);

	virtual int GetAutoAssignTeamId(EntityId playerId);

	virtual bool	CanTeamModifyWeapons(int teamId);

protected:
	void DoTeamChange(EntityId playerId, int teamId);

	CGameRules *m_pGameRules;
	bool m_bCanTeamModifyWeapons[2];
};
