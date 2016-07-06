// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     29/10/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CrySerialization/Math.h>
#include "ParticleSystem/ParticleFeature.h"
#include "ParticleSystem/ParticleEmitter.h"
#include "ParamMod.h"

CRY_PFX2_DBG

volatile bool gFeatureVelocity = false;

namespace pfx2
{

class CFeatureVelocityCone : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureVelocityCone()
		: m_velocity(0.0f)
		, m_angle(0.0f)
		, CParticleFeature(gpu_pfx2::eGpuFeatureType_VelocityCone)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_InitUpdate, this);
		m_angle.AddToComponent(pComponent, this);
		m_velocity.AddToComponent(pComponent, this);

		if (auto pInt = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersVelocityCone params;
			params.angle = m_angle.GetBaseValue();
			params.velocity = m_velocity.GetBaseValue();
			pInt->SetParameters(params);
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_angle, "Angle", "Angle");
		ar(m_velocity, "Velocity", "Velocity");
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& parentContainer = context.m_parentContainer;
		CParticleContainer& container = context.m_container;
		const Quat defaultQuat = context.m_runtime.GetEmitter()->GetLocation().q;
		IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
		const float baseAngle = m_angle.GetBaseValue();
		const float invBaseAngle = rcp_fast(min(-FLT_EPSILON, baseAngle));

		STempModBuffer angles(context, m_angle);
		STempModBuffer velocityMults(context, m_velocity);
		angles.ModifyInit(context, m_angle, container.GetSpawnedRange());
		velocityMults.ModifyInit(context, m_velocity, container.GetSpawnedRange());

		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const Vec3 wVelocity0 = velocities.Load(particleId);
			const Quat wQuat = parentQuats.SafeLoad(parentId);
			const float angleMult = angles.m_stream.SafeLoad(particleId);
			const float velocity = velocityMults.m_stream.SafeLoad(particleId);

			const Vec2 disc = SChaosKey().RandCircle();
			const float angle = sqrt(angleMult * invBaseAngle) * baseAngle;

			float as, ac;
			sincos(angle, &as, &ac);
			const Vec3 dir = Vec3(disc.x * as, disc.y * as, ac);
			const Vec3 oVelocity = dir * velocity;
			const Vec3 wVelocity1 = wVelocity0 + wQuat * oVelocity;
			velocities.Store(particleId, wVelocity1);
		}
		CRY_PFX2_FOR_END;
	}

private:
	CParamMod<SModParticleSpawnInit, UAngle180> m_angle;
	CParamMod<SModParticleSpawnInit, UFloat10>  m_velocity;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureVelocityCone, "Velocity", "Cone", defaultIcon, defaultColor);

//////////////////////////////////////////////////////////////////////////

class CFeatureVelocityDirectional : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureVelocityDirectional()
		: m_direction(ZERO)
		, m_scale(1.0f)
		, CParticleFeature(gpu_pfx2::eGpuFeatureType_VelocityDirectional)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_InitUpdate, this);
		m_scale.AddToComponent(pComponent, this);

		if (auto pGpu = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersVelocityDirectional params;
			params.direction = m_direction;
			params.scale = m_scale.GetBaseValue();
			pGpu->SetParameters(params);
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_direction, "Velocity", "Velocity");
		ar(m_scale, "Scale", "Scale");
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& parentContainer = context.m_parentContainer;
		CParticleContainer& container = context.m_container;
		const Quat defaultQuat = context.m_runtime.GetEmitter()->GetLocation().q;
		IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
		IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		STempModBuffer scales(context, m_scale);
		scales.ModifyInit(context, m_scale, container.GetSpawnedRange());

		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const Quat wQuat = parentQuats.SafeLoad(parentId);
			const float scale = scales.m_stream.SafeLoad(particleId);
			const Vec3 wVelocity0 = velocities.Load(particleId);
			const Vec3 oVelocity = m_direction * scale;
			const Vec3 wVelocity1 = wVelocity0 + wQuat * oVelocity;
			velocities.Store(particleId, wVelocity1);
		}
		CRY_PFX2_FOR_END;
	}

