// Copyright 2015-2018 Crytek GmbH / Crytek Group. All rights reserved.

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

}
