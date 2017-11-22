// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     29/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem/ParticleSystem.h"
#include "ParticleSystem/ParticleComponentRuntime.h"
#include "ParamMod.h"

namespace pfx2
{

struct CRY_ALIGN(CRY_PFX2_PARTICLES_ALIGNMENT) SSpawnData
{
	float m_amount;
	float m_spawned;
	float m_duration;
	float m_restart;
	float m_timer;
};

// PFX2_TODO : not enough self documented code. Refactor!

class CParticleFeatureSpawnBase : public CParticleFeature
{
public:
	CParticleFeatureSpawnBase()
	{
		m_duration = DefaultDuration();
	}

	virtual EFeatureType GetFeatureType() override
	{
		return EFT_Spawn;
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitSubInstances.add(this);
		pComponent->SpawnParticles.add(this);
		m_spawnDataOff = pComponent->AddInstanceData(sizeof(SSpawnData));
		m_amount.AddToComponent(pComponent, this);
		m_delay.AddToComponent(pComponent, this);
		m_duration.AddToComponent(pComponent, this);
		m_restart.AddToComponent(pComponent, this);

		pParams->m_emitterLifeTime.start += m_delay.GetValueRange().start;
		if (m_restart.IsEnabled())
			pParams->m_emitterLifeTime.end = gInfinity;
		else
			pParams->m_emitterLifeTime.end += m_delay.GetValueRange().end + m_duration.GetValueRange().end;
	}

