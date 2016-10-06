// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureMotion.h"
#include "ParticleSystem/ParticleEmitter.h"
#include <CrySerialization/Math.h>
#include "FeatureCollision.h"

CRY_PFX2_DBG

namespace pfx2
{

void ILocalEffector::Serialize(Serialization::IArchive& ar)
{
	ar(m_enabled);
}

//////////////////////////////////////////////////////////////////////////
// CFeatureMotionPhysics

EParticleDataType PDT(EPDT_Gravity, float, 1, BHasInit(true));
EParticleDataType PDT(EPDT_Drag, float, 1, BHasInit(true));
EParticleDataType PDT(EPVF_Acceleration, float, 3);
EParticleDataType PDT(EPVF_VelocityField, float, 3);

CFeatureMotionPhysics::CFeatureMotionPhysics()
	: m_gravity(0.0f)
	, m_drag(0.0f)
	, m_windMultiplier(1.0f)
	, m_angularDragMultiplier(1.0f)
	, m_uniformAcceleration(ZERO)
	, m_uniformWind(ZERO)
	, m_linearIntegrator(EI_Linear)
	, m_pCollisionFeature(nullptr)
	, CParticleFeature(gpu_pfx2::eGpuFeatureType_Motion)
{
}

void CFeatureMotionPhysics::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_Update, this);

	m_gravity.AddToComponent(pComponent, this, EPDT_Gravity);
	m_drag.AddToComponent(pComponent, this, EPDT_Drag);

	auto it = std::remove_if(m_localEffectors.begin(), m_localEffectors.end(), [](const PLocalEffector& ptr) { return !ptr; });
	m_localEffectors.erase(it, m_localEffectors.end());
	m_computeList.clear();
	m_moveList.clear();
	for (PLocalEffector pEffector : m_localEffectors)
	{
		if (pEffector->IsEnabled())
			pEffector->AddToMotionFeature(pComponent, this);
	}

	const bool hasDrag = m_drag.GetBaseValue() != 0.0f;
	const bool hasGravity = m_gravity.GetBaseValue() != 0.0f || !m_uniformAcceleration.IsZero();
	const bool hasEffectors = !m_localEffectors.empty();

	if (hasDrag || hasGravity || hasEffectors)
		m_linearIntegrator = EI_DragFast;
	else
		m_linearIntegrator = EI_Linear;
	m_angularIntegrator = m_linearIntegrator;
	if (m_angularDragMultiplier == 0.0f)
		m_angularIntegrator = EI_Linear;

	if (auto pInt = GetGpuInterface())
	{
		gpu_pfx2::SFeatureParametersMotionPhysics params;
		params.gravity = m_gravity.GetBaseValue();
		params.drag = m_drag.GetBaseValue();
		params.uniformAcceleration = m_uniformAcceleration;
		params.uniformWind = m_uniformWind;
		params.windMultiplier = m_windMultiplier;
		pInt->SetParameters(params);

		for (PLocalEffector pEffector : m_localEffectors)
		{
			if (pEffector->IsEnabled())
				pEffector->SetParameters(pInt);
		}
	}

	m_pCollisionFeature = pComponent->GetCFeatureByType<CFeatureCollision>();
	if (m_pCollisionFeature && m_pCollisionFeature->IsActive())
	{
		pComponent->AddToUpdateList(EUL_InitUpdate, this);
		pComponent->AddParticleData(EPDT_ContactPoint);
	}
	else
		m_pCollisionFeature = nullptr;
}

void CFeatureMotionPhysics::Serialize(Serialization::IArchive& ar)
{
	CParticleFeature::Serialize(ar);
	ar(m_gravity, "gravity", "Gravity Scale");
	ar(m_drag, "drag", "Drag");
	ar(m_uniformAcceleration, "UniformAcceleration", "Uniform Acceleration");
	ar(m_uniformWind, "UniformWind", "Uniform Wind");
	ar(m_windMultiplier, "windMultiplier", "Level Wind Scale");
	if (!ar(m_angularDragMultiplier, "AngularDragMultiplier", "Angular Drag Multiplier"))
		m_angularDragMultiplier = 0.0f;
	ar(m_localEffectors, "localEffectors", "Local Effectors");
}

