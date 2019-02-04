// Copyright 2015-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCommon.h"
#include "ParamMod.h"

namespace pfx2
{

struct SSpawnData
{
	float m_amount;
	float m_duration;
	float m_restart;
	float m_timer;
	float m_spawned;
	float m_fraction;

	Range SpawnTimes(const CParticleComponentRuntime& runtime) const
	{
		float dT = runtime.DeltaTime();
		const float endTime = min(dT, m_duration - m_timer);
		const float startTime = max(0.0f, endTime - runtime.ComponentParams().m_stableTime);
		return Range(startTime, endTime);
	}
};

// PFX2_TODO : not enough self documented code. Refactor!

MakeDataType(ESDT_ParticleCounts, SMaxParticleCounts, EDD_None);

class CParticleFeatureSpawnBase : public CParticleFeature
{
public:
	virtual EFeatureType GetFeatureType() override
	{
		return EFT_Spawn;
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddSubInstances.add(this);
		pComponent->InitSubInstances.add(this);
		pComponent->SpawnParticles.add(this);
		pComponent->GetDynamicData.add(this);
		pComponent->OnEdit.add(this);
		m_offsetSpawnData = pComponent->AddInstanceData<SSpawnData>();
		m_amount.AddToComponent(pComponent, this);
		m_delay.AddToComponent(pComponent, this);
		m_duration.AddToComponent(pComponent, this);
		m_restart.AddToComponent(pComponent, this);

		// Compute equilibrium and full life times
		CParticleComponent* pParent = pComponent->GetParentComponent();
		const float parentParticleLife = pParent ? pParent->ComponentParams().m_maxParticleLife : gInfinity;
		const float preDelay = pParams->m_maxTotalLIfe;
		const float delay = preDelay + m_delay.GetValueRange().end;

		STimingParams timings;
		timings.m_equilibriumTime = delay;
		timings.m_maxTotalLIfe = min(delay + m_duration.GetValueRange().end, parentParticleLife);

		if (m_restart.IsEnabled())
		{
			timings.m_stableTime = timings.m_maxTotalLIfe;
			timings.m_equilibriumTime = timings.m_maxTotalLIfe;
			timings.m_maxTotalLIfe = parentParticleLife;
		}
		SetMax(pParams->m_stableTime, timings.m_stableTime);
		SetMax(pParams->m_equilibriumTime, timings.m_equilibriumTime);
		SetMax(pParams->m_maxTotalLIfe, timings.m_maxTotalLIfe);

		if (m_duration.GetBaseValue() == 0.0f)
		{
			// Particles all have same age, add Spawn fraction or Id to distinguish them
			pComponent->AddParticleData(EPDT_SpawnFraction);
			if (m_restart.IsEnabled())
				pComponent->AddParticleData(EPDT_SpawnId);
		}
	}

	void AddSubInstances(CParticleComponentRuntime& runtime) override
	{
		if (!runtime.IsChild() && runtime.GetEmitter()->IsActive() && runtime.GetNumInstances() == 0)
		{
			// Add first instance
			SInstance instance;
			TVarArray<SInstance> instances(&instance, 1);
			runtime.AddInstances(instances);
		}
	}

	virtual void InitSubInstances(CParticleComponentRuntime& runtime, SUpdateRange instanceRange) override
	{
		StartInstances(runtime, instanceRange, {});
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_amount, "Amount", "Amount");

		SerializeEnabled(ar, m_delay, "Delay", 9, 0.0f);
		SerializeEnabled(ar, m_duration, "Duration", 11, DefaultDuration());
		if (m_duration.IsEnabled())
			SerializeEnabled(ar, m_restart, "Restart", 11, gInfinity);
	}

	virtual void OnEdit(CParticleComponentRuntime& runtime) override
	{
		// Re-init instances
		runtime.RemoveAllSubInstances();
	}

	virtual float DefaultDuration() const { return gInfinity; }

protected:

	static float CountScale(const CParticleComponentRuntime& runtime)
	{
		float scale = runtime.ComponentParams().m_scaleParticleCount;
		if (!runtime.IsChild())
			scale *= runtime.GetEmitter()->GetSpawnParams().fCountScale;
		return scale;
	}

