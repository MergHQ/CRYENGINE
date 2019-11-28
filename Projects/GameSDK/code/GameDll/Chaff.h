// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Projectile.h"
#include <CryCore/Containers/VectorSet.h>

#define MAX_SPAWNED_CHAFFS	16

class CChaff : public CProjectile
{
public:
	CChaff();
	virtual ~CChaff();

	virtual void HandleEvent(const SGameObjectEvent &);

	virtual Vec3 GetPosition(void);

	static VectorSet<CChaff*> s_chaffs;
};
