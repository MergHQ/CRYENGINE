// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     27/10/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CrySerialization/Serializer.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/SmartPtr.h>
#include "ParticleSystem/ParticleFeature.h"
#include "ParticleSystem/ParticleEmitter.h"
#include "ParamMod.h"
#include "Target.h"
#include <CryMath/SNoise.h>

CRY_PFX2_DBG

namespace pfx2
{

class ILocalEffectors : public _i_reference_target_t
{
public:
	bool         IsEnabled() const                                                                                             { return m_enabled; }
	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)                                     {}
	virtual void ComputeEffector(const SUpdateContext& context, IOVec3Stream localVelocities, IOVec3Stream localAccelerations) {}
	virtual void Serialize(Serialization::IArchive& ar);
	virtual void SetParameters(gpu_pfx2::IParticleFeatureGpuInterface* gpuInterface) const = 0;
private:
	SEnable m_enabled;
};

void ILocalEffectors::Serialize(Serialization::IArchive& ar)
{
	ar(m_enabled);
}

//////////////////////////////////////////////////////////////////////////

EParticleDataType PDT(EPDT_Gravity, float, 1, BHasInit(true));
EParticleDataType PDT(EPDT_Drag, float, 1, BHasInit(true));
EParticleDataType PDT(EPVF_Acceleration, float, 3);
EParticleDataType PDT(EPVF_VelocityField, float, 3);

enum EIntegrator
{
	EI_Linear,
	EI_DragFast,
};

class CFeatureMotionPhysics : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureMotionPhysics()
		: m_gravity(0.0f)
		, m_drag(0.0f)
		, m_windMultiplier(1.0f)
		, m_uniformAcceleration(ZERO)
		, m_uniformWind(ZERO)
		, m_integrator(EI_Linear)
		, CParticleFeature(gpu_pfx2::eGpuFeatureType_Motion) {}

	virtual EFeatureType GetFeatureType() override { return EFT_Motion; }

	virtual void         AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_Update, this);

		m_gravity.AddToComponent(pComponent, this, EPDT_Gravity);
		m_drag.AddToComponent(pComponent, this, EPDT_Drag);

		for (auto& effector : m_localEffectors)
			effector->AddToComponent(pComponent, pParams);

		const bool hasDrag = m_drag.GetBaseValue() != 0.0f;
		const bool hasGravity = m_gravity.GetBaseValue() != 0.0f || !m_uniformAcceleration.IsZero();
		const bool hasEffectors = !m_localEffectors.empty();

		if (hasDrag || hasGravity || hasEffectors)
			m_integrator = EI_DragFast;
		else
			m_integrator = EI_Linear;

		if (auto pInt = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersMotionPhysics params;
			params.gravity = m_gravity.GetBaseValue();
			params.drag = m_drag.GetBaseValue();
			params.uniformAcceleration = m_uniformAcceleration;
			params.uniformWind = m_uniformWind;
			params.windMultiplier = m_windMultiplier;
			pInt->SetParameters(params);

			for (const auto& pEffector : m_localEffectors)
			{
				if (pEffector && pEffector->IsEnabled())
					pEffector->SetParameters(pInt);
			}
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_gravity, "gravity", "Gravity Scale");
		ar(m_drag, "drag", "Drag");
		ar(m_uniformAcceleration, "UniformAcceleration", "Uniform Acceleration");
		ar(m_uniformWind, "UniformWind", "Uniform Wind");
		ar(m_windMultiplier, "windMultiplier", "Level Wind Scale");
		ar(m_localEffectors, "localEffectors", "Local Effectors");
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		m_gravity.InitParticles(context, EPDT_Gravity);
		m_drag.InitParticles(context, EPDT_Drag);
	}

	virtual void Update(const SUpdateContext& context) override
	{
		CParticleContainer& container = context.m_container;
		m_gravity.Update(context, EPDT_Gravity);
		m_drag.Update(context, EPDT_Drag);

		IOVec3Stream velocityField = container.GetIOVec3Stream(EPVF_VelocityField);
		IOVec3Stream accelerations = container.GetIOVec3Stream(EPVF_Acceleration);
		for (auto& pEffector : m_localEffectors)
		{
			if (pEffector && pEffector->IsEnabled())
				pEffector->ComputeEffector(context, velocityField, accelerations);
		}

		Integrate(context);
	}

