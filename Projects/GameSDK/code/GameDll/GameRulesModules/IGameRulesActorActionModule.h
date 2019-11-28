// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameRulesSystem.h>

struct IActor;

class IGameRulesActorActionModule
{
public:
	virtual ~IGameRulesActorActionModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;

	virtual void OnActorAction(IActor *pActor, const ActionId& actionId, int activationMode, float value) = 0;
};
