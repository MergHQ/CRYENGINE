// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     18/12/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"
#include "ParticleSystem/ParticleEmitter.h"

CRY_PFX2_DBG

namespace pfx2
{

//////////////////////////////////////////////////////////////////////////

SERIALIZATION_DECLARE_ENUM(ESecondGenMode,
                           All,
                           Random
                           )

class CFeatureSecondGenBase : public CParticleFeature
{
public:
	CFeatureSecondGenBase()
		: m_probability(1.0f)
		, m_mode(ESecondGenMode::All) {}

	void ResolveDependency(CParticleComponent* pComponent) override
	{
		CRY_PFX2_ASSERT(pComponent);

		CParticleEffect* pEffect = pComponent->GetEffect();

		m_componentIds.clear();
		for (auto& componentName : m_componentNames)
		{
			TComponentId componentId = ResolveIdForName(pComponent, componentName);
			const bool isvalid = componentId != gInvalidId;
			if (isvalid)
			{
				const bool isUnique = std::find(m_componentIds.begin(), m_componentIds.end(), componentId) == m_componentIds.end();
				if (isUnique)
				{
					CParticleComponent* pSubComp = pEffect->GetCComponent(componentId);
					if (pSubComp->SetSecondGeneration(pComponent))
						m_componentIds.push_back(componentId);
				}
			}
		}
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_mode, "Mode", "Mode");
		ar(m_probability, "Probability", "Probability");
		ar(m_componentNames, "Components", "!Components");
		if (ar.isInput())
			VersionFix(ar);
	}

	uint GetNumConnectors() const override
	{
		return m_componentNames.size();
	}

	const char* GetConnectorName(uint connectorId) const override
	{
		if (connectorId >= m_componentNames.size())
			return nullptr;
		return m_componentNames[connectorId];
	}

	void ConnectTo(const char* pOtherName) override
	{
		auto it = FindComponentName(pOtherName);
		if (it == m_componentNames.end())
			m_componentNames.push_back(pOtherName);
	}

	void DisconnectFrom(const char* pOtherName) override
	{
		auto it = FindComponentName(pOtherName);
		if (it != m_componentNames.end())
			m_componentNames.erase(it);
	}

protected:
	ILINE void TriggerParticles(const SUpdateContext& context, const TParticleId* pParticleIdTriggers, uint triggerCount)
	{
		if (m_mode == ESecondGenMode::All)
			TriggerAll(context, pParticleIdTriggers, triggerCount);
		else
			TriggerRandom(context, pParticleIdTriggers, triggerCount);
	}

private:
	ILINE void TriggerAll(const SUpdateContext& context, const TParticleId* pParticleIdTriggers, uint triggerCount)
	{
		CParticleContainer& container = context.m_container;
		TParticleHeap::Array<CParticleComponentRuntime::SInstance> newInstances(*context.m_pMemHeap);
		newInstances.reserve(triggerCount);
		const uint numEntries = m_componentIds.size();

		for (uint i = 0; i < numEntries; ++i)
		{
			const TComponentId componentId = m_componentIds[i];
			ICommonParticleComponentRuntime* pChildComponentRuntime =
			  context.m_runtime.GetEmitter()->GetRuntimes()[componentId].pRuntime;
			SChaosKey chaosKey = context.m_spawnRng;

			for (uint j = 0; j < triggerCount; ++j)
			{
				const TParticleId particleId = pParticleIdTriggers[j];
				const bool trigger = (chaosKey.RandUNorm() <= m_probability);
				if (trigger)
				{
					CParticleComponentRuntime::SInstance instance;
					instance.m_parentId = container.GetRealId(particleId);
					newInstances.push_back(instance);
				}
			}
			pChildComponentRuntime->AddSubInstances(newInstances.data(), newInstances.size());
			newInstances.clear();
		}
	}

