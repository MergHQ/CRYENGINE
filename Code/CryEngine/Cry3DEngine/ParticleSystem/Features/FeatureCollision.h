// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     22/10/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"

CRY_PFX2_DBG

namespace pfx2
{

enum class EContactPointsFlags
{
	Active   = BIT(0),
	Collided = BIT(1),
	Sliding  = BIT(2),
	Ignore   = BIT(3),
};

struct SContactPoint
{
	SContactPoint()
		: m_point(ZERO)
		, m_normal(ZERO)
		, m_flags(0)
		, m_totalCollisions(0) {}

	Vec3 m_point;
	Vec3 m_normal;
	uint m_totalCollisions;
	uint m_flags;	// EContactPointsFlags
};

extern EParticleDataType EPDT_ContactPoint;

SERIALIZATION_ENUM_DECLARE(ECollisionLimitMode, ,
	Unlimited,
	Ignore,
	Stop,
	Kill
	)

//////////////////////////////////////////////////////////////////////////
// CFeatureCollision

class CFeatureCollision : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

public:
	CFeatureCollision();

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override;
	virtual void Serialize(Serialization::IArchive& ar) override;
	virtual void Update(const SUpdateContext& context) override;

	bool  IsActive() const           { return m_terrain || m_staticObjects || m_staticObjects; }
	float GetElasticity() const      { return m_elasticity; }
	int   GetRayTraceFilter() const;

private:
	template<typename TCollisionLimit>
	void UpdateCollisionLimit(const SUpdateContext& context);

	UUnitFloat          m_elasticity;
	ECollisionLimitMode m_collisionsLimtMode;
	UBytePos            m_maxCollisions;
	bool                m_terrain;
	bool                m_staticObjects;
	bool                m_dynamicObjects;
};

//////////////////////////////////////////////////////////////////////////
// CFeatureGPUCollision

class CFeatureGPUCollision : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureGPUCollision()
		: CParticleFeature(gpu_pfx2::eGpuFeatureType_Collision)
		, m_offset(0.5f)
		, m_radius(0.5f)
		, m_restitution(0.5f)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override;
	virtual void Serialize(Serialization::IArchive& ar) override;
private:
	float m_offset;
	float m_radius;
	float m_restitution;
};

}