void CFeatureMotionPhysics::InitParticles(const SUpdateContext& context)
{
	m_gravity.InitParticles(context, EPDT_Gravity);
	m_drag.InitParticles(context, EPDT_Drag);

	CParticleContainer& container = context.m_container;
	const SContactPoint defaultVal;
	const SUpdateRange spawnRange(container.GetFirstSpawnParticleId(), container.GetLastParticleId());
	container.FillData(EPDT_ContactPoint, defaultVal, spawnRange);
}

void CFeatureMotionPhysics::Update(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleContainer& container = context.m_container;
	m_gravity.Update(context, EPDT_Gravity);
	m_drag.Update(context, EPDT_Drag);

	if (m_pCollisionFeature)
		ProcessCollisions(context);

	IOVec3Stream velocityField = container.GetIOVec3Stream(EPVF_VelocityField);
	IOVec3Stream accelerations = container.GetIOVec3Stream(EPVF_Acceleration);
	for (ILocalEffector* pEffector : m_computeList)
		pEffector->ComputeEffector(context, velocityField, accelerations);

	if (!m_moveList.empty())
	{
		STempVec3Stream moveBefore(context.m_pMemHeap, context.m_updateRange);
		STempVec3Stream moveAfter(context.m_pMemHeap, context.m_updateRange);
		moveBefore.Clear(context.m_updateRange);
		moveAfter.Clear(context.m_updateRange);

		for (ILocalEffector* pEffector : m_moveList)
			pEffector->ComputeMove(context, moveBefore.GetIOVec3Stream(), 0.0f);
		
		Integrate(context);
		
		for (ILocalEffector* pEffector : m_moveList)
			pEffector->ComputeMove(context, moveAfter.GetIOVec3Stream(), context.m_deltaTime);

		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
		{
			const Vec3v position0 = positions.Load(particleGroupId);
			const Vec3v move0 = moveBefore.GetIOVec3Stream().Load(particleGroupId);
			const Vec3v move1 = moveAfter.GetIOVec3Stream().Load(particleGroupId);
			const Vec3v position1 = position0 + (move1 - move0);
			positions.Store(particleGroupId, position1);
		}
		CRY_PFX2_FOR_END;
	}
	else
	{
		Integrate(context);
	}

	if (m_pCollisionFeature)
		CollisionResponse(context);
}

void CFeatureMotionPhysics::AddToComputeList(ILocalEffector* pEffector)
{
	if (std::find(m_computeList.begin(), m_computeList.end(), pEffector) == m_computeList.end())
		m_computeList.push_back(pEffector);
}

void CFeatureMotionPhysics::AddToMoveList(ILocalEffector* pEffector)
{
	if (std::find(m_moveList.begin(), m_moveList.end(), pEffector) == m_moveList.end())
		m_moveList.push_back(pEffector);
}

void CFeatureMotionPhysics::Integrate(const SUpdateContext& context)
{
	CParticleContainer& container = context.m_container;

	switch (m_linearIntegrator)
	{
	case EI_Linear:
		LinearSimpleIntegral(context);
		break;
	case EI_DragFast:
		LinearDragFastIntegral(context);
		break;
	}

	const bool spin2D = container.HasData(EPDT_Spin2D) && container.HasData(EPDT_Angle2D);
	const bool spin3D = container.HasData(EPVF_AngularVelocity) && container.HasData(EPQF_Orientation);
	if (spin2D || spin3D)
	{
		switch (m_angularIntegrator)
		{
		case EI_Linear:
			AngularSimpleIntegral(context);
			break;
		case EI_DragFast:
			AngularDragFastIntegral(context);
			break;
		}
	}
}

