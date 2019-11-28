// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryLobby/ICryLobbyUI.h>

class IGameLobbyEventListener
{
public:
	virtual ~IGameLobbyEventListener() {}

	virtual void InsertedUser(CryUserID userId, const char *userName) = 0;
	virtual void SessionChanged(const CrySessionHandle inOldSession, const CrySessionHandle inNewSession) = 0;
};
