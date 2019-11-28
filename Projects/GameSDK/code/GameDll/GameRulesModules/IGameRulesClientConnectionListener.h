// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IGameRulesClientConnectionListener
{
public:
	virtual ~IGameRulesClientConnectionListener() {}

	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) = 0;
	virtual void OnClientDisconnect(int channelId, EntityId playerId) = 0;
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) = 0;
	virtual void OnOwnClientEnteredGame() = 0;
};
