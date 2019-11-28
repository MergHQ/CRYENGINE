// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IGameRulesRevivedListener
{
public:
	virtual ~IGameRulesRevivedListener() {}

	virtual void EntityRevived(EntityId entityId) = 0;
};
