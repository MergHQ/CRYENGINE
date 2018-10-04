// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/IScriptSystem.h>
#include <CryAISystem/IAISystem.h>
#include "Flock.h"

//////////////////////////////////////////////////////////////////////////
class CFishFlock : public CFlock
{
public:
	CFishFlock(IEntity* pEntity) : CFlock(pEntity, EFLOCK_FISH) { m_boidEntityName = "FishBoid"; m_boidDefaultAnimName = "swim_loop"; }
	virtual void CreateBoids(SBoidsCreateContext& ctx);
};
