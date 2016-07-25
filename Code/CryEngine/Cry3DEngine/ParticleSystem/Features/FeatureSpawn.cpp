// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     29/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"
#include "ParamMod.h"
#include <CryRenderer/IGpuParticles.h>

CRY_PFX2_DBG

namespace pfx2
{

typedef TValue<float, USoftLimit<15000>> UFloatCount;

struct CRY_ALIGN(CRY_PFX2_PARTICLES_ALIGNMENT) SSpawnData
{
	float m_amount;
	float m_spawned;
	float m_delay;
	float m_duration;
	float m_timer;
};

// PFX2_TODO : not enough self documented code. Refactor!

template<typename Impl>
class CParticleFeatureSpawnBase : public CParticleFeature
{
protected:
	typedef CParamMod<SModInstanceCounter, UFloatCount> TCountParam;
	typedef CParamMod<SModInstanceTimer, UFloat10>      TTimeParams;

public:
	CParticleFeatureSpawnBase(gpu_pfx2::EGpuFeatureType gpuType)
		: m_amount(1.0f)
		, m_delay(0.0)
		, m_duration(0.0f)
		, m_useDelay(false)
		, m_useDuration(false)
		, CParticleFeature(gpuType) {}

	virtual EFeatureType GetFeatureType() override
	{
		return EFT_Spawn;
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_InitSubInstance, this);
		pComponent->AddToUpdateList(EUL_Spawn, this);
		m_spawnDataOff = pComponent->AddInstanceData(sizeof(SSpawnData));
		m_amount.AddToComponent(pComponent, this);
		m_delay.AddToComponent(pComponent, this);
		m_duration.AddToComponent(pComponent, this);

		if (auto pInt = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersSpawn params;
			params.amount = m_amount.GetBaseValue();
			params.delay = m_delay.GetBaseValue();
			params.duration = m_duration.GetBaseValue();
			params.useDelay = m_useDelay;
			params.useDuration = m_useDuration;
			pInt->SetParameters(params);
		}
	}

