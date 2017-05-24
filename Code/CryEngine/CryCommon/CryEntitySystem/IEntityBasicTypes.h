// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//!< Unique identifier for each entity instance. Don't change the type!
typedef uint32 EntityId;

constexpr EntityId INVALID_ENTITYID = 0;

typedef CryGUID EntityGUID;

enum class EEntitySimulationMode
{
	Idle,   // Not running.
	Game,   // Running in game mode.
	Editor, // Running in editor mode.
	Preview // Running in preview window.
};

#define FORWARD_DIRECTION (Vec3(0.f, 1.f, 0.f))

// (MATT) This should really live in a minimal AI include, which right now we don't have  {2009/04/08}
#ifndef INVALID_AIOBJECTID
typedef uint32 tAIObjectID;
#define INVALID_AIOBJECTID ((tAIObjectID)(0))
#endif
