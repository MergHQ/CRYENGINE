// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameRulesSystem.h>

class IGameRulesActorActionListener
{
public:
	virtual ~IGameRulesActorActionListener() {}

	virtual void OnAction(const ActionId& actionId, int activationMode, float value) = 0;
};
