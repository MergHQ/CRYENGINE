// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Projectile.h"

class CRocket : public CProjectile
{
public:
	CRocket();
	virtual ~CRocket();


	// CProjectile
	virtual void HandleEvent(const SGameObjectEvent &);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);

	//virtual void Serialize(TSerialize ser, unsigned aspects);
	// ~CProjectile

protected:
	void AutoDropOwnerWeapon();
	void EnableTrail();
	void DisableTrail();
	virtual bool ShouldCollisionsDamageTarget() const;
	virtual void ProcessEvent(const SEntityEvent& event);

	Vec3			m_launchLoc;
	bool			m_detonatorFired;
};
