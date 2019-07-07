// Copyright 2015-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCommon.h"

namespace pfx2
{

extern TDataType<STimingParams> EEDT_Timings;

class CFeatureLifeTime : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLifeTime(float lifetime = 1, bool killOnParentDeath = false)
		: m_lifeTime(lifetime), m_killOnParentDeath(killOnParentDeath) {}

	virtual EFeatureType GetFeatureType() override { return EFT_Life; }
	static uint DefaultForType() { return EFT_Life; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		SERIALIZE_VAR(ar, m_lifeTime);
		if (m_lifeTime.IsEnabled())
			SERIALIZE_VAR(ar, m_killOnParentDeath);
		else if (ar.isInput())
			m_killOnParentDeath = true;
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		m_lifeTime.AddToComponent(pComponent, this, EPDT_LifeTime);
		pComponent->GetDynamicData.add(this);
		pComponent->PreInitParticles.add(this);
		if (m_killOnParentDeath)
			pComponent->KillParticles.add(this);

		pComponent->UpdateGPUParams.add(this);
	}

	virtual void GetDynamicData(const CParticleComponentRuntime& runtime, EParticleDataType type, void* data, EDataDomain domain, SUpdateRange range) override
	{
		if (auto lifeTimes = EPDT_LifeTime.Cast(type, data, range))
		{
			float maxLife = m_lifeTime.GetValueRange(runtime).end;
			if (m_killOnParentDeath)
			{
				if (runtime.Parent())
				{
					const float parentLife = runtime.Parent()->GetDynamicData(EPDT_LifeTime);
					SetMin(maxLife, parentLife);
				}
			}
			SetMax(lifeTimes[0], maxLife);
		}
	}

	virtual void PreInitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		// Init Lifetime and InvLifetime
		CParticleContainer& container = runtime.GetContainer();
		IOFStream lifeTimes = container.GetIOFStream(EPDT_LifeTime);
		IOFStream invLifeTimes = container.GetIOFStream(EPDT_InvLifeTime);
		IOFStream normAges = container.GetIOFStream(EPDT_NormalAge);

		if (m_lifeTime.GetBaseValue() == 0.0f)
		{
			lifeTimes.Fill(runtime.SpawnedRange(), 0.0f);
			invLifeTimes.Fill(runtime.SpawnedRange(), 0.0f);
			normAges.Fill(runtime.SpawnedRange(), 0.0f);
		}
		else
		{
			if (!m_lifeTime.IsEnabled())
			{
				// Infinite lifetime. Store huge lifetime instead, so absolute age can be computed when needed
				static const float maxLifeTime = float(1 << 30);
				lifeTimes.Fill(runtime.SpawnedRange(), maxLifeTime);
				invLifeTimes.Fill(runtime.SpawnedRange(), 1.0f / maxLifeTime);
			}
			else
			{
				m_lifeTime.Init(runtime, EPDT_LifeTime);
				if (m_lifeTime.HasModifiers())
				{
					for (auto particleGroupId : runtime.SpawnedRangeV())
					{
						const floatv lifetime = lifeTimes.Load(particleGroupId);
						const floatv invLifeTime = rcp(max(lifetime, ToFloatv(FLT_EPSILON)));
						invLifeTimes.Store(particleGroupId, invLifeTime);
					}
				}
				else
				{
					invLifeTimes.Fill(runtime.SpawnedRange(), 1.0f / m_lifeTime.GetBaseValue());
				}
			}

			// Convert ages from absolute to normalized
			for (auto particleGroupId : runtime.SpawnedRangeV())
			{
				normAges[particleGroupId] *= invLifeTimes[particleGroupId];
			}
		}
	}

	virtual void KillParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		// Kill on parent death
		const CParticleContainer& parentContainer = runtime.GetParentContainer();
		const auto parentAges = parentContainer.GetIFStream(EPDT_NormalAge);

		CParticleContainer& container = runtime.GetContainer();
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		IOFStream ages = container.GetIOFStream(EPDT_NormalAge);

		for (auto particleId : runtime.FullRange())
		{
			const TParticleId parentId = parentIds.Load(particleId);
			if (!parentContainer.IdIsAlive(parentId) || IsExpired(parentAges.Load(parentId)))
				ages.Store(particleId, 1.0f);
		}
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.lifeTime = m_lifeTime.GetValueRange(runtime)(0.5f);
	}

protected:
	CParamMod<EDD_Particle, UInfFloat> m_lifeTime          = 1;
	bool                               m_killOnParentDeath = false;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLifeTime, "Life", "Time", colorLife);

//////////////////////////////////////////////////////////////////////////

// Legacy LifeImmortal and KillOnParentDeath features, for versions < 11

class CFeatureLifeImmortal : public CFeatureLifeTime
{
public:
	virtual CParticleFeature* ResolveDependency(CParticleComponent* pComponent) override
	{
		return (new CFeatureLifeTime(gInfinity, true))->ResolveDependency(pComponent);
	}
};

CRY_PFX2_LEGACY_FEATURE(CFeatureLifeImmortal, "Life", "Immortal")

class CFeatureKillOnParentDeath : public CFeatureLifeTime
{
public:
	virtual CParticleFeature* ResolveDependency(CParticleComponent* pComponent) override
	{
		// If another LifeTime feature exists, use it, and set the Kill param.
		// Otherwise, use this feature, with default LifeTime param.
		if (auto pFeature = pComponent->FindDuplicateFeature(this))
		{
			pFeature->m_killOnParentDeath = true;
		}
		return nullptr;
	}
};

CRY_PFX2_LEGACY_FEATURE(CFeatureKillOnParentDeath, "Kill", "OnParentDeath");

}

