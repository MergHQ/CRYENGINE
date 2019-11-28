// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IItemSystem.h>
#include "Weapon.h"


class CBinocular :	public CWeapon
{
	typedef CWeapon BaseClass;

public:

	CBinocular();
	virtual ~CBinocular();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	
	virtual bool OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void Select(bool select);
	virtual void UpdateFPView(float frameTime);
	virtual bool AllowZoomToggle() { return false; }

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
	virtual bool CanModify() const;
	virtual bool CanFire() const;
	virtual void StartFire();
	virtual void OnZoomIn();
	virtual void OnZoomOut();

	virtual bool AllowInteraction(EntityId interactionEntity, EInteractionType interactionType);

protected:
	
	virtual bool ShouldDoPostSerializeReset() const;

private:

	bool ShouldUseSoundAttenuation(const CActor& ownerActor) const;
	void SwitchSoundAttenuation(const CActor& ownerActor, const float coneInRadians) const;
	void UpdateSoundAttenuation(const CActor& ownerActor) const;

	bool OnActionChangeZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool TrumpAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
};