	template<typename TParam>
	void SerializeEnabled(Serialization::IArchive& ar, TParam& param, cstr name, uint newVersion, float disabledValue)
	{
		if (ar.isInput() && GetVersion(ar) < newVersion)
		{
			struct SEnabledValue
			{
				SEnabledValue(TParam& value)
					: m_value(value) {}
				TParam& m_value;
			
				void Serialize(Serialization::IArchive& ar)
				{
					typedef CParamMod<EDD_PerInstance, UFloat> TTimeParam;

					bool state = false;
					ar(state, "State", "^");
					if (state)
						ar(reinterpret_cast<TTimeParam&>(m_value), "Value", "^");
				}
			};
			param = disabledValue;
			ar(SEnabledValue(param), name, name);
			return;
		}
		if (ar.isInput() && GetVersion(ar) < 12)
			param = TParam::TValue::Default();
		ar(param, name, name);
	}

	// Convert amounts to spawn counts for frame
	virtual void GetSpawnCounts(CParticleComponentRuntime& runtime, TVarArray<float> amounts) const = 0;
	
	virtual void SpawnParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		uint numInstances = runtime.GetNumInstances();

		const bool isIndependent = runtime.GetEmitter()->IsIndependent() && !runtime.IsChild();
		if (isIndependent)
		{
			// Skip spawning immortal independent effects
			float maxLifetime = m_delay.GetValueRange().end + m_duration.GetValueRange().end + runtime.ComponentParams().m_maxParticleLife;
			if (!std::isfinite(maxLifetime))
				numInstances = 0;
		}
		else if (m_restart.IsEnabled())
		{
			// Skip restarts on independent effects
			THeapArray<uint> indicesArray(runtime.MemHeap());
			indicesArray.reserve(numInstances);

			for (uint i = 0; i < numInstances; ++i)
			{
				SSpawnData& spawnData = runtime.GetInstanceData(i, m_offsetSpawnData);
				if (std::isfinite(spawnData.m_restart))
					runtime.SetAlive();
				spawnData.m_restart -= runtime.DeltaTime();
				if (spawnData.m_restart <= 0.0f)
					indicesArray.push_back(i);
			}

			StartInstances(runtime, SUpdateRange(), indicesArray);
		}

		float countScale = CountScale(runtime);
		const float dT = runtime.DeltaTime();
		SUpdateRange range(0, numInstances);

		TFloatArray amounts(runtime.MemHeap(), numInstances);
		IOFStream amountStream(amounts.data());
		for (uint i = 0; i < numInstances; ++i)
		{
			SSpawnData& spawnData = runtime.GetInstanceData(i, m_offsetSpawnData);
			amounts[i] = spawnData.m_amount;
		}

		// Pad end of array, for vectorized modifiers
		std::fill(amounts.end(), amounts.begin() + amounts.capacity(), 0.0f);
		m_amount.ModifyUpdate(runtime, amountStream, range);

		GetSpawnCounts(runtime, amounts);

		THeapArray<SSpawnEntry> spawnEntries(runtime.MemHeap());
		spawnEntries.reserve(numInstances);

		for (uint i = 0; i < numInstances; ++i)
		{
			SSpawnData& spawnData = runtime.GetInstanceData(i, m_offsetSpawnData);

			if (spawnData.m_timer <= spawnData.m_duration)
				runtime.SetAlive();

			const Range spawnTimes = spawnData.SpawnTimes(runtime);
			const float spawnTime = spawnTimes.Length();
			const float spawned = amounts[i] * countScale;

			if (spawnTime >= 0.0f && spawned > 0.0f)
			{
				SSpawnEntry entry = {};
				entry.m_count = uint32(ceil(spawnData.m_spawned + spawned) - ceil(spawnData.m_spawned));
				if (entry.m_count)
				{
					// Set parameters for absolute particle age and spawn fraction
					entry.m_parentId = runtime.GetParentId(i);
					entry.m_ageIncrement = -spawnTime * rcp(spawned);
					entry.m_ageBegin = dT - spawnTimes.start;
					entry.m_ageBegin += (ceil(spawnData.m_spawned) - spawnData.m_spawned) * entry.m_ageIncrement;

					if (std::isfinite(spawnData.m_duration))
					{
						float total = spawned;
						if (spawnData.m_duration * spawnTime > 0.0f)
							total *= spawnData.m_duration * rcp(spawnTime);
						entry.m_fractionIncrement = rcp(total - 1.0f);
						entry.m_fractionBegin = spawnData.m_fraction;
						spawnData.m_fraction += entry.m_fractionIncrement * entry.m_count;
					}

					spawnEntries.push_back(entry);
				}
				spawnData.m_spawned += spawned;
			}

			spawnData.m_timer += dT;
		}

