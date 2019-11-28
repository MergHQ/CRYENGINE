// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameRulesSystem.h>
#include "GameRules.h"

class CWeapon;

class IGameRulesDamageHandlingModule
{
public:

	enum EGameEvents
	{
		eGameEvent_LocalPlayerEnteredMercyTime = 0,
		eGameEvent_TrainRideStarts,
		eGameEvent_TrainRideEnds
	};

	virtual ~IGameRulesDamageHandlingModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;
	virtual void Update(float frameTime) = 0;

	virtual bool SvOnHit(const HitInfo &hitInfo) = 0;
	virtual bool SvOnHitScaled(const HitInfo &hitInfo) = 0;
	virtual void SvOnExplosion(const ExplosionInfo &explosionInfo, const CGameRules::TExplosionAffectedEntities& affectedEntities) = 0;
	virtual void SvOnCollision(const IEntity *entity, const CGameRules::SCollisionHitInfo& colHitInfo) = 0;

	virtual void ClProcessHit(Vec3 dir, EntityId shooterId, EntityId weaponId, float damage, uint16 projectileClassId, uint8 hitTypeId) = 0;

	virtual bool AllowHitIndicatorForType(int hitTypeId) = 0;

	virtual void MakeMovementVisibleToAIForEntityClass(const IEntityClass* pEntityClass) = 0;

	virtual void OnGameEvent(const EGameEvents& gameEvent) = 0;

	enum ePID_flags
	{
		PID_None = 0,
		PID_Headshot = BIT(1),
	};
};