// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IGameObject.h"
#include <CryNetwork/SerializeFwd.h>

class IGameRulesEntityObjective
{
public:
	virtual ~IGameRulesEntityObjective() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void Update(float frameTime) = 0;

	virtual void OnStartGame() = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual bool IsComplete(int teamId) = 0;

	virtual void AddEntityId(int type, EntityId entityId, int index, bool isNewEntity, const CTimeValue &timeAdded) = 0;
	virtual void RemoveEntityId(int type, EntityId entityId) = 0;
	virtual void ClearEntities(int type) = 0;
	virtual bool IsEntityFinished(int type, int index) = 0;
	virtual bool CanRemoveEntity(int type, int index) = 0;

	virtual void OnHostMigration(bool becomeServer) = 0;		// Host migration

	virtual void OnTimeTillRandomChangeUpdated(int type, float fPercLiveSpan) = 0;

	virtual bool IsPlayerEntityUsingObjective(EntityId playerId) = 0;
};