private:
	Vec3 m_direction;
	CParamMod<SModParticleSpawnInit, UFloat10> m_scale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureVelocityDirectional, "Velocity", "Directional", defaultIcon, defaultColor);

//////////////////////////////////////////////////////////////////////////

class CFeatureVelocityOmniDirectional : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureVelocityOmniDirectional()
		: m_velocity(0.0f)
		, CParticleFeature(gpu_pfx2::eGpuFeatureType_VelocityOmniDirectional)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_InitUpdate, this);
		m_velocity.AddToComponent(pComponent, this);

		if (auto pGpu = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersVelocityOmniDirectional params;
			params.velocity = m_velocity.GetBaseValue();
			pGpu->SetParameters(params);
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_velocity, "Velocity", "Velocity");
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& container = context.m_container;
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);

		STempModBuffer velocityMults(context, m_velocity);
		velocityMults.ModifyInit(context, m_velocity, container.GetSpawnedRange());

		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const Vec3 wVelocity0 = velocities.Load(particleId);
			const float velocity = velocityMults.m_stream.SafeLoad(particleId);
			const Vec3 oVelocity = SChaosKey().RandSphere() * velocity;
			const Vec3 wVelocity1 = wVelocity0 + oVelocity;
			velocities.Store(particleId, wVelocity1);
		}
		CRY_PFX2_FOR_END;
	}

private:
	CParamMod<SModParticleSpawnInit, UFloat10> m_velocity;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureVelocityOmniDirectional, "Velocity", "OmniDirectional", defaultIcon, defaultColor);

//////////////////////////////////////////////////////////////////////////

class CFeatureVelocityInherit : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureVelocityInherit()
		: m_scale(1.0f)
		, CParticleFeature(gpu_pfx2::eGpuFeatureType_VelocityInherit) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_InitUpdate, this);
		m_scale.AddToComponent(pComponent, this);

		if (auto gpuInt = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersScale params;
			params.scale = m_scale.GetBaseValue();
			gpuInt->SetParameters(params);
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_scale, "Scale", "Scale");
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		if (context.m_parentContainer.HasData(EPDT_AngularVelocityX))
			InheritVelocity<true>(context);
		else
			InheritVelocity<false>(context);
	}

	template<bool hasRotation>
	void InheritVelocity(const SUpdateContext& context)
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& parentContainer = context.m_parentContainer;
		CParticleContainer& container = context.m_container;
		IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
		const IVec3Stream parentAngularVelocities = parentContainer.GetIVec3Stream(EPVF_AngularVelocity);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);

		STempModBuffer scales(context, m_scale);
		scales.ModifyInit(context, m_scale, container.GetSpawnedRange());

		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const float scale = scales.m_stream.SafeLoad(particleId);
			const TParticleId parentId = parentIds.Load(particleId);
			const Vec3 wVelocity0 = velocities.Load(particleId);
			Vec3 wInheritVelocity = parentVelocities.Load(parentId);
			if (hasRotation)
			{
				const Vec3 wPos = positions.Load(particleId);
				const Vec3 wParentPos = parentPositions.Load(parentId);
				const Vec3 wParentAngularVelocity = parentAngularVelocities.Load(parentId);
				wInheritVelocity += wParentAngularVelocity ^ (wPos - wParentPos);
			}
			const Vec3 wVelocity1 = wVelocity0 + wInheritVelocity * scale;
			velocities.Store(particleId, wVelocity1);
		}
		CRY_PFX2_FOR_END;
	}

private:
	CParamMod<SModParticleSpawnInit, SFloat> m_scale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureVelocityInherit, "Velocity", "Inherit", defaultIcon, defaultColor);

//////////////////////////////////////////////////////////////////////////

