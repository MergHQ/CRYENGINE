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
	float m_restart;
	float m_timer;
};

// PFX2_TODO : not enough self documented code. Refactor!

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
		, m_restart(1.0f)
		, m_useDuration(false)
		, m_useRestart(false)
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
		m_restart.AddToComponent(pComponent, this);

		if (auto pInt = GetGpuInterface())
		{
			gpu_pfx2::SFeatureParametersSpawn params;
			params.amount = m_amount.GetBaseValue();
			params.delay = m_delay.GetBaseValue();
			params.duration = m_duration.GetBaseValue();
			params.restart = m_restart.GetBaseValue();
			params.useDelay = m_delay.GetBaseValue() > 0.0f;
			params.useDuration = m_useDuration;
			params.useRestart = m_useRestart;
			pInt->SetParameters(params);
		}
	}

	virtual void InitSubInstance(CParticleComponentRuntime* pComponentRuntime, size_t firstInstance, size_t lastInstance) override
	{
		SUpdateRange range;
		range.m_firstParticleId = 0;
		range.m_lastParticleId = lastInstance - firstInstance;
		SUpdateContext context(pComponentRuntime, range);
		RestartInstances(context, firstInstance, lastInstance, true);
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

		if (ar.isInput() && GetVersion(ar) < 9)
		{
			bool useDelay = false;
			ar(SEnabledValue(m_delay, useDelay), "Delay", "Delay");
			if (!useDelay)
				m_delay = TTimeParams(0.0f);
		}
		else
		{
			ar(m_delay, "Delay", "Delay");
		}
		ar(SEnabledValue(m_duration, m_useDuration), "Duration", "Duration");
		if (m_useDuration)
			ar(SEnabledValue(m_restart, m_useRestart), "Restart", "Restart");
	}

	void UpdateAmounts(const SUpdateContext& context, Array<float, uint> amounts) const {}

protected:
	void SetUseDuration(bool value) { m_useDuration = value; }

	template<typename Impl>
	void SpawnParticlesT(Impl& impl, const SUpdateContext& context)
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleComponentRuntime& runtime = context.m_runtime;
		const size_t numInstances = runtime.GetNumInstances();
		if (numInstances == 0)
			return;

		const float dT = context.m_deltaTime;
		const float invDT = dT ? 1.0f / dT : 0.0f;
		SUpdateRange range(0, numInstances);

		if (m_useRestart)
			RestartInstances(context, 0, numInstances + 1, false);

		TFloatArray amounts(*context.m_pMemHeap, numInstances);
		for (size_t i = 0; i < numInstances; ++i)
		{
			SSpawnData* pSpawn = GetSpawnData(runtime, i);
			amounts[i] = pSpawn->m_amount * context.m_params.m_scaleParticleCount;
		}
		m_amount.ModifyUpdate(context, amounts.data(), range);

		impl.UpdateAmounts(context, amounts);

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

				impl.UpdateSpawnInfo(*pSpawn, i, entry, context, amount, spawnTime);

				entry.m_count = uint(ceil(pSpawn->m_spawned) - ceil(spawnedBefore));

				const float spawnPast = 1.0f - (spawnedBefore - floor(spawnedBefore));
				entry.m_ageBegin += spawnPast * entry.m_ageIncrement - 1.0f;
				entry.m_fractionBegin = floor(spawnedBefore) * entry.m_fractionCounter + entry.m_fractionCounter;

				runtime.SpawnParticles(entry);
			}

			pSpawn->m_timer += context.m_deltaTime;
		}
	}

	void RestartInstances(const SUpdateContext& context, size_t firstInstance, size_t lastInstance, bool restartAll)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const size_t numInstances = lastInstance - firstInstance;
		CParticleComponentRuntime& runtime = context.m_runtime;

		typedef TParticleHeap::Array<size_t> TIndicesArray;
		TIndicesArray indicesArray(*context.m_pMemHeap);
		indicesArray.reserve(numInstances);
		if (restartAll)
		{
			indicesArray.resize(numInstances);
			for (size_t i = 0; i < numInstances; ++i)
				indicesArray[i] = firstInstance + i;
		}
		else
		{
			for (size_t i = 0; i < numInstances; ++i)
			{
				const size_t idx = i + firstInstance;
				SSpawnData* pSpawn = GetSpawnData(runtime, idx);
				pSpawn->m_restart -= context.m_deltaTime;
				if (pSpawn->m_restart <= 0.0f)
					indicesArray.push_back(idx);
			}
		}

		if (indicesArray.empty())
			return;

		const uint numRestarts = indicesArray.size();
		SUpdateRange restartRange;
		restartRange.m_firstParticleId = 0;
		restartRange.m_lastParticleId = numRestarts;
		TFloatArray amounts(*context.m_pMemHeap, numRestarts);
		TFloatArray delays(*context.m_pMemHeap, numRestarts);
		TFloatArray durations(*context.m_pMemHeap, numRestarts);
		TFloatArray restarts(*context.m_pMemHeap, numRestarts);
		m_amount.ModifyInit(context, amounts.data(), restartRange);
		m_delay.ModifyInit(context, delays.data(), restartRange);
		if (m_useDuration)
			m_duration.ModifyInit(context, durations.data(), restartRange);
		if (m_useRestart)
			m_restart.ModifyInit(context, restarts.data(), restartRange);

		for (size_t i = 0; i < numRestarts; ++i)
		{
			const size_t idx = indicesArray[i];
			SSpawnData* pSpawn = GetSpawnData(runtime, idx);
			pSpawn->m_timer = 0.0f;
			pSpawn->m_spawned = -FLT_EPSILON;
			pSpawn->m_amount = amounts[i];
			pSpawn->m_delay = delays[i];
			pSpawn->m_duration = m_useDuration ? durations[i] : gInfinity;
			pSpawn->m_restart = m_useRestart ? restarts[i] : gInfinity;
		}
	}

