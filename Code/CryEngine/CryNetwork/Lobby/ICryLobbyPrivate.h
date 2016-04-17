// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef ICRYLOBBY_PRIVATE_H
#define ICRYLOBBY_PRIVATE_H

#pragma once

#include <CryLobby/ICryLobby.h>
#include "LobbyCVars.h"

struct ICryLobbyPrivate : public ICryLobby
{
	virtual void         Tick(bool flush) = 0;
	virtual CLobbyCVars& GetCVars() = 0;
};

#endif//ICRYLOBBY_PRIVATE_H