void CFeatureMotionPhysics::LinearSimpleIntegral(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleContainer& container = context.m_container;
	IOVec3Stream position = container.GetIOVec3Stream(EPVF_Position);
	IVec3Stream velocity = container.GetIVec3Stream(EPVF_Velocity);
	IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const float deltaTime = context.m_deltaTime;
	const floatv deltaTimeV = ToFloatv(deltaTime);
	
	CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
	{
		const Vec3v p0 = position.Load(particleGroupId);
		const Vec3v v0 = velocity.Load(particleGroupId);
		const floatv normAge = normAges.Load(particleGroupId);
		const floatv dT = DeltaTime(normAge, deltaTimeV);
		const Vec3v p1 = MAdd(v0, dT, p0);
		position.Store(particleGroupId, p1);
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionPhysics::LinearDragFastIntegral(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleEmitter* pEmitter = context.m_runtime.GetEmitter();
	CParticleContainer& container = context.m_container;

	IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
	IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
	IVec3Stream velocityField = container.GetIVec3Stream(EPVF_VelocityField);
	IVec3Stream accelerations = container.GetIVec3Stream(EPVF_Acceleration);
	IFStream gravities = container.GetIFStream(EPDT_Gravity, 0.0f);
	IFStream drags = container.GetIFStream(EPDT_Drag);
	IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const floatv fTime = ToFloatv(context.m_deltaTime);

	const Vec3v physAccel = ToVec3v(pEmitter->GetPhysicsEnv().m_UniformForces.vAccel);
	const Vec3v physWind = ToVec3v(pEmitter->GetPhysicsEnv().m_UniformForces.vWind * m_windMultiplier + m_uniformWind);
	const Vec3v uniformAccel = ToVec3v(m_uniformAcceleration);

	const float maxDragFactor = m_drag.GetValueRange(context).end * context.m_deltaTime;
	const floatv dragReduction = ToFloatv(div_min(1.0f - exp(-maxDragFactor), maxDragFactor, 1.0f));

	CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
	{
		const floatv normAge = normAges.Load(particleGroupId);
		const floatv dT = DeltaTime(normAge, fTime);
		const floatv dTh = dT * ToFloatv(0.5f);

		const Vec3v partAccel = accelerations.SafeLoad(particleGroupId);
		const floatv gravMult = gravities.SafeLoad(particleGroupId);
		const Vec3v p0 = positions.Load(particleGroupId);
		const Vec3v v0 = velocities.Load(particleGroupId);

		Vec3v a = MAdd(physAccel, gravMult, partAccel) + uniformAccel;
		if (maxDragFactor > 0.0f)
		{
			const Vec3v fieldVel = velocityField.SafeLoad(particleGroupId);
			const Vec3v partVel = physWind + fieldVel;
			const floatv drag = drags.SafeLoad(particleGroupId) * dragReduction;
			a = MAdd(partVel - v0, drag, a);                        // (partVel-v0)*drag + a
		}
		const Vec3v v1 = MAdd(a, dT, v0);                         // a*dT + v0
		const Vec3v p1 = MAdd(v0 + v1, dTh, p0);                  // (v0 + v1)/2 * dT + p0

		positions.Store(particleGroupId, p1);
		velocities.Store(particleGroupId, v1);
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionPhysics::AngularSimpleIntegral(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleContainer& container = context.m_container;
	const IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const IFStream spins = container.GetIFStream(EPDT_Spin2D);
	const IVec3Stream angularVelocities = container.GetIVec3Stream(EPVF_AngularVelocity);
	const float deltaTime = context.m_deltaTime;
	const floatv deltaTimeV = ToFloatv(deltaTime);
	IOFStream angles = container.GetIOFStream(EPDT_Angle2D);
	IOQuatStream orientations = container.GetIOQuatStream(EPQF_Orientation);

	const bool spin2D = container.HasData(EPDT_Spin2D) && container.HasData(EPDT_Angle2D);
	const bool spin3D = container.HasData(EPVF_AngularVelocity) && container.HasData(EPQF_Orientation);

	CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
	{
		const floatv normAge = normAges.Load(particleGroupId);
		const floatv dT = DeltaTime(normAge, deltaTimeV);

		if (spin2D)
		{
			const floatv spin0 = spins.Load(particleGroupId);
			const floatv angle0 = angles.Load(particleGroupId);
			const floatv angle1 = MAdd(spin0, deltaTimeV, angle0);
			angles.Store(particleGroupId, angle1);
		}

		if (spin3D)
		{
			const Vec3v angularVelocity = angularVelocities.Load(particleGroupId);
			const Quatv orientation0 = orientations.Load(particleGroupId);
			const Quatv orientation1 = AddAngularVelocity(orientation0, angularVelocity, deltaTimeV);
			orientations.Store(particleGroupId, orientation1);
		}
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionPhysics::AngularDragFastIntegral(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleContainer& container = context.m_container;
	const IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const IFStream drags = container.GetIFStream(EPDT_Drag);
	const float deltaTime = context.m_deltaTime;
	const floatv deltaTimeV = ToFloatv(deltaTime);
	IOFStream spins = container.GetIOFStream(EPDT_Spin2D);
	IOVec3Stream angularVelocities = container.GetIOVec3Stream(EPVF_AngularVelocity);
	IOFStream angles = container.GetIOFStream(EPDT_Angle2D);
	IOQuatStream orientations = container.GetIOQuatStream(EPQF_Orientation);

	const float maxDragFactor = m_drag.GetValueRange(context).end * context.m_deltaTime * m_angularDragMultiplier;
	const floatv dragReduction = ToFloatv(div_min(1.0f - exp_tpl(-maxDragFactor), maxDragFactor, 1.0f));

	const bool spin2D = container.HasData(EPDT_Spin2D) && container.HasData(EPDT_Angle2D);
	const bool spin3D = container.HasData(EPVF_AngularVelocity) && container.HasData(EPQF_Orientation);

	CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
	{
		const floatv normAge = normAges.Load(particleGroupId);
		const floatv dT = DeltaTime(normAge, deltaTimeV);
		const floatv drag = drags.SafeLoad(particleGroupId) * dragReduction;

		if (spin2D)
		{
			const floatv angle0 = angles.Load(particleGroupId);
			const floatv spin0 = spins.Load(particleGroupId);
			const floatv a = -spin0 * drag;
			const floatv spin1 = MAdd(a, dT, spin0);
			const floatv angle1 = MAdd(spin1, deltaTimeV, angle0);
			angles.Store(particleGroupId, angle1);
			spins.Store(particleGroupId, spin1);
		}

		if (spin3D)
		{
			const Quatv orientation0 = orientations.Load(particleGroupId);
			const Vec3v angularVelocity0 = angularVelocities.Load(particleGroupId);
			const Vec3v a = -angularVelocity0 * drag;
			const Vec3v angularVelocity1 = MAdd(a, dT, angularVelocity0);
			const Quatv orientation1 = AddAngularVelocity(orientation0, angularVelocity1, deltaTimeV);
			orientations.Store(particleGroupId, orientation1);
			angularVelocities.Store(particleGroupId, angularVelocity1);
		}
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionPhysics::ProcessCollisions(const SUpdateContext& context)
{
	// #PFX2_TODO : raytrace caching not implemented yet

	IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;

	const int collisionsFlags = sf_max_pierceable | (geom_colltype_ray | geom_colltype13) << rwi_colltype_bit | rwi_colltype_any | rwi_ignore_noncolliding;
	const int raytraceFilter = m_pCollisionFeature->GetRayTraceFilter();

	CParticleContainer& container = context.m_container;
	IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
	IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
	IFStream sizes = container.GetIFStream(EPDT_Size);
	const IFStream normAges = container.GetIFStream(EPDT_NormalAge);
	const float deltaTime = context.m_deltaTime;
	const float threshold = 1.0f / 1024.0f;
	TIOStream<SContactPoint> contactPoints = container.GetTIOStream<SContactPoint>(EPDT_ContactPoint);

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		SContactPoint contact = contactPoints.Load(particleId);
		contact.m_flags &= ~uint(EContactPointsFlags::Collided);
		if (!(contact.m_flags & uint(EContactPointsFlags::Sliding)))
			contact.m_flags &= ~uint(EContactPointsFlags::Active);
		contactPoints.Store(particleId, contact);
		if (contact.m_flags & uint(EContactPointsFlags::Ignore))
			continue;

		const Vec3 position0 = positions.Load(particleId);
		const Vec3 velocity0 = velocities.Load(particleId);
		const float normAge = normAges.Load(particleId);
		const float dT = DeltaTime(normAge, deltaTime);
		const Vec3 rayStart = position0;
		const Vec3 rayDir = velocity0 * dT;
		const float size = sizes.Load(particleId);

		if (rayDir.GetLengthSquared() < threshold)
			continue;

		ray_hit rayHit;
		const int numHits = pPhysics->RayWorldIntersection(
			rayStart, rayDir, raytraceFilter, collisionsFlags,
			&rayHit, 1);
		if (numHits == 0)
			continue;
		if (rayHit.n.Dot(velocity0) >= 0.0f)
			continue;

		contact.m_point = rayHit.pt;
		contact.m_normal = rayHit.n;
		contact.m_totalCollisions++;
		contact.m_flags = uint(EContactPointsFlags::Active) | uint(EContactPointsFlags::Collided);
		contactPoints.Store(particleId, contact);
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionPhysics::CollisionResponse(const SUpdateContext& context)
{
	CParticleContainer& container = context.m_container;
	TIOStream<SContactPoint> contactPoints = container.GetTIOStream<SContactPoint>(EPDT_ContactPoint);
	IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
	IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
	const float elasticity = m_pCollisionFeature->GetElasticity();

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		SContactPoint contact = contactPoints.Load(particleId);
		if (!(contact.m_flags & uint(EContactPointsFlags::Active)))
			continue;
		if (contact.m_flags & uint(EContactPointsFlags::Ignore))
			continue;

		const Vec3 normal = contact.m_normal;
		const Plane plane = Plane(normal, -contact.m_point.Dot(normal));
		const Vec3 position0 = positions.Load(particleId);
		const Vec3 velocity0 = velocities.Load(particleId);

		const float distToPlane = -plane.DistFromPlane(position0);
		if (distToPlane < 0.0f)
			continue;
		if (normal.Dot(velocity0) > 0.0f)
			continue;
		const Vec3 position1 = position0 + normal * distToPlane;
		positions.Store(particleId, position1);

		const Vec3 normalVelocity0 = normal * normal.Dot(velocity0);
		const Vec3 tangentVelocity0 = velocity0 - normalVelocity0;
		const Vec3 velocity1 = -normalVelocity0 * elasticity + tangentVelocity0;

		const float collideBufferDist = 1.0f / 1024.0f;
		const float velBounce = (normalVelocity0 * elasticity).GetLengthSquared();
		if (velBounce <= collideBufferDist)
		{
			contact.m_flags |= uint(EContactPointsFlags::Sliding);
			contactPoints.Store(particleId, contact);
		}

		velocities.Store(particleId, velocity1);

		if (contact.m_flags & uint(EContactPointsFlags::Sliding))
		{
			contact.m_point = position1;
			contactPoints.Store(particleId, contact);
		}
	}
	CRY_PFX2_FOR_END;
}

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMotionPhysics, "Motion", "Physics", colorMotion);

//////////////////////////////////////////////////////////////////////////
// CFeatureMotionCryPhysics

EParticleDataType PDT(EPDT_PhysicalEntity, IPhysicalEntity*);

CFeatureMotionCryPhysics::CFeatureMotionCryPhysics()
	: m_gravity(1.0f)
	, m_drag(0.0f)
	, m_density(1.0f)
	, m_thickness(0.0f)
	, m_uniformAcceleration(ZERO)
{
}

void CFeatureMotionCryPhysics::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_PostInitUpdate, this);
	pComponent->AddToUpdateList(EUL_Update, this);
	pComponent->AddParticleData(EPDT_PhysicalEntity);
	pComponent->AddParticleData(EPVF_Position);
	pComponent->AddParticleData(EPVF_Velocity);
	pComponent->AddParticleData(EPVF_AngularVelocity);
	pComponent->AddParticleData(EPQF_Orientation);
}

void CFeatureMotionCryPhysics::Serialize(Serialization::IArchive& ar)
{
	CParticleFeature::Serialize(ar);
	ar(m_surfaceTypeName, "SurfaceType", "Surface Type");
	ar(m_gravity, "gravity", "Gravity Scale");
	ar(m_drag, "drag", "Drag");
	ar(m_density, "density", "Density");
	ar(m_thickness, "thickness", "Thickness");
	ar(m_uniformAcceleration, "UniformAcceleration", "Uniform Acceleration");
}

void CFeatureMotionCryPhysics::PostInitParticles(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	CParticleEmitter* pEmitter = context.m_runtime.GetEmitter();
	const SPhysEnviron& physicsEnv = pEmitter->GetPhysicsEnv();

	CParticleContainer& container = context.m_container;
	auto physicalEntities = container.GetTIOStream<IPhysicalEntity*>(EPDT_PhysicalEntity);
	const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
	const IVec3Stream velocities = container.GetIVec3Stream(EPVF_Velocity);
	const IVec3Stream angularVelocities = container.GetIVec3Stream(EPVF_AngularVelocity);
	const IQuatStream orientations = container.GetIQuatStream(EPQF_Orientation);
	const IFStream sizes = container.GetIFStream(EPDT_Size);
	const float sphereVolume = 4.0f / 3.0f * gf_PI;
	const Vec3 uniformAcceleration = physicsEnv.m_UniformForces.vAccel * m_gravity + m_uniformAcceleration;
	ISurfaceType* pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName(m_surfaceTypeName.c_str());
	const int surfaceTypeId = pSurfaceType ? pSurfaceType->GetId() : 0;

	CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
	{
		pe_params_pos particlePosition;
		particlePosition.pos = positions.Load(particleId);
		particlePosition.q = orientations.Load(particleId);

		IPhysicalEntity* pPhysicalEntity = pPhysicalWorld->CreatePhysicalEntity(
			PE_PARTICLE,
			&particlePosition);
		pe_params_particle particleParams;

		// Compute particle mass from volume of object.
		const float size = sizes.Load(particleId);
		const float sizeCube = size * size * size;
		particleParams.size = size;
		particleParams.mass = m_density * sizeCube * sphereVolume;

		const Vec3 velocity = velocities.Load(particleId);
		particleParams.thickness = m_thickness * size;
		particleParams.velocity = velocity.GetLength();
		if (particleParams.velocity > 0.f)
			particleParams.heading = velocity / particleParams.velocity;

		particleParams.surface_idx = surfaceTypeId;
		particleParams.flags = particle_no_path_alignment;
		particleParams.kAirResistance = m_drag;
		particleParams.gravity = uniformAcceleration;
		particleParams.q0 = particlePosition.q;
		particleParams.wspin = angularVelocities.Load(particleId);

		pPhysicalEntity->SetParams(&particleParams);

		pe_params_flags particleFlags;
		particleFlags.flagsOR = pef_never_affect_triggers;
		particleFlags.flagsOR |= pef_log_collisions;
		pPhysicalEntity->SetParams(&particleFlags);
		pPhysicalEntity->AddRef();

		physicalEntities.Store(particleId, pPhysicalEntity);
	}
	CRY_PFX2_FOR_END;
}

void CFeatureMotionCryPhysics::Update(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	CParticleContainer& container = context.m_container;
	auto physicalEntities = container.GetTIOStream<IPhysicalEntity*>(EPDT_PhysicalEntity);
	IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
	IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
	IOVec3Stream angularVelocities = container.GetIOVec3Stream(EPVF_AngularVelocity);
	IOQuatStream orientations = container.GetIOQuatStream(EPQF_Orientation);
	auto states = container.GetTIStream<uint8>(EPDT_State);

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		IPhysicalEntity* pPhysicalEntity = physicalEntities.Load(particleId);
		if (!pPhysicalEntity)
			continue;

		pe_status_pos statusPosition;
		if (pPhysicalEntity->GetStatus(&statusPosition))
		{
			positions.Store(particleId, statusPosition.pos);
			orientations.Store(particleId, statusPosition.q);
		}

		pe_status_dynamics statusDynamics;
		if (pPhysicalEntity->GetStatus(&statusDynamics))
		{
			velocities.Store(particleId, statusDynamics.v);
			angularVelocities.Store(particleId, statusDynamics.w);
		}

		const uint8 state = states.Load(particleId);
		if (state == ES_Expired)
		{
			pPhysicalWorld->DestroyPhysicalEntity(pPhysicalEntity);
			physicalEntities.Store(particleId, 0);
		}
	}
	CRY_PFX2_FOR_END;
}

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMotionCryPhysics, "Motion", "CryPhysics", colorMotion);

//////////////////////////////////////////////////////////////////////////
// CFeatureMoveRelativeToEmitter

class CFeatureMoveRelativeToEmitter : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

		CFeatureMoveRelativeToEmitter()
		: m_positionInherit(1.0f)
		, m_velocityInherit(1.0f)
		, m_angularInherit(0.0f)
		, m_velocityInheritAfterDeath(0.0f) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		const bool inheritPositions = (m_positionInherit != 0.0f);
		const bool inheritVelocities = (m_velocityInherit != 0.0f);
		const bool inheritAngles = (m_angularInherit != 0.0f);
		const bool inheritVelOnDeath = (m_velocityInheritAfterDeath != 0.0f);

		if (inheritPositions || inheritVelocities || inheritAngles)
			pComponent->AddToUpdateList(EUL_PreUpdate, this);
		if (inheritPositions)
			pComponent->AddParticleData(EPVF_LocalPosition);
		if (inheritVelocities)
			pComponent->AddParticleData(EPVF_LocalVelocity);
		if (inheritAngles)
		{
			pComponent->AddParticleData(EPQF_Orientation);
			pComponent->AddParticleData(EPQF_LocalOrientation);
		}
		if (inheritVelOnDeath)
			pComponent->AddToUpdateList(EUL_Update, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_positionInherit, "PositionInherit", "Position Inherit");
		ar(m_velocityInherit, "VelocityInherit", "Velocity Inherit");
		ar(m_angularInherit, "AngularInherit", "Angular Inherit");
		ar(m_velocityInheritAfterDeath, "VelocityInheritAfterDeath", "Velocity Inherit After Death");
	}

	virtual void PreUpdate(const SUpdateContext& context) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		CParticleContainer& container = context.m_container;
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const auto parentStates = parentContainer.GetTIStream<uint8>(EPDT_State);
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		const IQuatStream parentOrientations = parentContainer.GetIQuatStream(EPQF_Orientation);
		const IVec3Stream localPositions = container.GetIVec3Stream(EPVF_LocalPosition);
		const IVec3Stream localVelocities = container.GetIVec3Stream(EPVF_LocalVelocity);
		const IQuatStream localOrientations = container.GetIQuatStream(EPQF_LocalOrientation);
		IOVec3Stream worldPositions = container.GetIOVec3Stream(EPVF_Position);
		IOVec3Stream worldVelocities = container.GetIOVec3Stream(EPVF_Velocity);
		IOQuatStream worldOrientations = container.GetIOQuatStream(EPQF_Orientation);

		const bool inheritPositions = (m_positionInherit != 0.0f);
		const bool inheritVelocities = (m_velocityInherit != 0.0f);
		const bool inheritAngles = (m_angularInherit != 0.0f);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const uint8 parentState = (parentId != gInvalidId) ? parentStates.Load(parentId) : ES_Dead;
			if (parentState == ES_Dead)
				continue;

			const Vec3 wParentPos = parentPositions.SafeLoad(parentId);
			const Quat parentOrientation = parentOrientations.SafeLoad(parentId);
			const QuatT parentToWorld = QuatT(wParentPos, parentOrientation);

			if (inheritPositions)
			{
				const Vec3 wPosition0 = worldPositions.Load(particleId);
				const Vec3 oPosition = localPositions.Load(particleId);
				const Vec3 wPosition1 = Lerp(
					wPosition0, parentToWorld * oPosition,
					m_positionInherit);
				worldPositions.Store(particleId, wPosition1);
			}

			if (inheritVelocities)
			{
				const Vec3 wVelocity0 = worldVelocities.Load(particleId);
				const Vec3 oVelocity = localVelocities.Load(particleId);
				const Vec3 wVelocity1 = Lerp(
					wVelocity0, parentToWorld.q * oVelocity,
					m_velocityInherit);
				worldVelocities.Store(particleId, wVelocity1);
			}

			if (inheritAngles)
			{
				const Quat wOrientation0 = worldOrientations.Load(particleId);
				const Quat oOrientation = localOrientations.Load(particleId);
				const Quat wOrientation1 = Lerp(
					wOrientation0, parentToWorld.q * oOrientation,
					m_angularInherit);
				worldOrientations.Store(particleId, wOrientation1);
			}
		}
		CRY_PFX2_FOR_END;
	}

	virtual void Update(const SUpdateContext& context) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		CParticleContainer& container = context.m_container;
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const auto parentStates = parentContainer.GetTIStream<uint8>(EPDT_State);
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
		IOVec3Stream worldVelocities = container.GetIOVec3Stream(EPVF_Velocity);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const uint8 parentState = (parentId != gInvalidId) ? parentStates.Load(parentId) : 0;
			if (parentState == ES_Expired)
			{
				const Vec3 wParentVelocity = parentVelocities.Load(parentId);
				const Vec3 wVelocity0 = worldVelocities.Load(particleId);
				const Vec3 wVelocity1 = wParentVelocity * m_velocityInheritAfterDeath + wVelocity0;
				worldVelocities.Store(particleId, wVelocity1);
			}
		}
		CRY_PFX2_FOR_END;
	}

private:
	SFloat m_positionInherit;
	SFloat m_velocityInherit;
	SFloat m_angularInherit;
	SFloat m_velocityInheritAfterDeath;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMoveRelativeToEmitter, "Motion", "MoveRelativeToEmitter", colorMotion);
CRY_PFX2_LEGACY_FEATURE(CParticleFeature, CFeatureMoveRelativeToEmitter, "VelocityMoveRelativeToEmitter");

}
