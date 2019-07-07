// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Flock.h"
struct IEntity;

//////////////////////////////////////////////////////////////////////////
class CFishFlock : public CFlock
{
public:
	CFishFlock(IEntity* pEntity) : CFlock(pEntity, EFLOCK_FISH) { m_boidEntityName = "FishBoid"; m_boidDefaultAnimName = "swim_loop"; }
	virtual void CreateBoids(SBoidsCreateContext& ctx);
};