class CFeatureMoveRelativeToEmitter : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureMoveRelativeToEmitter()
		: m_positionInherit(1.0f)
		, m_velocityInheritAfterDeath(0.0f) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_Update, this);
		pComponent->AddParticleData(EPVF_Position);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_positionInherit, "PositionInherit", "Position Inherit");
		ar(m_velocityInheritAfterDeath, "VelocityInheritAfterDeath", "Velocity Inherit After Death");
	}

	virtual void Update(const SUpdateContext& context) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		if (abs(m_positionInherit) > FLT_EPSILON)
		{
			const bool parentsHaveAngles3D = context.m_parentContainer.HasData(EPDT_OrientationX);
			const bool childrenHaveAngles3D = context.m_container.HasData(EPDT_OrientationX);
			if (parentsHaveAngles3D && childrenHaveAngles3D)
				MoveRelativeToEmitter<true>(context);
			else
				MoveRelativeToEmitter<false>(context);
		}

		if (abs(m_velocityInheritAfterDeath) > FLT_EPSILON)
			InheritParentVelocity(context);
	}

private:
	template<const bool UpdateAngles>
	void MoveRelativeToEmitter(const SUpdateContext& context)
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& container = context.m_container;
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		IOQuatStream orientations = container.GetIOQuatStream(EPQF_Orientation);
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
		const IVec3Stream parentAngularVelocities = parentContainer.GetIVec3Stream(EPVF_AngularVelocity);
		const IQuatStream parentOrientations = parentContainer.GetIQuatStream(EPQF_Orientation);
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const auto parentStates = parentContainer.GetTIStream<uint8>(EPDT_State);
		const float deltaTime = context.m_deltaTime;
		const float positionInherit = m_positionInherit;

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const uint8 parentState = (parentId != gInvalidId) ? parentStates.Load(parentId) : ES_Expired;
			if (parentState == ES_Expired)
				continue;

			const Vec3 parentPosition = parentPositions.SafeLoad(parentId);
			const Vec3 parentVelocity = parentVelocities.SafeLoad(parentId);
			const Vec3 parentAngularVelocity = parentAngularVelocities.SafeLoad(parentId);
			const Quat parentOrientation = parentOrientations.SafeLoad(parentId);

			const QuatT worldToParent = QuatT(-parentPosition, parentOrientation.GetInverted());
			const QuatT parentToWorld = QuatT(parentPosition, parentOrientation);

			const Quat deltaQuat = Quat::CreateRotationXYZ(Ang3(parentAngularVelocity * deltaTime));
			const Vec3 wPosition0 = positions.Load(particleId);
			const Vec3 pos = deltaQuat * ((wPosition0 - parentPosition) + parentVelocity * deltaTime);
			const Vec3 wPosition1 = pos + parentPosition;

			if (UpdateAngles)
			{
				const Quat wOrientation0 = orientations.Load(particleId);
				const Quat wOrientation1 = deltaQuat * wOrientation0;
				orientations.Store(particleId, wOrientation1);
			}

			positions.Store(particleId, wPosition1);
		}
		CRY_PFX2_FOR_END;
	}

	void InheritParentVelocity(const SUpdateContext& context)
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& container = context.m_container;
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const auto parentStates = parentContainer.GetTIStream<uint8>(EPDT_State);
		auto states = container.GetTIOStream<uint8>(EPDT_State);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const uint8 parentState = (parentId != gInvalidId) ? parentStates.Load(parentId) : 0;
			if (parentState == ES_Expired)
			{
				const Vec3 parentVelocity = parentVelocities.Load(parentId);
				const Vec3 velocity0 = velocities.Load(particleId);
				const Vec3 velocity1 = parentVelocity * m_velocityInheritAfterDeath + velocity0;
				velocities.Store(particleId, velocity1);
			}
		}
		CRY_PFX2_FOR_END;
	}

	SFloat m_positionInherit;
	SFloat m_velocityInheritAfterDeath;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureMoveRelativeToEmitter, "Velocity", "MoveRelativeToEmitter", defaultIcon, defaultColor);

}