	virtual void InitSubInstances(const SUpdateContext& context, SUpdateRange instanceRange) override
	{
		StartInstances(context, instanceRange, {});
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

	void UpdateAmounts(const SUpdateContext& context, TVarArray<float> amounts) const {}

	virtual float DefaultDuration() const { return gInfinity; }

protected:

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
					typedef CParamMod<SModInstanceTimer, UFloat> TTimeParam;

					bool state = false;
					ar(state, "State", "^");
					if (state)
						ar(reinterpret_cast<TTimeParam&>(m_value), "Value", "^");
				}
			};
			param = disabledValue;
			ar(SEnabledValue(param), name, name);
		}
		else
			ar(param, name, name);
	}

	template<typename Impl>
	void SpawnParticlesT(Impl& impl, const SUpdateContext& context)
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleComponentRuntime& runtime = context.m_runtime;
		const uint numInstances = runtime.GetNumInstances();
		if (numInstances == 0)
			return;

		const CParticleEmitter* pEmitter = context.m_runtime.GetEmitter();
		if (pEmitter->IsIndependent())
		{
			if (!runtime.IsChild() && context.m_params.IsImmortal())
				return;
		}
		else if (m_restart.IsEnabled())
		{
			THeapArray<uint> indicesArray(*context.m_pMemHeap);
			indicesArray.reserve(numInstances);

			for (uint i = 0; i < numInstances; ++i)
			{
				SSpawnData* pSpawn = GetSpawnData(runtime, i);
				pSpawn->m_restart -= context.m_deltaTime;
				if (pSpawn->m_restart <= 0.0f)
					indicesArray.push_back(i);
			}

			StartInstances(context, SUpdateRange(), indicesArray);
		}

		const float countScale = runtime.IsChild() ? 1.0f : runtime.GetEmitter()->GetSpawnParams().fCountScale;
		const float dT = context.m_deltaTime;
		const float invDT = dT ? 1.0f / dT : 0.0f;
		SUpdateRange range(0, numInstances);

		TFloatArray amounts(*context.m_pMemHeap, numInstances);
		for (uint i = 0; i < numInstances; ++i)
		{
			SSpawnData* pSpawn = GetSpawnData(runtime, i);
			amounts[i] = pSpawn->m_amount * context.m_params.m_scaleParticleCount;
		}
		m_amount.ModifyUpdate(context, amounts.data(), range);

		impl.UpdateAmounts(context, amounts);

		for (uint i = 0; i < numInstances; ++i)
		{
			SSpawnData* pSpawn = GetSpawnData(runtime, i);

			const float startTime = max(pSpawn->m_timer, 0.0f);
			const float endTime = min(pSpawn->m_timer + dT, pSpawn->m_duration);
			const float spawnTime = endTime - startTime;
			const float amount = amounts[i] * countScale;

			if (spawnTime >= 0.0f && amount > 0.0f)
			{
				const float spawned = impl.GetSpawnCount(context, *pSpawn, i, amount, spawnTime);
				SSpawnEntry entry = {};
				entry.m_count = uint32(ceil(pSpawn->m_spawned + spawned) - ceil(pSpawn->m_spawned));
				if (entry.m_count)
				{
					entry.m_parentId = runtime.GetParentId(i);
					entry.m_ageIncrement = rcp(spawned) * spawnTime * invDT;
					entry.m_ageBegin = (startTime - pSpawn->m_timer - dT) * invDT;
					entry.m_ageBegin += (ceil(pSpawn->m_spawned) - pSpawn->m_spawned) * entry.m_ageIncrement;

					if (std::isfinite(pSpawn->m_duration))
					{
						entry.m_fractionIncrement = rcp((float)entry.m_count);
						if (pSpawn->m_duration > 0.0f)
						{
							const float invDuration = rcp(pSpawn->m_duration);
							entry.m_fractionBegin = startTime * invDuration;
							const float fractionEnd = endTime * invDuration;
							entry.m_fractionIncrement *= (fractionEnd - entry.m_fractionBegin);
						}
					}

					runtime.AddSpawnEntry(entry);
				}
				pSpawn->m_spawned += spawned;
			}

			pSpawn->m_timer += dT;
		}
	}

	void StartInstances(const SUpdateContext& context, SUpdateRange instanceRange, TConstArray<uint> instanceIndices)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const uint numStarts = instanceRange.size() + instanceIndices.size();
		if (numStarts == 0)
			return;

		CParticleComponentRuntime& runtime = context.m_runtime;
		SUpdateRange startRange(0, numStarts);
		TFloatArray amounts(*context.m_pMemHeap, numStarts);
		TFloatArray delays(*context.m_pMemHeap, numStarts);
		TFloatArray durations(*context.m_pMemHeap, numStarts);
		TFloatArray restarts(*context.m_pMemHeap, numStarts);
		m_amount.ModifyInit(context, amounts.data(), startRange);
		m_delay.ModifyInit(context, delays.data(), startRange);
		if (m_duration.IsEnabled())
			m_duration.ModifyInit(context, durations.data(), startRange);
		if (m_restart.IsEnabled())
			m_restart.ModifyInit(context, restarts.data(), startRange);

		for (uint i = 0; i < numStarts; ++i)
		{
			const uint idx = i < instanceRange.size() ?
				instanceRange.m_begin + i
				: instanceIndices[i - instanceRange.size()];
			SSpawnData* pSpawn = GetSpawnData(runtime, idx);

			const float delay = delays[i] + runtime.GetInstance(idx).m_startDelay;

			pSpawn->m_timer = -delay;
			pSpawn->m_spawned = 0.0f;
			pSpawn->m_amount = amounts[i];
			pSpawn->m_duration = m_duration.IsEnabled() ? durations[i] : gInfinity;
			pSpawn->m_restart = m_restart.IsEnabled() ? max(restarts[i], delay + pSpawn->m_duration) : gInfinity;
		}
	}

	SSpawnData* GetSpawnData(CParticleComponentRuntime& runtime, uint idx) { return runtime.GetSubInstanceData<SSpawnData>(idx, m_spawnDataOff); }

protected:

	CParamMod<SModInstanceCounter, UFloat>  m_amount   = 1;
	CParamMod<SModInstanceTimer, UFloat>    m_delay    = 0;
	CParamMod<SModInstanceTimer, UInfFloat> m_duration = gInfinity;
	CParamMod<SModInstanceTimer, PInfFloat> m_restart  = gInfinity;

	TInstanceDataOffset                     m_spawnDataOff;
};

