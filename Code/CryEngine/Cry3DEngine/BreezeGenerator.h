// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Cry3DEngineBase.h"
#include <Cry3DEngine/IBreezeGenerator.h>

struct SBreeze;

// Spawns wind volumes around the camera to emulate breezes
class CBreezeGenerator : public IBreezeGenerator, public Cry3DEngineBase
{
	friend class C3DEngine;
public:
	CBreezeGenerator();
	~CBreezeGenerator();

	virtual void   Initialize() override;
	virtual void   Shutdown() override;

	virtual void   SetParams(const BreezeGeneratorParams& params) override;
	virtual BreezeGeneratorParams GetParams() const override;

	virtual void   SetEnabled(bool enabled) override                  { m_params.breezeGenerationEnabled = enabled; }
	virtual bool   GetEnabled() const override                        { return m_params.breezeGenerationEnabled;  }

	virtual void   SetStrength(float strength) override               { m_params.breezeStrength = strength; }
	virtual float  GetStrength() const override                       { return m_params.breezeStrength; }

	virtual void   SetMovementSpeed(float movementSpeed) override     { m_params.breezeMovementSpeed = movementSpeed; }
	virtual float  GetMovementSpeed() const override                  { return m_params.breezeMovementSpeed; }

	virtual void   SetVariance(float variance) override               { m_params.breezeVariance = variance; }
	virtual float  GetVariance() const override                       { return m_params.breezeVariance; }

	virtual void   SetLifetime(float lifetime) override               { m_params.breezeLifeTime = lifetime; }
	virtual float  GetLifetime() const override                       { return m_params.breezeLifeTime; }

	virtual void   SetCount(uint32 count) override                    { m_params.breezeCount = count; }
	virtual uint32 GetCount() const override                          { return m_params.breezeCount; }

	virtual void   SetSpawnRadius(float spawnRadius) override         { m_params.breezeSpawnRadius = spawnRadius; }
	virtual float  GetSpawnRadius() const override                    { return m_params.breezeSpawnRadius; }

	virtual void   SetSpread(float spread) override                   { m_params.breezeSpread = spread; }
	virtual float  GetSpread() const override                         { return m_params.breezeSpread; }

	virtual void   SetRadius(float radius) override                   { m_params.breezeRadius = radius; }
	virtual float  GetRadius() const override                         { return m_params.breezeRadius; }

	virtual void   SetAwakeThreshold(float awakeThreshSpeed) override { m_params.breezeAwakeThreshold = awakeThreshSpeed; }
	virtual float  GetAwakeThreshold() const override                 { return m_params.breezeAwakeThreshold; }

	virtual void   SetFixedHeight(float fixedHeight) override         { m_params.breezeFixedHeight = fixedHeight; }
	virtual float  GetFixedHeight() const override                    { return m_params.breezeFixedHeight; }

private:
	void Reset();
	void Update();

	BreezeGeneratorParams m_params;
	
	// The array of active breezes
	SBreeze* m_breezes;
};
