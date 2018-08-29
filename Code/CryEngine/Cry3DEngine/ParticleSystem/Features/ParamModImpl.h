// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <CrySerialization/SmartPtr.h>
#include "ParamMod.h"

namespace pfx2
{

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature)
{
	if (Domain & EDD_PerParticle)
		pComponent->InitParticles.add(pFeature);

	for (auto& pModifier : m_modifiers)
	{
		if (pModifier && pModifier->IsEnabled())
			pModifier->AddToParam(pComponent);
	}
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature, ThisDataType dataType)
{
	AddToComponent(pComponent, pFeature);
	pComponent->AddParticleData(dataType);

	for (auto& pModifier : m_modifiers)
	{
		if (pModifier && pModifier->IsEnabled())
			pModifier->SetDataType(dataType);
	}

	if (Domain & EDD_HasUpdate && !m_modUpdate.empty())
	{
		pComponent->AddParticleData(dataType.InitType());
		if (Domain & EDD_PerParticle)
			pComponent->UpdateParticles.add(pFeature);
	}
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::Serialize(Serialization::IArchive& ar)
{
	EDataDomain domain = Domain;
	Serialization::SContext _modContext(ar, static_cast<EDataDomain*>(&domain));
	ar(m_baseValue, T::ValueName(), "^");
	ar(SkipEmpty(m_modifiers), T::ModsName(), "^");

	if (ar.isInput())
	{
		m_modInit.clear();
		m_modUpdate.clear();
		stl::find_and_erase_all(m_modifiers, nullptr);
		for (auto& pMod : m_modifiers)
		{
			if (TModifier* pNewMod = pMod->VersionFixReplace())
			{
				pMod.reset(pNewMod);
			}
			if (pMod->IsEnabled())
			{
				if (pMod->GetDomain() & Domain & EDD_HasUpdate)
					m_modUpdate.push_back(pMod);
				else
					m_modInit.push_back(pMod);
			}
		}
	}
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::Init(CParticleComponentRuntime& runtime, ThisDataType dataType) const
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleContainer& container = runtime.GetContainer();
	if (container.GetMaxParticles() == 0 || !container.HasData(dataType))
		return;

	TIOStream<TType> stream = container.IOStream(dataType);
	ModifyInit(runtime, stream, runtime.SpawnedRange());

	if (Domain & EDD_HasUpdate)
		container.CopyData(dataType.InitType(), dataType, runtime.SpawnedRange());
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::Update(CParticleComponentRuntime& runtime, ThisDataType dataType) const
{
	CParticleContainer& container = runtime.GetContainer();
	if (container.GetMaxParticles() == 0 || !container.HasData(dataType))
		return;

	CRY_PFX2_ASSERT(runtime.FullRange().m_end <= container.GetNumParticles());
	TIOStream<TType> stream = container.IOStream(dataType);
	ModifyUpdate(runtime, stream, runtime.FullRange());
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::ModifyInit(const CParticleComponentRuntime& runtime, TIOStream<TType>& stream, SUpdateRange range) const
{
	CRY_PFX2_PROFILE_DETAIL;
	stream.Fill(range, m_baseValue);

	for (auto& pModifier : m_modInit)
		pModifier->Modify(runtime, range, stream, Domain);
}

template<EDataDomain Domain, typename T>
void CParamMod<Domain, T>::ModifyUpdate(const CParticleComponentRuntime& runtime, TIOStream<TType>& stream, SUpdateRange range) const
{
	CRY_PFX2_PROFILE_DETAIL;
	for (auto& pModifier : m_modUpdate)
		pModifier->Modify(runtime, range, stream, Domain);
}

template<EDataDomain Domain, typename T>
auto CParamMod<Domain, T>::GetValues(const CParticleComponentRuntime& runtime, TVarArray<TType> data, EDataDomain domain) const -> TRange<TFrom>
{
	TRange<TType> minmax(1);
	TIOStream<TType> stream(data.data());
	SUpdateRange range(0, data.size());

	data.fill(m_baseValue);

	for (auto& pMod : m_modifiers)
	{
		if (domain >= pMod->GetDomain())
			pMod->Modify(runtime, range, stream, domain);
		else
			minmax = minmax * pMod->GetMinMax();
	}
	return minmax;
}

template<EDataDomain Domain, typename T>
auto CParamMod<Domain, T>::GetValueRange(const CParticleComponentRuntime& runtime) const -> TRange<TFrom>
{
	floatv curValueV = convert<floatv>(0.0f);
	float& curValue = (float&)curValueV;
	auto minmax = GetValues(runtime, TVarArray<float>(&curValue, 1), EDD_None);
	return minmax * curValue;
}

template<EDataDomain Domain, typename T>
auto CParamMod<Domain, T>::GetValueRange() const -> TRange<TFrom>
{
	TRange<TType> minmax(m_baseValue);
	for (auto& pMod : m_modifiers)
		minmax = minmax * pMod->GetMinMax();
	return minmax;
}

template<EDataDomain Domain, typename T>
void pfx2::CParamMod<Domain, T>::Sample(TVarArray<TFrom> samples) const
{
	samples.fill(T::From(m_baseValue));
	for (auto& pModifier : m_modifiers)
	{
		if (pModifier->IsEnabled())
			pModifier->Sample(samples);
	}
}

}