	virtual void InitSubInstance(CParticleComponentRuntime* pComponentRuntime, size_t firstInstance, size_t lastInstance) override
	{
		const size_t numInstances = lastInstance - firstInstance;
		SUpdateRange range;
		range.m_firstParticleId = 0;
		range.m_lastParticleId = numInstances;
		SUpdateContext context(pComponentRuntime, range);
		TFloatArray amounts(*context.m_pMemHeap, numInstances + CRY_PFX2_PARTICLESGROUP_STRIDE);
		TFloatArray delays(*context.m_pMemHeap, numInstances + CRY_PFX2_PARTICLESGROUP_STRIDE);
		TFloatArray durations(*context.m_pMemHeap, numInstances + CRY_PFX2_PARTICLESGROUP_STRIDE);
		m_amount.ModifyInit(context, amounts.data(), range);
		if (m_useDelay)
			m_delay.ModifyInit(context, delays.data(), range);
		if (m_useDuration)
			m_duration.ModifyInit(context, durations.data(), range);

		for (size_t i = 0; i < numInstances; ++i)
		{
			const size_t idx = i + firstInstance;
			SSpawnData* pSpawn = pComponentRuntime->GetSubInstanceData<SSpawnData>(idx, m_spawnDataOff);
			pSpawn->m_timer = 0.0f;
			pSpawn->m_spawned = 0.0f;
			pSpawn->m_amount = amounts[i];
			pSpawn->m_delay = m_useDelay ? delays[i] : 0.0f;
			pSpawn->m_duration = m_useDuration ? durations[i] : Impl::DefaultDuration();
		}
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleComponentRuntime& runtime = context.m_runtime;
		const size_t numInstances = runtime.GetNumInstances();
		SUpdateRange range;
		range.m_firstParticleId = 0;
		range.m_lastParticleId = numInstances;

		TFloatArray amounts(*context.m_pMemHeap, numInstances + CRY_PFX2_PARTICLESGROUP_STRIDE);
		for (size_t i = 0; i < numInstances; ++i)
		{
			SSpawnData* pSpawn = GetSpawnData(runtime, i);
			amounts[i] = pSpawn->m_amount * context.m_params.m_scaleParticleCount;
		}
		m_amount.ModifyUpdate(context, amounts.data(), range);

		static_cast<Impl const&>(*this).UpdateAmounts(context, amounts);

		const float dT = context.m_deltaTime;
		const float invDT = dT ? 1.0f / dT : 0.0f;

		for (size_t i = 0; i < numInstances; ++i)
		{
			SSpawnData* pSpawn = GetSpawnData(runtime, i);

			const float startTime = max(pSpawn->m_timer, pSpawn->m_delay);
			const float endTime = min(pSpawn->m_timer + dT, pSpawn->m_delay + pSpawn->m_duration);
			const float spawnTime = endTime - startTime;
			const float amount = amounts[i];

			if (spawnTime >= 0.0f && amount > 0.0f)
			{
				auto& instance = runtime.GetInstance(i);

				CParticleContainer::SSpawnEntry entry = {};
				entry.m_parentId = instance.m_parentId;
				entry.m_ageBegin = (pSpawn->m_timer - startTime) * invDT;

				const float spawnedBefore = pSpawn->m_spawned;

				static_cast<Impl const&>(*this).UpdateSpawnInfo(*pSpawn, entry, context, amount, spawnTime);

				entry.m_count = uint(pSpawn->m_spawned) - uint(spawnedBefore);

				const float spawnPast = 1.0f - (spawnedBefore - uint(spawnedBefore));
				entry.m_ageBegin += spawnPast * entry.m_ageIncrement - 1.0f;
				entry.m_fractionBegin = uint(spawnedBefore) * entry.m_fractionCounter;

				runtime.SpawnParticles(entry);
			}

			pSpawn->m_timer += context.m_deltaTime;
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		struct SEnabledValue
		{
			SEnabledValue(TTimeParams& value, bool& state)
				: m_value(value), m_state(state) {}
			TTimeParams& m_value;
			bool&        m_state;
			void         Serialize(Serialization::IArchive& ar)
			{
				ar(m_state, "State", "^");
				if (m_state)
					ar(m_value, "Value", "^");
				else
					ar(m_value, "Value", "!^");
			}
		};

		CParticleFeature::Serialize(ar);
		ar(m_amount, "Amount", "Amount");
		ar(SEnabledValue(m_delay, m_useDelay), "Delay", "Delay");
		ar(SEnabledValue(m_duration, m_useDuration), "Duration", "Duration");
	}

	void UpdateAmounts(const SUpdateContext& context, Array<float, uint> amounts) const {}

protected:
	SSpawnData* GetSpawnData(CParticleComponentRuntime& runtime, size_t idx) { return runtime.GetSubInstanceData<SSpawnData>(idx, m_spawnDataOff); }

private:
	TCountParam         m_amount;
	TTimeParams         m_delay;
	TTimeParams         m_duration;
	bool                m_useDelay;
	bool                m_useDuration;
	TInstanceDataOffset m_spawnDataOff;
};

class CFeatureSpawnCount : public CParticleFeatureSpawnBase<CFeatureSpawnCount>
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnCount() : CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_SpawnCount) {}

	ILINE static float DefaultDuration() { return 0.0f; }

	ILINE void         UpdateSpawnInfo(SSpawnData& spawn, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		if (spawn.m_duration)
		{
			const float spawned = amount * dt * rcp_fast(spawn.m_duration);
			spawn.m_spawned += spawned;
			entry.m_ageIncrement = rcp_safe(spawned);
		}
		else
		{
			spawn.m_spawned = amount;
		}
		entry.m_fractionCounter = rcp_fast(max(amount - 1.0f, 1.0f));
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnCount, "Spawn", "Count", defaultIcon, spawnColor);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESpawnRateMode,
                           ParticlesPerSecond,
                           _SecondPerParticle, SecondsPerParticle = _SecondPerParticle
                           )

