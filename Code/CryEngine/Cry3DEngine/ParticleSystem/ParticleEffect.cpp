// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     23/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CrySerialization/STL.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/SmartPtr.h>
#include "ParticleEffect.h"
#include "ParticleEmitter.h"
#include "ParticleFeature.h"

CRY_PFX2_DBG

namespace pfx2
{

//////////////////////////////////////////////////////////////////////////
// CParticleEffect

CParticleEffect::CParticleEffect()
	: m_editVersion(0)
	, m_dirty(true)
	, m_numRenderObjects(0)
{
}

cstr CParticleEffect::GetName() const
{
	return m_name.c_str();
}

void CParticleEffect::Compile()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	if (!m_dirty)
		return;

	m_numRenderObjects = 0;
	m_attributeInstance.Reset(&m_attributes, EAttributeScope::PerEffect);
	for (size_t i = 0; i < m_components.size(); ++i)
	{
		m_components[i]->m_pEffect = this;
		m_components[i]->m_componentId = i;
		m_components[i]->SetChanged();
		m_components[i]->m_componentParams.Reset();
		m_components[i]->PreCompile();
	}
	for (auto& component : m_components)
		component->ResolveDependencies();
	for (auto& component : m_components)
		component->Compile();
	for (auto& component : m_components)
		component->FinalizeCompile();

	m_dirty = false;
}

TComponentId CParticleEffect::FindComponentIdByName(const char* name) const
{
	const auto it = std::find_if(m_components.begin(), m_components.end(), [name](TComponentPtr pComponent)
		{
			return strcmp(pComponent->GetName(), name) == 0;
	  });
	if (it == m_components.end())
		return gInvalidId;
	return TComponentId(it - m_components.begin());
}

string CParticleEffect::MakeUniqueName(TComponentId forComponentId, const char* name)
{
	TComponentId foundId = FindComponentIdByName(name);
	if (foundId == forComponentId || foundId == gInvalidId)
		return string(name);

	CryStackStringT<char, 256> newName(name);
	const uint sz = strlen(name);
	if (isdigit(name[sz - 2]) && isdigit(name[sz - 1]))
	{
		const uint newIdent = (name[sz - 2] - '0') * 10 + (name[sz - 1] - '0') + 1;
		newName.replace(sz - 2, 1, 1, (newIdent / 10) % 10 + '0');
		newName.replace(sz - 1, 1, 1, newIdent % 10 + '0');
	}
	else
	{
		newName.append("01");
	}
	return MakeUniqueName(forComponentId, newName);
}

uint CParticleEffect::AddRenderObjectId()
{
	return m_numRenderObjects++;
}

uint CParticleEffect::GetNumRenderObjectIds() const
{
	return m_numRenderObjects;
}

void CParticleEffect::SetName(cstr name)
{
	m_name = name;
}

void CParticleEffect::Serialize(Serialization::IArchive& ar)
{
	uint documentVersion = 1;
	if (ar.isOutput())
		documentVersion = gCurrentVersion;
	ar(documentVersion, "Version", "");
	SSerializationContext documentContext(documentVersion);
	Serialization::SContext context(ar, &documentContext);

	ar(m_attributes, "Attributes");

	if (ar.isInput() && documentVersion < 3)
	{
		std::vector<CParticleComponent> oldComponents;
		ar(oldComponents, "Components", "+Components");
		m_components.reserve(oldComponents.size());
		for (auto& oldComponent : oldComponents)
			m_components.push_back(new CParticleComponent(oldComponent));
	}
	else
		ar(m_components, "Components", "+Components");

	if (ar.isInput())
	{
		auto it = std::remove_if(m_components.begin(), m_components.end(), [](TComponentPtr ptr){ return !ptr; });
		m_components.erase(it, m_components.end());
		SetChanged();
		for (auto& component : m_components)
			component->SetChanged();
		Compile();
	}
}

IParticleEmitter* CParticleEffect::Spawn(const ParticleLoc& loc, const SpawnParams* pSpawnParams)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	PParticleEmitter pEmitter = GetPSystem()->CreateEmitter(this);
	CParticleEmitter* pCEmitter = static_cast<CParticleEmitter*>(pEmitter.get());
	pCEmitter->SetLocation(loc);
	return pEmitter;
}

void CParticleEffect::AddComponent(uint componentIdx)
{
	uint idx = m_components.size();
	CParticleComponent* pNewComponent = new CParticleComponent();
	pNewComponent->m_pEffect = this;
	pNewComponent->m_componentId = componentIdx;
	pNewComponent->SetName("Component01");
	m_components.insert(m_components.begin() + componentIdx, pNewComponent);
	SetChanged();
}

void CParticleEffect::RemoveComponent(uint componentIdx)
{
	m_components.erase(m_components.begin() + componentIdx);
	SetChanged();
}

void CParticleEffect::SetChanged()
{
	if (!m_dirty)
		++m_editVersion;
	m_dirty = true;
}

Serialization::SStruct CParticleEffect::GetEffectOptionsSerializer() const
{
	return Serialization::SStruct(m_attributes);
}

}
