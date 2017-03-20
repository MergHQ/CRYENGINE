// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCollision.h"

CRY_PFX2_DBG

namespace pfx2
{

EParticleDataType PDT(EPDT_ContactPoint, SContactPoint, 1);

extern EParticleDataType EPVF_PositionPrev;

//////////////////////////////////////////////////////////////////////////
// CFeatureCollision

CFeatureCollision::CFeatureCollision()
	: m_collisionsLimitMode(ECollisionLimitMode::Unlimited)
	, m_terrain(true), m_staticObjects(true), m_dynamicObjects(false)
{
}

void CFeatureCollision::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_InitUpdate, this);
	pComponent->AddToUpdateList(EUL_PostUpdate, this);
	pComponent->AddParticleData(EPVF_PositionPrev);
	pComponent->AddParticleData(EPDT_ContactPoint);
}

void CFeatureCollision::Serialize(Serialization::IArchive& ar)
{
	ar(m_terrain, "Terrain", "Terrain");
	ar(m_staticObjects, "StaticObjects", "Static Objects");
	ar(m_dynamicObjects, "DynamicObjects", "Dynamic Objects");
	ar(m_elasticity, "Elasticity", "Elasticity");
	ar(m_slidingFriction, "SlidingFriction", "Sliding Friction");
	ar(m_collisionsLimitMode, "CollisionsLimitMode", "Collision Limit");
	if (m_collisionsLimitMode != ECollisionLimitMode::Unlimited)
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
		contact.m_state.ignore = 1;
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
		contact.m_state.ignore = 1;
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

void CFeatureCollision::InitParticles(const SUpdateContext& context)
{
	CParticleContainer& container = context.m_container;
	container.FillData(EPDT_ContactPoint, SContactPoint(), container.GetSpawnedRange());
}

// Collision constants
static const bool  bTraceTerrain = true;
static const bool  bAdjustPos    = false;
static const bool  bAdjustVel    = false;
static int   kMaxIter      = 3;        // Number of collisions to test per update; if more occur, leave particle at collide point
static float kExpandBack   = 0.01f,    // Fraction of test distance to expand backwards or forwards to prevent missed collisions
	         kExpandFront  = 0.0f;
static float kSlideBuffer  = 0.001f;   // Distance to slide particle above surface

ILINE void SetContactTime(SContactPoint& contact, const Vec3& start, const Vec3& ray)
{
	const float d = contact.m_normal | ray;
	const float s = contact.m_normal | (contact.m_point - start);
	contact.m_time = s * rcp_fast(d - FLT_EPSILON);
}

bool RayWorldIntersection(SContactPoint& contact, const Vec3& startIn, const Vec3& rayIn, int objectFilter, int iter)
{
	static const int collisionsFlags = sf_max_pierceable | (geom_colltype_ray | geom_colltype13) << rwi_colltype_bit | rwi_colltype_any | rwi_ignore_noncolliding | rwi_ignore_back_faces;

	bool bCollided = false;
	Vec3 start = startIn - rayIn * kExpandBack;
	Vec3 ray = rayIn * (1.0f + kExpandBack + kExpandFront);

	if (contact.m_state.sliding)
	{
		// Move point away from sliding surface for test
		start += contact.m_normal * kSlideBuffer;
		if (iter == 0)
			// Test into surface when using original movement
			ray -= contact.m_normal * (kSlideBuffer * 2.0f);
	}
	if (bTraceTerrain && (objectFilter & ent_terrain))
	{
		objectFilter &= ~ent_terrain;
		CHeightMap::SRayTrace rt;
		if (Cry3DEngineBase::GetTerrain()->RayTrace(start, start + ray, &rt, 0))
		{
			bCollided = true;
			contact.m_point = rt.vHit;
			contact.m_normal = rt.vNorm;
			SetContactTime(contact, startIn, rayIn);
			ray *= rt.fInterp;
		}
	}

	if (objectFilter)
	{
		ray_hit rayHit;
		if (gEnv->pPhysicalWorld->RayWorldIntersection(start, ray, objectFilter, collisionsFlags, &rayHit, 1))
		{
			CRY_PFX2_ASSERT((rayHit.n | ray) <= 0.0f);
			bCollided = true;
			contact.m_point = rayHit.pt;
			contact.m_normal = rayHit.n;
			SetContactTime(contact, startIn, rayIn);
		}
	}
	return bCollided;
}


void CFeatureCollision::ProcessCollisions(const SUpdateContext& context)
{
	// #PFX2_TODO : raytrace caching not implemented yet
	const int raytraceFilter = GetRayTraceFilter();

	CParticleContainer& container = context.m_container;
	const IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const IFStream lifeTimes = container.GetIFStream(EPDT_LifeTime);
	IVec3Stream positionsPrev = container.GetIVec3Stream(EPVF_PositionPrev);
	IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
	IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
	TIOStream<SContactPoint> contactPoints = container.GetTIOStream<SContactPoint>(EPDT_ContactPoint);

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		SContactPoint contact = contactPoints.Load(particleId);
		if (contact.m_state.ignore)
			continue;

		Vec3 position0 = positionsPrev.Load(particleId);
		Vec3 position1 = positions.Load(particleId);
		Vec3 accel;

		#if defined(DEBUG) || defined(CRY_DEBUG_PARTICLE_SYSTEM)
			// Allow rerunning code when debugging
			Vec3 p0save = position0;
			Vec3 p1save = position1;
			Vec3 v1save = velocities.Load(particleId);

			position0 = p0save;
			position1 = p1save;
			velocities.Store(particleId, v1save);
		#endif

		float dT = DeltaTime(context.m_deltaTime, particleId, normAges, lifeTimes);

		for (int iter = 0; iter < kMaxIter; ++iter)
		{
			const Vec3 rayDir = position1 - position0;

			#if defined(DEBUG) || defined(CRY_DEBUG_PARTICLE_SYSTEM)
				float h0 = position0.z - gEnv->p3DEngine->GetTerrainElevation(position0.x, position0.y);
				float h1 = position1.z - gEnv->p3DEngine->GetTerrainElevation(position1.x, position1.y);
			#endif

			if (rayDir.IsZero() || !RayWorldIntersection(contact, position0, rayDir, raytraceFilter, iter))
			{
				if (iter == 0)
					contact.m_state = {};
				break;
			}

			contact.m_time *= dT;

			// Infer average acceleration over frame
			// p1 = p0 + v1 dt + a/2 dt^2
			const Vec3 velocity1 = velocities.Load(particleId);
			if (iter == 0)
			{
				const float invDT = rcp(dT);
				accel = (velocity1 - rayDir * invDT) * (invDT * 2.0f);
			}

			if (bAdjustPos)
			{
				// Point is adjusted away from linear raycast, accounting for acceleration
				const Plane plane = Plane::CreatePlane(contact.m_normal, contact.m_point);
				const Vec3 hit = position0 + velocity1 * contact.m_time - accel * (sqr(contact.m_time) * 0.5f);
				contact.m_point = hit - plane.n * (plane | hit);
			}

			contact.m_time = clamp(contact.m_time, 0.0f, dT);

			dT -= contact.m_time;

			Vec3 velContact = velocity1;
			if (bAdjustVel)
				velContact -= accel * dT;
			
			// Collision response
			if (iter >= kMaxIter - 1)
				dT = 0.0f;  // On last iteration, stop particle at collision point for this frame

			Vec3 velocity2 = velocity1;

			// Sliding occurs when the bounce distance would be less than the collide buffer.
			// d = - v^2 / 2a <= dmax
			// v^2 <= -dmax*2a
			const float velNorm = contact.m_normal | velContact;
			const float accNorm = accel | contact.m_normal;
			if (sqr(velNorm * m_elasticity) > accNorm * kSlideBuffer * -2.0f)
			{
				// Bounce
				contact.m_state.collided = 1;
				contact.m_totalCollisions++;
				velocity2 -= contact.m_normal * (velNorm * (1.0f + m_elasticity));
				position1 += (velocity2 - velocity1) * dT;
			}
			else
			{
				// Slide
				contact.m_state.collided = !contact.m_state.sliding;
				contact.m_totalCollisions += contact.m_state.collided;
				contact.m_state.sliding = 1;
				velocity2 -= contact.m_normal * velNorm;
				position1 += (velocity2 - velocity1) * dT;
				position1 -= contact.m_normal * ((position1 - contact.m_point) | contact.m_normal);

				// Create sliding acceleration opposite to current velocity,
				// limit it by maximum movement during frame
				// V/v * min(an * sf, (V|V/v)/t + (A|V/v))
				const float velSqr = velocity2.len2();
				if (dT * velSqr * m_slidingFriction > FLT_EPSILON)
				{
					const Vec3 velDir = velocity2 * rsqrt_fast(velSqr);
					const float accDrag = -accNorm * m_slidingFriction;
					const float accMax = sqrt_fast(velSqr) * rcp_fast(dT) + (accel | velDir);
					const Vec3 accelDrag = velDir * -min(accDrag, accMax);
					velocity2 += accelDrag * dT;
					position1 += accelDrag * (sqr(dT) * 0.5f);
				}
			}

			positions.Store(particleId, position1);
			velocities.Store(particleId, velocity2);

			position0 = contact.m_point;
		}	
		contactPoints.Store(particleId, contact);
	}
	CRY_PFX2_FOR_END;
}

void CFeatureCollision::PostUpdate(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	ProcessCollisions(context);

	switch (m_collisionsLimitMode)
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