	ILINE void TriggerRandom(const SUpdateContext& context, const TParticleId* pParticleIdTriggers, uint triggerCount)
	{
		CParticleContainer& container = context.m_container;
		TParticleHeap::Array<CParticleComponentRuntime::SInstance> newInstances(*context.m_pMemHeap);
		newInstances.reserve(triggerCount);
		const uint numEntries = m_componentIds.size();

		for (uint i = 0; i < numEntries; ++i)
		{
			const TComponentId componentId = m_componentIds[i];
			ICommonParticleComponentRuntime* pChildComponentRuntime =
			  context.m_runtime.GetEmitter()->GetRuntimes()[componentId].pRuntime;
			SChaosKey chaosKey = context.m_spawnRng;

			for (uint j = 0; j < triggerCount; ++j)
			{
				const TParticleId particleId = pParticleIdTriggers[j];
				const float u = chaosKey.RandUNorm();
				const float v = chaosKey.RandUNorm() * numEntries;
				const bool trigger = (u <= m_probability);
				const bool inRange = v >= float(i) && v < (float(i) + 1.0f);
				if (trigger && inRange)
				{
					CParticleComponentRuntime::SInstance instance;
					instance.m_parentId = container.GetRealId(particleId);
					newInstances.push_back(instance);
				}
			}
			pChildComponentRuntime->AddSubInstances(newInstances.data(), newInstances.size());
			newInstances.clear();
		}
	}

	std::vector<string>::iterator FindComponentName(const char* pOther)
	{
		auto it = std::find_if(m_componentNames.begin(), m_componentNames.end(),
		                       [pOther](const string& componentName)
			{
				return strcmp(componentName.c_str(), pOther) == 0;
		  });
		return it;
	}

	TComponentId ResolveIdForName(CParticleComponent* pComponent, const string& componentName)
	{
		TComponentId compId = pComponent->GetComponentId();
		CParticleEffect* pEffect = pComponent->GetEffect();
		TComponentId subComponentId = gInvalidId;

		if (componentName == pComponent->GetName())
		{}  // TODO - user error - user set this component as sub component for itself.
		for (TComponentId i = 0; i < pEffect->GetNumComponents(); ++i)
		{
			CParticleComponent* pSubComp = pEffect->GetCComponent(i);
			if (pSubComp->GetName() == componentName)
			{
				subComponentId = i;
				break;
			}
		}
		if (subComponentId == gInvalidId)
		{}      // TODO - user error - sub component name was not found
		return subComponentId;
	}

	void VersionFix(Serialization::IArchive& ar)
	{
		switch (GetVersion(ar))
		{
		case 3:
			{
				string subComponentName;
				ar(subComponentName, "subComponent", "Trigger Component");
				ConnectTo(subComponentName);
			}
			break;
		}
	}

	std::vector<string>       m_componentNames;
	std::vector<TComponentId> m_componentIds;
	SUnitFloat                m_probability;
	ESecondGenMode            m_mode;
};

//////////////////////////////////////////////////////////////////////////

class CFeatureSecondGenOnSpawn : public CFeatureSecondGenBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CFeatureSecondGenBase::AddToComponent(pComponent, pParams);
		if (GetNumConnectors() != 0)
			pComponent->AddToUpdateList(EUL_InitUpdate, this);
	}

	virtual void InitParticles(const SUpdateContext& context) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		TParticleHeap::Array<TParticleId> triggers(*context.m_pMemHeap);
		triggers.reserve(context.m_container.GetNumSpawnedParticles());
		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		triggers.push_back(particleId);
		CRY_PFX2_FOR_END;
		TriggerParticles(context, triggers.data(), triggers.size());
	}
};

CRY_PFX2_IMPLEMENT_FEATURE_WITH_CONNECTOR(CParticleFeature, CFeatureSecondGenOnSpawn, "SecondGen", "OnSpawn", "Editor/Icons/Particles/onspawn.png", secondGenColor);

//////////////////////////////////////////////////////////////////////////

class CFeatureSecondGenOnDeath : public CFeatureSecondGenBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		CFeatureSecondGenBase::AddToComponent(pComponent, pParams);
		if (GetNumConnectors() != 0)
			pComponent->AddToUpdateList(EUL_KillUpdate, this);
	}

	void KillParticles(const SUpdateContext& context, TParticleId* pParticles, size_t count) override
	{
		TriggerParticles(context, pParticles, count);
	}
};

CRY_PFX2_IMPLEMENT_FEATURE_WITH_CONNECTOR(CParticleFeature, CFeatureSecondGenOnDeath, "SecondGen", "OnDeath", "Editor/Icons/Particles/ondeath.png", secondGenColor);

}