private:
	void Integrate(const SUpdateContext& context);
	void LinearIntegral(const SUpdateContext& context);
	void DragFastIntegral(const SUpdateContext& context);

	std::vector<_smart_ptr<ILocalEffectors>> m_localEffectors;
	CParamMod<SModParticleField, SFloat>     m_gravity;
	CParamMod<SModParticleField, UFloat>     m_drag;
	Vec3        m_uniformAcceleration;
	Vec3        m_uniformWind;
	UFloat      m_windMultiplier;
	EIntegrator m_integrator;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMotionPhysics, "Motion", "Physics", defaultIcon, motionColor);

//////////////////////////////////////////////////////////////////////////

void CFeatureMotionPhysics::Integrate(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	switch (m_integrator)
	{
	case EI_Linear:
		LinearIntegral(context);
		break;
	case EI_DragFast:
		DragFastIntegral(context);
		break;
	}
}

void CFeatureMotionPhysics::LinearIntegral(const SUpdateContext& context)
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

void CFeatureMotionPhysics::DragFastIntegral(const SUpdateContext& context)
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

		const Vec3v partAccel = accelerations.SafeLoad(particleGroupId);
		const Vec3v fieldVel = velocityField.SafeLoad(particleGroupId);
		const Vec3v partVel = Add(physWind, fieldVel);

		const floatv gravMult = gravities.SafeLoad(particleGroupId);
		const floatv drag = Mul(drags.SafeLoad(particleGroupId), dragReduction);
		const Vec3v p0 = positions.Load(particleGroupId);
		const Vec3v v0 = velocities.Load(particleGroupId);

		const Vec3v a = Add(MAdd(physAccel, gravMult, partAccel), uniformAccel);
		const Vec3v accel = MAdd(Sub(partVel, v0), drag, a);          // (partVel-v0)*drag + a
		const Vec3v p1 = MAdd(v0, dT, p0);                            // v0*dT + p0
		const Vec3v v1 = MAdd(accel, dT, v0);                         // accel*dT + v0

		positions.Store(particleGroupId, p1);
		velocities.Store(particleGroupId, v1);
	}
	CRY_PFX2_FOR_END;
}

//////////////////////////////////////////////////////////////////////////

EParticleDataType PDT(EPDT_PhysicalEntity, IPhysicalEntity*);

class CFeatureMotionCryPhysics : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureMotionCryPhysics()
		: m_gravity(1.0f)
		, m_drag(0.0f)
		, m_density(1.0f)
		, m_thickness(0.0f)
		, m_uniformAcceleration(ZERO) {}

	virtual EFeatureType GetFeatureType() override { return EFT_Motion; }

	virtual void         AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_PostInitUpdate, this);
		pComponent->AddToUpdateList(EUL_Update, this);
		pComponent->AddParticleData(EPDT_PhysicalEntity);
		pComponent->AddParticleData(EPVF_Position);
		pComponent->AddParticleData(EPVF_Velocity);
		pComponent->AddParticleData(EPVF_AngularVelocity);
		pComponent->AddParticleData(EPQF_Orientation);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_surfaceTypeName, "SurfaceType", "Surface Type");
		ar(m_gravity, "gravity", "Gravity Scale");
		ar(m_drag, "drag", "Drag");
		ar(m_density, "density", "Density");
		ar(m_thickness, "thickness", "Thickness");
		ar(m_uniformAcceleration, "UniformAcceleration", "Uniform Acceleration");
	}

	virtual void PostInitParticles(const SUpdateContext& context) override
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

	virtual void Update(const SUpdateContext& context) override
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

private:
	string m_surfaceTypeName;
	SFloat m_gravity;
	UFloat m_drag;
	SFloat m_density;
	SFloat m_thickness;
	Vec3   m_uniformAcceleration;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMotionCryPhysics, "Motion", "CryPhysics", defaultIcon, motionColor);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ETurbulenceMode,
                           Brownian,
                           Simplex,
                           SimplexCurl
                           )

class CEffectorTurbulence : public ILocalEffectors
{
private:
	typedef TValue<uint, THardLimits<1, 6>> UIntOctaves;

public:
	CEffectorTurbulence()
		: m_scale(1.0f, 1.0f, 1.0f)
		, m_speed(1.0f)
		, m_size(1.0f)
		, m_rate(0.0f)
		, m_octaves(1)
		, m_mode(ETurbulenceMode::Brownian) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
	{
		pComponent->AddParticleData(EPVF_Position);
		if (IsSimplex())
			pComponent->AddParticleData(EPVF_VelocityField);
		else
			pComponent->AddParticleData(EPVF_Acceleration);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		ILocalEffectors::Serialize(ar);
		const bool isUI = (ar.isEdit() && ar.isOutput());

		ar(m_mode, "mode", "Mode");

		ar(m_speed, "speed", "Speed");
		if (!isUI || IsSimplex())
		{
			ar(m_size, "size", "Size");
			ar(m_rate, "rate", "Rate");
			ar(m_octaves, "octaves", "Octaves");
		}
		ar(m_scale, "scale", "Scale");
	}