		runtime.AddParticles(spawnEntries);
	}

	void StartInstances(CParticleComponentRuntime& runtime, SUpdateRange instanceRange, TConstArray<uint> instanceIndices)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const uint numStarts = instanceRange.size() + instanceIndices.size();
		if (numStarts == 0)
			return;

		SUpdateRange startRange(0, numStarts);

		STempInitBuffer<float> amounts(runtime, m_amount, startRange);
		STempInitBuffer<float> delays(runtime, m_delay, startRange);
		STempInitBuffer<float> durations(runtime, m_duration, startRange);
		STempInitBuffer<float> restarts(runtime, m_restart, startRange);

		for (uint i = 0; i < numStarts; ++i)
		{
			const uint idx = i < instanceRange.size() ?
				instanceRange.m_begin + i
				: instanceIndices[i - instanceRange.size()];
			SSpawnData& spawnData = runtime.GetInstanceData(idx, m_offsetSpawnData);
			const float delay = delays[i] + runtime.GetInstance(idx).m_startDelay;

			spawnData.m_timer    = -delay;
			spawnData.m_amount   = amounts[i];
			spawnData.m_duration = durations[i];
			spawnData.m_restart  = max(restarts[i], delay + spawnData.m_duration);
			spawnData.m_spawned  = 0.0f;
			spawnData.m_fraction = 0.0f;
		}
	}

protected:

	CParamMod<EDD_InstanceUpdate, UFloat>  m_amount   = 1;
	CParamMod<EDD_PerInstance, UFloat>     m_delay    = 0;
	CParamMod<EDD_PerInstance, UInfFloat>  m_duration = gInfinity;
	CParamMod<EDD_PerInstance, PInfFloat>  m_restart  = gInfinity;

	TDataOffset<SSpawnData>                m_offsetSpawnData;
};

SERIALIZATION_DECLARE_ENUM(ESpawnCountMode,
	MaximumParticles, 
	TotalParticles
)

class CFeatureSpawnCount : public CParticleFeatureSpawnBase
{
public:
	CRY_PFX2_DECLARE_FEATURE
		
	CFeatureSpawnCount()
	{
		m_duration = 0.0f;
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		if (m_duration.IsEnabled() && m_duration.GetBaseValue() > 0.0f)
			ar(m_mode, "Mode", "Mode");
	}

	virtual float DefaultDuration() const override { return 0.0f; }

	virtual void GetDynamicData(const CParticleComponentRuntime& runtime, EParticleDataType type, void* data, EDataDomain domain, SUpdateRange range) override
	{
		if (auto counts = ESDT_ParticleCounts.Cast(type, data, range))
		{
			SInstanceUpdateBuffer<float> amounts(runtime, m_amount, domain);
			SInstanceUpdateBuffer<float> durations(runtime, m_duration, domain);
			const float scale = CountScale(runtime);
			for (auto i : range)
			{
				const float amount = amounts[i].end * scale;
				const float spawnTime = m_mode == ESpawnCountMode::TotalParticles ? durations[i].start
					: min(runtime.ComponentParams().m_maxParticleLife, durations[i].start);
				if (spawnTime > 0.0f)
					counts[i].rate += amount / spawnTime;
				else
					counts[i].burst += int_ceil(amount);
			}
		}
	}

	void GetSpawnCounts(CParticleComponentRuntime& runtime, TVarArray<float> amounts) const override
	{
		for (uint i = 0; i < amounts.size(); ++i)
		{
			const SSpawnData& spawnData = runtime.GetInstanceData(i, m_offsetSpawnData);
			const float dt = spawnData.SpawnTimes(runtime).Length();
			if (dt < 0.0f)
				continue;
			if (m_mode == ESpawnCountMode::TotalParticles || spawnData.m_duration <= runtime.ComponentParams().m_maxParticleLife)
			{
				if (spawnData.m_duration > dt)
					amounts[i] *= dt * rcp(spawnData.m_duration);
				else
					amounts[i] -= spawnData.m_spawned;
			}
			else
			{
				amounts[i] *= dt * rcp(runtime.ComponentParams().m_maxParticleLife);
			}
		}
	}

private:
	ESpawnCountMode m_mode = ESpawnCountMode::MaximumParticles;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnCount, "Spawn", "Count", colorSpawn);

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESpawnRateMode,
	ParticlesPerSecond,
	_SecondPerParticle, SecondsPerParticle = _SecondPerParticle,
	ParticlesPerFrame
)