class CFeatureSpawnCount : public CParticleFeatureSpawnBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual float DefaultDuration() const override { return 0.0f; }

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CParticleFeatureSpawnBase::AddToComponent(pComponent, pParams);

		const float maxParticles = m_amount.GetValueRange().end;
		const float spawnTime = min(pParams->m_maxParticleLifeTime, m_duration.GetValueRange().start);
		if (spawnTime > 0.0f)
			pParams->m_maxParticleSpawnRate += maxParticles / spawnTime;
		else
			pParams->m_maxParticlesBurst += int_ceil(maxParticles);
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	ILINE float GetSpawnCount(const SUpdateContext& context, const SSpawnData& spawn, uint instanceId, float amount, float dt)
	{
		const float spawnTime = min(context.m_params.m_maxParticleLifeTime, spawn.m_duration);
		if (spawnTime > 0.0f)
		{
			const float rate = amount * rcp_fast(spawnTime);
			return rate * dt;
		}
		else
		{
			return amount - spawn.m_spawned;
		}
	}
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

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CParticleFeatureSpawnBase::AddToComponent(pComponent, pParams);

		const auto amount = m_amount.GetValueRange();
		if (m_mode == ESpawnRateMode::ParticlesPerFrame)
			pParams->m_maxParticlesBurst += int_ceil(amount.end);
		else
			pParams->m_maxParticleSpawnRate += (m_mode == ESpawnRateMode::ParticlesPerSecond ? amount.end : rcp(amount.start));
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

	ILINE float GetSpawnCount(const SUpdateContext& context, const SSpawnData& spawn, uint instanceId, float amount, float dt)
	{
		const float spawned = 
			m_mode == ESpawnRateMode::ParticlesPerFrame ? amount
			: dt * (m_mode == ESpawnRateMode::ParticlesPerSecond ? amount : rcp(amount));
		return spawned;
	}

private:
	ESpawnRateMode m_mode = ESpawnRateMode::ParticlesPerSecond;
};

CRY_PFX2_IMPLEMENT_FEATURE_DEFAULT(CParticleFeature, CFeatureSpawnRate, "Spawn", "Rate", colorSpawn, EFT_Spawn);

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
		m_emitPosOffset = pComponent->AddInstanceData(sizeof(Vec3));
	}
	virtual void InitSubInstances(const SUpdateContext& context, SUpdateRange instanceRange) override
	{
		CParticleFeatureSpawnBase::InitSubInstances(context, instanceRange);

		for (auto inst : instanceRange)
		{
			const TParticleId parentId = context.m_runtime.GetParentId(inst);
			Vec3* pEmitPos = EmitPositionData(context.m_runtime, inst);
			*pEmitPos = EmitPosition(context, parentId);
		}
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	ILINE float GetSpawnCount(const SUpdateContext& context, const SSpawnData& spawn, uint instanceId, float amount, float dt)
	{
		Vec3* pEmitPos = EmitPositionData(context.m_runtime, instanceId);
		const Vec3 emitPos0 = *pEmitPos;
		const Vec3 emitPos1 = EmitPosition(context, context.m_runtime.GetParentId(instanceId));
		*pEmitPos = emitPos1;

		const float distance = (emitPos1 - emitPos0).GetLengthFast();
		const float spawned = distance * (m_mode == ESpawnDistanceMode::ParticlesPerMeter ? amount : rcp(amount));
		return spawned;
	}

private:
	ESpawnDistanceMode m_mode = ESpawnDistanceMode::ParticlesPerMeter;
	TInstanceDataOffset m_emitPosOffset;

	Vec3* EmitPositionData(CParticleComponentRuntime& runtime, uint idx) const { return runtime.GetSubInstanceData<Vec3>(idx, m_emitPosOffset); }

	Vec3 EmitPosition(const SUpdateContext& context, TParticleId parentId) const
	{
		QuatT parentLoc;
		parentLoc.t = context.m_parentContainer.GetIVec3Stream(EPVF_Position).SafeLoad(parentId);
		parentLoc.q = context.m_parentContainer.GetIQuatStream(EPQF_Orientation).SafeLoad(parentId);

		Vec3 emitOffset(0);
		context.m_runtime.GetComponent()->GetEmitOffset(context, parentId, emitOffset);
		return parentLoc * emitOffset;
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDistance, "Spawn", "Distance", colorSpawn);

class CFeatureSpawnDensity : public CFeatureSpawnCount
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeatureSpawnBase::Serialize(ar);
	}

	virtual void SpawnParticles(const SUpdateContext& context) override
	{
		SpawnParticlesT(*this, context);
	}

	void UpdateAmounts(const SUpdateContext& context, TVarArray<float> amounts) const
	{
		CParticleComponentRuntime& runtime = context.m_runtime;
		const uint numInstances = runtime.GetNumInstances();

		TFloatArray extents(*context.m_pMemHeap, numInstances);
		extents.fill(0.0f);
		runtime.GetComponent()->GetSpatialExtents(context, amounts, extents);
		for (uint i = 0; i < numInstances; ++i)
		{
			amounts[i] = extents[i];
		}
	}
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureSpawnDensity, "Spawn", "Density", colorSpawn);

}
