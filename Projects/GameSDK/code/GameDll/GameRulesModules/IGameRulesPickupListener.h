// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityBasicTypes.h>

class IGameRulesPickupListener
{
public:
	virtual ~IGameRulesPickupListener() {}

	virtual void OnItemPickedUp(EntityId itemId, EntityId actorId) = 0;
	virtual void OnItemDropped(EntityId itemId, EntityId actorId) = 0;

	virtual void OnPickupEntityAttached(EntityId entityId, EntityId actorId) = 0;
	virtual void OnPickupEntityDetached(EntityId entityId, EntityId actorId, bool isOnRemove) = 0;
};
