// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCollision.h"

CRY_PFX2_DBG

namespace pfx2
{

EParticleDataType PDT(EPDT_ContactPoint, SContactPoint, 1);

//////////////////////////////////////////////////////////////////////////
// CFeatureCollision

CFeatureCollision::CFeatureCollision()
	: m_collisionsLimtMode(ECollisionLimitMode::Unlimited)
	, m_terrain(true), m_staticObjects(true), m_dynamicObjects(false)
{
}

void CFeatureCollision::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	if (m_collisionsLimtMode != ECollisionLimitMode::Unlimited)
		pComponent->AddToUpdateList(EUL_Update, this);
}

void CFeatureCollision::Serialize(Serialization::IArchive& ar)
{
	ar(m_terrain, "Terrain", "Terrain");
	ar(m_staticObjects, "StaticObjects", "Static Objects");
	ar(m_dynamicObjects, "DynamicObjects", "Dynamic Objects");
	ar(m_elasticity, "Elasticity", "Elasticity");
	ar(m_collisionsLimtMode, "CollisionsLimtMode", "Collision Limit");
	if (m_collisionsLimtMode != ECollisionLimitMode::Unlimited)
		ar(m_maxCollisions, "MaxCollisions", "Maximum Collisions");
}

int CFeatureCollision::GetRayTraceFilter() const
{
	int filter = ent_no_ondemand_activation;
	if (m_terrain)
		filter |= ent_terrain;
	if (m_staticObjects)
		filter |= ent_static;
	if (m_dynamicObjects)
		filter |= ent_rigid | ent_sleeping_rigid | ent_living | ent_independent;
	return filter;
}

struct SCollisionLimitIgnore
{
	SCollisionLimitIgnore(CParticleContainer& container)
		: m_contactPoints(container.GetTIOStream<SContactPoint>(EPDT_ContactPoint)) {}
	void CollisionLimit(TParticleId particleId)
	{
		SContactPoint contact = m_contactPoints.Load(particleId);
		contact.m_flags |= uint(EContactPointsFlags::Ignore);
		m_contactPoints.Store(particleId, contact);
	}
	TIOStream<SContactPoint> m_contactPoints;
};

struct SCollisionLimitStop
{
	SCollisionLimitStop(CParticleContainer& container)
		: m_contactPoints(container.GetTIOStream<SContactPoint>(EPDT_ContactPoint))
		, m_positions(container.GetIOVec3Stream(EPVF_Position))
		, m_velocities(container.GetIOVec3Stream(EPVF_Velocity)) {}
	void CollisionLimit(TParticleId particleId)
	{
		SContactPoint contact = m_contactPoints.Load(particleId);
		contact.m_flags |= uint(EContactPointsFlags::Ignore);
		m_positions.Store(particleId, contact.m_point);
		m_velocities.Store(particleId, Vec3(ZERO));
		m_contactPoints.Store(particleId, contact);
	}
	TIOStream<SContactPoint> m_contactPoints;
	IOVec3Stream             m_positions;
	IOVec3Stream             m_velocities;
};

struct SCollisionLimitKill
{
	SCollisionLimitKill(CParticleContainer& container)
		: m_ages(container.GetIOFStream(EPDT_NormalAge)) {}
	void CollisionLimit(TParticleId particleId)
	{
		m_ages.Store(particleId, 1.0f);
	}
	IOFStream m_ages;
};

void CFeatureCollision::Update(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	switch (m_collisionsLimtMode)
	{
	case ECollisionLimitMode::Ignore:
		UpdateCollisionLimit<SCollisionLimitIgnore>(context);
		break;
	case ECollisionLimitMode::Stop:
		UpdateCollisionLimit<SCollisionLimitStop>(context);
		break;
	case ECollisionLimitMode::Kill:
		UpdateCollisionLimit<SCollisionLimitKill>(context);
		break;
	}
}

template<typename TCollisionLimit>
void CFeatureCollision::UpdateCollisionLimit(const SUpdateContext& context)
{
	CParticleContainer& container = context.m_container;
	TCollisionLimit limiter(container);
	const TIStream<SContactPoint> contactPoints = container.GetTIStream<SContactPoint>(EPDT_ContactPoint);

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		const SContactPoint contact = contactPoints.Load(particleId);
		if (contact.m_totalCollisions >= m_maxCollisions)
			limiter.CollisionLimit(particleId);
	}
	CRY_PFX2_FOR_END;
}

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureCollision, "Motion", "Collisions", colorMotion);

//////////////////////////////////////////////////////////////////////////
// CFeatureGPUCollision

void CFeatureGPUCollision::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_Update, this);

	if (auto pInt = GetGpuInterface())
	{
		gpu_pfx2::SFeatureParametersCollision params;
		params.offset = m_offset;
		params.radius = m_radius;
		params.restitution = m_restitution;
		pInt->SetParameters(params);
	}
}

void CFeatureGPUCollision::Serialize(Serialization::IArchive& ar)
{
	ar(m_offset, "Offset", "Offset");
	ar(m_radius, "Radius", "Radius");
	ar(m_restitution, "Restitution", "Restitution");
}

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureGPUCollision, "GPU Particles", "Collision", colorGPU);
CRY_PFX2_LEGACY_FEATURE(CParticleFeature, CFeatureGPUCollision, "MotionCollision");

}
