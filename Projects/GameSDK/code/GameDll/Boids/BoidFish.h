// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/IScriptSystem.h>
#include <CryAISystem/IAISystem.h>
#include "BoidObject.h"

//////////////////////////////////////////////////////////////////////////
//! Boid object with fish behavior.
//////////////////////////////////////////////////////////////////////////

class CBoidFish : public CBoidObject
{
public:
	CBoidFish( SBoidContext &bc );
	~CBoidFish();

	virtual void Update( float dt,SBoidContext &bc );
	virtual void Kill( const Vec3 &hitPoint,const Vec3 &force );
	virtual void Physicalize( SBoidContext &bc );

protected:
	void SpawnParticleEffect( const Vec3 &pos,SBoidContext &bc,int nEffect );

	float m_dyingTime; // Deisred height this birds want to fly at.
	SmartScriptTable vec_Bubble;

	enum EScriptFunc {
		SPAWN_BUBBLE,
		SPAWN_SPLASH,
	};
	HSCRIPTFUNCTION m_pOnSpawnBubbleFunc;
	HSCRIPTFUNCTION m_pOnSpawnSplashFunc;
};
