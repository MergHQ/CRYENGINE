// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     22/10/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"

CRY_PFX2_DBG

namespace pfx2
{

class CFeatureCollision : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureCollision()
		: CParticleFeature(gpu_pfx2::eGpuFeatureType_Collision)
		, m_offset(0.5f)
		, m_radius(0.5f)
		, m_restitution(0.5f)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override;
	virtual void Serialize(Serialization::IArchive& ar) override;
private:
	float m_offset;
	float m_radius;
	float m_restitution;
};

}