class CFeatureSpawnRate : public CParticleFeatureSpawnBase<CFeatureSpawnRate>
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnRate()
		: m_mode(ESpawnRateMode::ParticlesPerSecond)
		, CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_SpawnRate)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		if (GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersSpawnMode params;
			params.mode = (m_mode == ESpawnRateMode::ParticlesPerSecond
			               ? gpu_pfx2::ESpawnRateMode::ParticlesPerSecond
			               : gpu_pfx2::ESpawnRateMode::SecondPerParticle);
			GetGpuInterface()->SetParameters(params);
		}

		CParticleFeatureSpawnBase::AddToComponent(pComponent, pParams);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
	}

	ILINE static float DefaultDuration() { return fHUGE; }

	ILINE void         UpdateSpawnInfo(SSpawnData& spawn, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		const float spawned = m_mode == ESpawnRateMode::ParticlesPerSecond ? dt * amount : dt / amount;
		spawn.m_spawned += spawned;
		entry.m_ageIncrement = rcp_safe(spawned);
		entry.m_fractionCounter = dt * rcp_safe(spawned * context.m_params.m_baseParticleLifeTime);
	}

private:
	ESpawnRateMode m_mode;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnRate, "Spawn", "Rate", defaultIcon, spawnColor);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESpawnDistanceMode,
                           ParticlesPerMeter,
                           MetersPerParticle
                           )

class CFeatureSpawnDistance : public CParticleFeatureSpawnBase<CFeatureSpawnDistance>
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnDistance()
		: CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_None)
		, m_mode(ESpawnDistanceMode::ParticlesPerMeter) {}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
	}

	ILINE static float DefaultDuration() { return fHUGE; }

	ILINE void         UpdateSpawnInfo(SSpawnData& spawn, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		const CParticleContainer& parentContainer = context.m_parentContainer;
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
		const Vec3 parentVelocity = parentVelocities.Load(entry.m_parentId);
		const float parentSpeed = parentVelocity.GetLength();
		const float distance = parentSpeed * dt;
		const float spawned = m_mode == ESpawnDistanceMode::ParticlesPerMeter ? distance * amount : distance / amount;
		spawn.m_spawned += spawned;
		entry.m_ageIncrement = rcp_safe(spawned);
	}

private:
	ESpawnDistanceMode m_mode;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDistance, "Spawn", "Distance", defaultIcon, spawnColor);

class CFeatureSpawnDensity : public CParticleFeatureSpawnBase<CFeatureSpawnDensity>
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnDensity()
		: CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_None)
	{}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
	}

	ILINE static float DefaultDuration() { return fHUGE; }

	void               UpdateAmounts(const SUpdateContext& context, Array<float, uint> amounts) const
	{
		CParticleComponentRuntime& runtime = context.m_runtime;
		const size_t numInstances = runtime.GetNumInstances();

		TFloatArray extents(*context.m_pMemHeap, numInstances + CRY_PFX2_PARTICLESGROUP_STRIDE);
		extents.fill(0.0f);
		runtime.GetSpatialExtents(context, amounts, extents);
		for (size_t i = 0; i < numInstances; ++i)
		{
			amounts[i] *= extents[i];
		}
	}

	ILINE void UpdateSpawnInfo(SSpawnData& spawn, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		const float spawned = context.m_params.m_baseParticleLifeTime > 0.0f ?
		                      amount * dt / context.m_params.m_baseParticleLifeTime :
		                      max(amount - spawn.m_spawned, 0.0f);
		spawn.m_spawned += spawned;
		entry.m_ageIncrement = rcp_safe(spawned);
		entry.m_fractionCounter = rcp_fast(max(amount - 1.0f, 1.0f));
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDensity, "Spawn", "Density", defaultIcon, spawnColor);

}
