// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EntityUtility/EntityEffects.h"
#include "Automatic.h"

class CCharge :
	public CAutomatic
{
public:
	CRY_DECLARE_GTI(CCharge);

	CCharge();
	virtual ~CCharge();

	virtual void Update(float frameTime, uint32 frameId) override;
	virtual void GetMemoryUsage(ICrySizer * s) const override;

	virtual void Activate(bool activate) override;

	virtual void StartFire() override;
	virtual void StopFire() override;

	virtual bool Shoot(bool resetAnimation, bool autoreload, bool isRemote=false) override;

	virtual void ChargeEffect(bool attach);
	virtual void ChargedShoot();
	
protected:

	int							m_charged;
	bool						m_charging;
	float						m_chargeTimer;
	bool						m_autoreload;

	EntityEffects::TAttachedEffectId m_chargeEffectId;
	float						m_chargedEffectTimer;
};