	virtual void SetParameters(gpu_pfx2::IParticleFeatureGpuInterface* gpuInterface) const
	{

		switch (m_mode)
		{
		case ETurbulenceMode::Brownian:
			{
				gpu_pfx2::SFeatureParametersMotionPhysicsBrownian params;
				params.scale = m_scale;
				params.speed = m_speed;
				gpuInterface->SetParameters(params);
			}
			break;
		case ETurbulenceMode::Simplex:
			{
				gpu_pfx2::SFeatureParametersMotionPhysicsSimplex params;
				params.scale = m_scale;
				params.speed = m_speed;
				params.size = m_size;
				params.rate = m_rate;
				params.octaves = m_octaves;
				gpuInterface->SetParameters(params);
			}
			break;
		case ETurbulenceMode::SimplexCurl:
			{
				gpu_pfx2::SFeatureParametersMotionPhysicsCurl params;
				params.scale = m_scale;
				params.speed = m_speed;
				params.size = m_size;
				params.rate = m_rate;
				params.octaves = m_octaves;
				gpuInterface->SetParameters(params);
			}
			break;
		default:
			CRY_ASSERT(0);
		}
	}

	virtual void ComputeEffector(const SUpdateContext& context, IOVec3Stream localVelocities, IOVec3Stream localAccelerations)
	{
		switch (m_mode)
		{
		case ETurbulenceMode::Brownian:
			Brownian(context, localAccelerations);
			break;
		case ETurbulenceMode::Simplex:
			ComputeSimplex(context, localVelocities, &Potential);
			break;
		case ETurbulenceMode::SimplexCurl:
			ComputeSimplex(context, localVelocities, &Curl);
			break;
		}
	}

private:
	void Brownian(const SUpdateContext& context, IOVec3Stream localAccelerations)
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		const CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const float time = max(1.0f / 1024.0f, context.m_deltaTime);
		const floatv speed = ToFloatv(m_speed * rsqrt(time));

		CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
		{
			const Vec3v position = positions.Load(particleGroupId);
			const Vec3v accel0 = localAccelerations.Load(particleGroupId);
			const floatv keyX = context.m_updateRngv.RandSNorm();
			const floatv keyY = context.m_updateRngv.RandSNorm();
			const floatv keyZ = context.m_updateRngv.RandSNorm();
			const Vec3v accel1 = MAdd(Vec3v(keyX, keyY, keyZ), speed, accel0);
			localAccelerations.Store(particleGroupId, accel1);
		}
		CRY_PFX2_FOR_END;
	}

	template<typename FieldFn>
	void ComputeSimplex(const SUpdateContext& context, IOVec3Stream localVelocities, FieldFn fieldFn)
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		CParticleContainer& container = context.m_container;
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		const float maxSize = (float)(1 << 12);
		const float minSize = rcp_fast(maxSize); // small enough and prevents SIMD exceptions
		const floatv time = ToFloatv(fmodf(context.m_time * m_rate * minSize, 1.0f) * maxSize);
		const floatv invSize = ToFloatv(rcp_fast(MAX(minSize, float(m_size))));
		const floatv speed = ToFloatv(m_speed);
		const uint octaves = m_octaves;
		const floatv scalex = ToFloatv(m_scale.x);
		const floatv scaley = ToFloatv(m_scale.y);
		const floatv scalez = ToFloatv(m_scale.z);

		CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
		{
			const Vec3v position = positions.Load(particleGroupId);
			const Vec3v velocity0 = localVelocities.Load(particleGroupId);

			Vec4v sample;
			sample.x = Mul(position.x, invSize);
			sample.y = Mul(position.y, invSize);
			sample.z = Mul(position.z, invSize);
			sample.w = time;

			Vec3v fieldSample = Fractal(sample, octaves, fieldFn);
			fieldSample.x = Mul(fieldSample.x, scalex);
			fieldSample.y = Mul(fieldSample.y, scaley);
			fieldSample.z = Mul(fieldSample.z, scalez);
			const Vec3v velocity1 = MAdd(fieldSample, speed, velocity0);
			localVelocities.Store(particleGroupId, velocity1);
		}
		CRY_PFX2_FOR_END;
	}

	ILINE static Vec3v Potential(const Vec4v sample)
	{
		const Vec4v offy = ToVec4v(Vec4(149, 311, 191, 491));
		const Vec4v offz = ToVec4v(Vec4(233, 197, 43, 59));
		const Vec3v potential = Vec3v(
		  SNoise(sample),
		  SNoise(Add(sample, offy)),
		  SNoise(Add(sample, offz)));
		return potential;
	}

	ILINE static Vec3v Curl(const Vec4v sample)
	{
		Vec4v gradX, gradY, gradZ;
		const Vec4v offy = ToVec4v(Vec4(149, 311, 191, 491));
		const Vec4v offz = ToVec4v(Vec4(233, 197, 43, 59));
		SNoise(sample, &gradX);
		SNoise(Add(sample, offy), &gradY);
		SNoise(Add(sample, offz), &gradZ);

		Vec3v curl;
		curl.x = Sub(gradY.z, gradZ.y);
		curl.y = Sub(gradZ.x, gradX.z);
		curl.z = Sub(gradX.y, gradY.x);

		return curl;
	}

	template<typename FieldFn>
	ILINE static Vec3v Fractal(const Vec4v sample, const uint octaves, FieldFn fieldFn)
	{
		Vec3v total = ToVec3v(Vec3(ZERO));
		floatv mult = ToFloatv(1.0f);
		floatv totalMult = ToFloatv(0.0f);
		for (uint i = 0; i < octaves; ++i)
		{
			totalMult = Add(mult, totalMult);
			mult = Mul(ToFloatv(0.5f), mult);
		}
		mult = rcp_fast(totalMult);
		for (uint i = 0; i < octaves; ++i)
		{
			total = MAdd(fieldFn(sample), mult, total);
			mult = Mul(mult, ToFloatv(0.5f));
		}
		return total;
	}

	ILINE bool IsSimplex() const
	{
		return (m_mode == ETurbulenceMode::Simplex) || (m_mode == ETurbulenceMode::SimplexCurl);
	}

	Vec3            m_scale;
	UFloat10        m_speed;
	UFloat10        m_size;
	UFloat10        m_rate;
	UIntOctaves     m_octaves;
	ETurbulenceMode m_mode;
};

SERIALIZATION_CLASS_NAME(ILocalEffectors, CEffectorTurbulence, "Turbulence", "Turbulence");

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(EGravityType,
                           Spherical,
                           Cylindrical
                           )

class CEffectorGravity : public ILocalEffectors
{
public:
	CEffectorGravity()
		: m_acceleration(1.0f)
		, m_decay(10.0f)
		, m_type(EGravityType::Spherical)
		, m_axis(0.0f, 0.0f, 1.0f) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
	{
		pComponent->AddParticleData(EPVF_Position);
		pComponent->AddParticleData(EPVF_Acceleration);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		ILocalEffectors::Serialize(ar);
		ar(m_targetSource, "Target", "Target");
		ar(m_type, "Type", "Type");
		ar(m_acceleration, "Acceleration", "Acceleration");
		ar(m_decay, "Decay", "Decay");
		if (m_type == EGravityType::Cylindrical)
			ar(m_axis, "Axis", "Axis");
	}

	virtual void SetParameters(gpu_pfx2::IParticleFeatureGpuInterface* gpuInterface) const
	{
		gpu_pfx2::SFeatureParametersMotionPhysicsGravity params;
		params.gravityType =
		  (
		    (m_type == EGravityType::Cylindrical) ?
		    gpu_pfx2::eGravityType_Cylindrical :
		    gpu_pfx2::eGravityType_Spherical
		  );
		params.acceleration = m_acceleration;
		params.decay = m_decay;
		params.axis = m_axis;
		gpuInterface->SetParameters(params);
	}