private:
	SSpawnData* GetSpawnData(CParticleComponentRuntime& runtime, size_t idx) { return runtime.GetSubInstanceData<SSpawnData>(idx, m_spawnDataOff); }

private:
	TCountParam         m_amount;
	TTimeParams         m_delay;
	TTimeParams         m_duration;
	TTimeParams         m_restart;
	TInstanceDataOffset m_spawnDataOff;
	bool                m_useDuration;
	bool                m_useRestart;
};

class CFeatureSpawnCount : public CParticleFeatureSpawnBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnCount()
		: CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_SpawnCount)
	{
		// Default duration = 0 for SpawnCount
		SetUseDuration(true);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		if (GetVersion(ar) < 9)
		{
			// Infinite lifetime was impossible. Default disabled behavior was lifetime = 0.
			SetUseDuration(true);
		}
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	ILINE void UpdateSpawnInfo(SSpawnData& spawn, size_t instanceId, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		const float spawnTime = min(context.m_params.m_baseParticleLifeTime, spawn.m_duration);
		if (spawnTime > 0.0f)
		{
			const float rate = amount * rcp_fast(spawnTime);
			spawn.m_spawned += rate * dt;
			entry.m_ageIncrement = rcp_safe(rate * context.m_deltaTime);
		}
		else
		{
			spawn.m_spawned = amount;
			entry.m_ageIncrement = 0.0f;
		}
		entry.m_fractionCounter = rcp_fast(max(amount - 1.0f, 1.0f));
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnCount, "Spawn", "Count", colorSpawn);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESpawnRateMode,
                           ParticlesPerSecond,
                           _SecondPerParticle, SecondsPerParticle = _SecondPerParticle
                           )

class CFeatureSpawnRate : public CParticleFeatureSpawnBase
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

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	ILINE void UpdateSpawnInfo(SSpawnData& spawn, size_t instanceId, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		const float spawned = dt * (m_mode == ESpawnRateMode::ParticlesPerSecond ? amount : rcp(amount));
		spawn.m_spawned += spawned;
		entry.m_ageIncrement = rcp_safe(spawned);
	}

private:
	ESpawnRateMode m_mode;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnRate, "Spawn", "Rate", colorSpawn);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESpawnDistanceMode,
    ParticlesPerMeter,
    MetersPerParticle
)

class CFeatureSpawnDistance : public CParticleFeatureSpawnBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnDistance()
		: CParticleFeatureSpawnBase(gpu_pfx2::eGpuFeatureType_None)
		, m_mode(ESpawnDistanceMode::ParticlesPerMeter)
		{}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CParticleFeatureSpawnBase::AddToComponent(pComponent, pParams);
		m_emitPosOffset = pComponent->AddInstanceData(sizeof(Vec3));
	}
	virtual void InitSubInstance(CParticleComponentRuntime* pComponentRuntime, size_t firstInstance, size_t lastInstance) override
	{
		CParticleFeatureSpawnBase::InitSubInstance(pComponentRuntime, firstInstance, lastInstance);
		for (size_t inst = firstInstance; inst < lastInstance; ++inst)
			*GetEmitPosition(*pComponentRuntime, inst) = Vec3(gInfinity);
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	void UpdateSpawnInfo(SSpawnData& spawn, size_t instanceId, CParticleContainer::SSpawnEntry& entry, const SUpdateContext& context, float amount, float dt) const
	{
		CParticleContainer& parentContainer = context.m_parentContainer;
		const TParticleId parentId = entry.m_parentId;

		QuatT parentLoc;
		parentLoc.t = parentContainer.GetIVec3Stream(EPVF_Position).SafeLoad(parentId);
		parentLoc.q = parentContainer.GetIQuatStream(EPQF_Orientation).SafeLoad(parentId);
		Vec3 emitOffset(0);
		for (auto& it : context.m_runtime.GetComponent()->GetUpdateList(EUL_GetEmitOffset))
			emitOffset += it->GetEmitOffset(context, entry.m_parentId);
		const Vec3 emitPos1 = parentLoc * emitOffset;

		Vec3* pEmitPos = GetEmitPosition(context.m_runtime, instanceId);
		const Vec3 emitPos0 = *pEmitPos;
		*pEmitPos = emitPos1;
		if (!std::isfinite(emitPos0.x))
			return;

		const float distance = (emitPos1 - emitPos0).GetLengthFast();
		const float spawned = distance * (m_mode == ESpawnDistanceMode::ParticlesPerMeter ? amount : rcp(amount));
		spawn.m_spawned += spawned;
		entry.m_ageIncrement = rcp_safe(spawned);
	}

private:
	ESpawnDistanceMode m_mode;
	TInstanceDataOffset m_emitPosOffset;

	Vec3* GetEmitPosition(CParticleComponentRuntime& runtime, size_t idx) const { return runtime.GetSubInstanceData<Vec3>(idx, m_emitPosOffset); }
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDistance, "Spawn", "Distance", colorSpawn);

class CFeatureSpawnDensity : public CFeatureSpawnCount
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureSpawnDensity()
	{
		SetUseDuration(false);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	void UpdateAmounts(const SUpdateContext& context, Array<float, uint> amounts) const
	{
		CParticleComponentRuntime& runtime = context.m_runtime;
		const size_t numInstances = runtime.GetNumInstances();

		TFloatArray extents(*context.m_pMemHeap, numInstances);
		extents.fill(0.0f);
		runtime.GetSpatialExtents(context, amounts, extents);
		for (size_t i = 0; i < numInstances; ++i)
		{
			amounts[i] = extents[i];
		}
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDensity, "Spawn", "Density", colorSpawn);

}
