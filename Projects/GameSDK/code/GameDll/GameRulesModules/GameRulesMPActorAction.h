// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IGameRulesActorActionModule.h"

class CGameRules;

class CGameRulesMPActorAction : public IGameRulesActorActionModule
{
public:
	CGameRulesMPActorAction();
	virtual ~CGameRulesMPActorAction();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();

	virtual void OnActorAction(IActor *pActor, const ActionId& actionId, int activationMode, float value);
};
