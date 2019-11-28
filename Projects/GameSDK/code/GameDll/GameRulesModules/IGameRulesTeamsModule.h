// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryNetwork/SerializeFwd.h>
#include "IGameObject.h"

class IGameRulesTeamsModule
{
public:
	virtual ~IGameRulesTeamsModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;
	virtual void	PostInit() = 0;

	virtual void	RequestChangeTeam(EntityId playerId, int teamId, bool onlyIfUnassigned) = 0;

	virtual int		GetAutoAssignTeamId(EntityId playerId)	= 0;

	virtual bool	CanTeamModifyWeapons(int teamId)				= 0;
};