	virtual void ComputeEffector(const SUpdateContext& context, IOVec3Stream localVelocities, IOVec3Stream localAccelerations)
	{
		switch (m_type)
		{
		case EGravityType::Spherical:
			ComputeGravity<false>(context, localAccelerations);
			break;
		case EGravityType::Cylindrical:
			ComputeGravity<true>(context, localAccelerations);
			break;
		}
	}

private:
	template<const bool useAxis>
	ILINE void ComputeGravity(const SUpdateContext& context, IOVec3Stream localAccelerations)
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		const CParticleContainer& container = context.m_container;
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const Quat defaultQuat = context.m_runtime.GetEmitter()->GetLocation().q;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		const IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		// m_decay is actually the distance at which gravity is halved.
		const float decay = rcp_fast(m_decay * m_decay);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			if (parentId != gInvalidId)
			{
				const Vec3 position = positions.Load(particleId);
				const Vec3 targetPosition = m_targetSource.GetTarget(context, particleId);
				const Vec3 accel0 = localAccelerations.Load(particleId);

				Vec3 accelVec;
				if (useAxis)
				{
					const Quat wQuat = parentQuats.SafeLoad(parentId);
					const Vec3 axis = wQuat * m_axis;
					accelVec = (targetPosition + axis * axis.Dot(position - targetPosition)) - position;
				}
				else
				{
					accelVec = targetPosition - position;
				}

				const float distanceSqr = accelVec.GetLengthSquared();
				const float gravity = rcp_fast(1.0f + decay * distanceSqr) * m_acceleration;
				const Vec3 accel1 = accel0 + accelVec.GetNormalized() * gravity;
				localAccelerations.Store(particleId, accel1);
			}
		}
		CRY_PFX2_FOR_END;
	}

	CTargetSource m_targetSource;
	Vec3          m_axis;
	SFloat10      m_acceleration;
	UFloat100     m_decay;
	EGravityType  m_type;
};

SERIALIZATION_CLASS_NAME(ILocalEffectors, CEffectorGravity, "Gravity", "Gravity");

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(EVortexDirection,
                           ClockWise,
                           CounterClockWise
                           )

class CEffectorVortex : public ILocalEffectors
{
public:
	CEffectorVortex()
		: m_speed(1.0f)
		, m_decay(10.0f)
		, m_direction(EVortexDirection::ClockWise)
		, m_axis(0.0f, 0.0f, 1.0f) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
	{
		pComponent->AddParticleData(EPVF_Position);
		pComponent->AddParticleData(EPVF_VelocityField);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		ILocalEffectors::Serialize(ar);
		ar(m_targetSource, "Target", "Target");
		ar(m_speed, "Speed", "Speed");
		ar(m_decay, "Decay", "Decay");
		ar(m_direction, "Direction", "Direction");
		ar(m_axis, "Axis", "Axis");
	}

	virtual void SetParameters(gpu_pfx2::IParticleFeatureGpuInterface* gpuInterface) const
	{
		gpu_pfx2::SFeatureParametersMotionPhysicsVortex params;
		params.vortexDirection =
		  (
		    (m_direction == EVortexDirection::ClockWise) ?
		    gpu_pfx2::eVortexDirection_Clockwise :
		    gpu_pfx2::eVortexDirection_CounterClockwise
		  );
		params.speed = m_speed;
		params.decay = m_decay;
		params.axis = m_axis;
		gpuInterface->SetParameters(params);
	}

	virtual void ComputeEffector(const SUpdateContext& context, IOVec3Stream localVelocities, IOVec3Stream localAccelerations)
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		const CParticleContainer& container = context.m_container;
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const Quat defaultQuat = context.m_runtime.GetEmitter()->GetLocation().q;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		// m_decay is actually the distance at which gravity is halved.
		const float decay = rcp_fast(m_decay * m_decay);
		const float speed = m_speed * (m_direction == EVortexDirection::ClockWise ? -1.0f : 1.0f);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			if (parentId != gInvalidId)
			{
				const Vec3 position = positions.Load(particleId);
				const Vec3 targetPosition = m_targetSource.GetTarget(context, particleId);
				const Quat wQuat = parentQuats.SafeLoad(parentId);
				const Vec3 velocity0 = localVelocities.Load(particleId);

				const Vec3 axis = wQuat * m_axis;
				const Vec3 toAxis = (targetPosition + axis * axis.Dot(position - targetPosition)) - position;
				const float distanceSqr = toAxis.GetLengthSquared();
				const float vortexSpeed = rcp_fast(1.0f + decay * distanceSqr) * speed;
				const Vec3 velocity1 = velocity0 + toAxis.GetNormalized().Cross(axis) * vortexSpeed;
				localVelocities.Store(particleId, velocity1);
			}
		}
		CRY_PFX2_FOR_END;
	}

private:
	CTargetSource    m_targetSource;
	Vec3             m_axis;
	UFloat10         m_speed;
	UFloat100        m_decay;
	EVortexDirection m_direction;
};

SERIALIZATION_CLASS_NAME(ILocalEffectors, CEffectorVortex, "Vortex", "Vortex");

}
