// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCollision.h"

CRY_PFX2_DBG

namespace pfx2
{

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureCollision, "Motion", "Collision", defaultIcon, defaultColor);

void CFeatureCollision::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_Update, this);

	if (auto pInt = GetGpuInterface())
	{
		gpu_pfx2::SFeatureParametersCollision params;
		params.offset = m_offset;
		params.radius = m_radius;
		params.restitution = m_restitution;
		pInt->SetParameters(params);
	}
}

void CFeatureCollision::Serialize(Serialization::IArchive& ar)
{
	ar(m_offset, "Offset", "Offset");
	ar(m_radius, "Radius", "Radius");
	ar(m_restitution, "Restitution", "Restitution");
}

}
