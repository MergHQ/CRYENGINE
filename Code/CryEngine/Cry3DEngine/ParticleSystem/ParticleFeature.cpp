// Copyright 2015-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleFeature.h"
#include "ParticleSystem.h"

namespace pfx2
{

bool CParticleFeature::RegisterFeature(const SParticleFeatureParams& params)
{
	return CParticleSystem::RegisterFeature(params);
}

void CParticleFeature::Serialize(Serialization::IArchive& ar)
{
	if (!ar.isEdit())
		ar(m_enabled);
}

gpu_pfx2::IParticleFeature* CParticleFeature::MakeGpuInterface(CParticleComponent* pComponent, gpu_pfx2::EGpuFeatureType feature)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (!feature || !pComponent->UsesGPU() || !gEnv->pRenderer)
		m_gpuInterface.reset();
	else if (!m_gpuInterface)
		m_gpuInterface = gEnv->pRenderer->GetGpuParticleManager()->CreateParticleFeature(feature);
	pComponent->AddGPUFeature(m_gpuInterface);
	return m_gpuInterface;
}

CParticleComponent* EditingComponent(IArchive& ar)
{
	if (ar.isInput() && ar.isEdit())
		return static_cast<CParticleComponent*>(ar.context<IParticleComponent>());
	return nullptr;
}

// SCheckEditFeature implementation
SCheckEditFeature::SCheckEditFeature(IArchive& ar, CParticleFeature& feature)
	: m_pComponent(EditingComponent(ar))
	, m_feature(feature)
{
	if (!m_pComponent)
		return;
	SaveText(m_archiveText);
}

SCheckEditFeature::~SCheckEditFeature()
{
	if (!m_pComponent)
		return;
	string newText;
	SaveText(newText);
	if (newText != m_archiveText)
		GetPSystem()->OnEditFeature(m_pComponent, &m_feature);
}

void SCheckEditFeature::SaveText(string& dest)
{
	// Save current state of feature
	DynArray<char> buffer;
	Serialization::SaveJsonBuffer(buffer, m_feature);
	dest = string(buffer.data(), buffer.size());
}

}