class CFeatureSpawnRate : public CParticleFeatureSpawnBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	static uint DefaultForType() { return EFT_Spawn; }

	virtual void GetDynamicData(const CParticleComponentRuntime& runtime, EParticleDataType type, void* data, EDataDomain domain, SUpdateRange range) override
	{
		if (auto counts = ESDT_ParticleCounts.Cast(type, data, range))
		{
			SInstanceUpdateBuffer<float> amounts(runtime, m_amount, domain);
			const float scale = CountScale(runtime);
			for (auto i : range)
			{
				const auto amount = amounts[i] * scale;
				if (m_mode == ESpawnRateMode::ParticlesPerFrame)
					counts[i].perFrame += int_ceil(amount.end);
				else
					counts[i].rate += (m_mode == ESpawnRateMode::ParticlesPerSecond ? amount.end : rcp(amount.start));
			}
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
	}

	void GetSpawnCounts(CParticleComponentRuntime& runtime, TVarArray<float> amounts) const override
	{
		for (uint i = 0; i < amounts.size(); ++i)
		{
			SSpawnData& spawnData = runtime.GetInstanceData(i, m_offsetSpawnData);
			const float dt = spawnData.SpawnTimes(runtime).Length();
			if (dt < 0.0f)
				continue;
			amounts[i] =
				m_mode == ESpawnRateMode::ParticlesPerFrame ? amounts[i]
				: dt * (m_mode == ESpawnRateMode::ParticlesPerSecond ? amounts[i] : rcp(amounts[i]));
		}
	}

private:
	ESpawnRateMode m_mode = ESpawnRateMode::ParticlesPerSecond;
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

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CParticleFeatureSpawnBase::AddToComponent(pComponent, pParams);
		m_offsetEmitPos = pComponent->AddInstanceData<Vec3>();
	}
	virtual void InitSubInstances(CParticleComponentRuntime& runtime, SUpdateRange instanceRange) override
	{
		CParticleFeatureSpawnBase::InitSubInstances(runtime, instanceRange);

		THeapArray<QuatTS> locations(runtime.MemHeap(), instanceRange.size());
		runtime.GetEmitLocations(locations, instanceRange.m_begin);

		for (auto instanceId : instanceRange)
		{
			Vec3& emitPos = runtime.GetInstanceData(instanceId, m_offsetEmitPos);
			emitPos = locations[instanceId - instanceRange.m_begin].t;
		}
	}

	void GetSpawnCounts(CParticleComponentRuntime& runtime, TVarArray<float> amounts) const override
	{
		THeapArray<QuatTS> locations(runtime.MemHeap(), amounts.size());
		runtime.GetEmitLocations(locations, 0);

		for (uint i = 0; i < amounts.size(); ++i)
		{
			Vec3& emitPos = runtime.GetInstanceData(i, m_offsetEmitPos);
			const Vec3 emitPos1 = locations[i].t;
			const Vec3 emitPos0 = emitPos;
			const float distance = (emitPos1 - emitPos0).GetLengthFast();
			amounts[i] = distance * (m_mode == ESpawnDistanceMode::ParticlesPerMeter ? amounts[i] : rcp(amounts[i]));
			emitPos = emitPos1;
		}
	}

private:
	ESpawnDistanceMode m_mode = ESpawnDistanceMode::ParticlesPerMeter;
	TDataOffset<Vec3>  m_offsetEmitPos;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDistance, "Spawn", "Distance", colorSpawn);

extern TDataType<Vec4> ESDT_SpatialExtents;

class CFeatureSpawnDensity : public CFeatureSpawnCount
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
	}

	static float ApplyExtents(float amount, Vec4 const& extents)
	{
		return extents[0] + amount * extents[1] + sqr(amount) * extents[2] + cube(amount) * extents[3];
	}

	virtual void GetDynamicData(const CParticleComponentRuntime& runtime, EParticleDataType type, void* data, EDataDomain domain, SUpdateRange range) override
	{
		if (auto counts = ESDT_ParticleCounts.Cast(type, data, range))
		{
			SDynamicData<SMaxParticleCounts> baseCounts(runtime, type, domain, range);
			SDynamicData<Vec4> extents(runtime, ESDT_SpatialExtents, domain, range);

			for (auto i : range)
			{
				counts[i].rate += ApplyExtents(baseCounts[i].rate, extents[i]);
				counts[i].burst += int_ceil(ApplyExtents((float)baseCounts[i].burst, extents[i]));
			}
		}
	}

	void GetSpawnCounts(CParticleComponentRuntime& runtime, TVarArray<float> amounts) const override
	{
		SUpdateRange range(0, amounts.size());
		SDynamicData<Vec4> extents(runtime, ESDT_SpatialExtents, EDD_PerInstance, range);
		for (auto i : range)
		{
			amounts[i] = ApplyExtents(amounts[i], extents[i]);
		}

		CFeatureSpawnCount::GetSpawnCounts(runtime, amounts);
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDensity, "Spawn", "Density", colorSpawn);

}
