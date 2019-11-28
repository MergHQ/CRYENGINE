// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityBasicTypes.h>

struct IActor;

class IGameRulesPlayerSetupModule
{
public:
	virtual ~IGameRulesPlayerSetupModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;

	virtual void OnClientConnect(int channelId, bool isReset, const char* playerName, bool isSpectator) = 0;
	virtual void OnPlayerRevived(EntityId playerId) = 0;
	virtual void OnActorJoinedFromSpectate(IActor* pActor, int channelId) = 0;

	virtual void SvOnStartNewRound(bool isReset) = 0;
};
