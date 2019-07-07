// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ITimeOfDay.h"

typedef ITimeOfDay::Wind BreezeGeneratorParams;

//! Interface to the Breeze Generator functionality.
struct IBreezeGenerator
{
	// <interfuscator:shuffle>
	virtual ~IBreezeGenerator() = default;
	
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;

	// Set/get all parameters at once
	virtual void SetParams(const BreezeGeneratorParams& params) = 0;
	virtual BreezeGeneratorParams GetParams() const = 0;

	// Breeze generation enabled?
	virtual void SetEnabled(bool enabled) = 0;
	virtual bool GetEnabled() const = 0;

	// The strength of the breeze (as a factor of the original wind vector)
	virtual void SetStrength(float strength) = 0;
	virtual float GetStrength() const = 0;

	// The speed of the breeze movement (not coupled to the wind speed)
	virtual void SetMovementSpeed(float movementSpeed) = 0;
	virtual float GetMovementSpeed() const = 0;

	// The random variance of each breeze in respect to it's other attributes
	virtual void SetVariance(float variance) = 0;
	virtual float GetVariance() const = 0;

	// The max. life of each breeze
	virtual void SetLifetime(float lifetime) = 0;
	virtual float GetLifetime() const = 0;

	// The max. number of wind areas active at the same time
	virtual void SetCount(uint32 count) = 0;
	virtual uint32 GetCount() const = 0;

	// The radius around the camera where the breezes will be spawned
	virtual void SetSpawnRadius(float spawnRadius) = 0;
	virtual float GetSpawnRadius() const = 0;

	// The spread (variation in direction on spawn)
	virtual void SetSpread(float spread) = 0;
	virtual float GetSpread() const = 0;

	// The max. extents of each breeze
	virtual void SetRadius(float radius) = 0;
	virtual float GetRadius() const = 0;

	// Approximate threshold velocity that the wind can add to an entity part per second that will awake it (0 disables)
	virtual void SetAwakeThreshold(float awakeThreshSpeed) = 0;
	virtual float GetAwakeThreshold() const = 0;

	// Set a fixed height for the breeze, for levels without terrain. -1 uses the terrain height
	virtual void SetFixedHeight(float fixedHeight) = 0;
	virtual float GetFixedHeight() const = 0;
	// </interfuscator:shuffle>
};
