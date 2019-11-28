// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryLobby/ICryLobbyUI.h>

class ISquadEventListener
{
public:
	// Local squad events
	enum ESquadEventType
	{
		eSET_CreatedSquad,
		eSET_MigratedSquad,
		eSET_JoinedSquad,
		eSET_LeftSquad,
	};

public:
	virtual ~ISquadEventListener() {}

	virtual void AddedSquaddie(CryUserID userId) = 0;
	virtual void RemovedSquaddie(CryUserID userId) = 0;
	virtual void UpdatedSquaddie(CryUserID userId) = 0;
	virtual void SquadLeaderChanged(CryUserID userId) = 0;

	virtual void SquadEvent(ISquadEventListener::ESquadEventType eventType) = 0;
};
