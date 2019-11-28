// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct HitInfo;

class IGameRulesKillListener
{
public:
	virtual ~IGameRulesKillListener() {}

	virtual void OnEntityKilledEarly(const HitInfo &hitInfo) = 0;
	virtual void OnEntityKilled(const HitInfo &hitInfo) = 0;
};
