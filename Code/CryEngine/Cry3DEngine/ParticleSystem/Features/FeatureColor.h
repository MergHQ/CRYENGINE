// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParamMod.h"
#include "ParticleSystem/ParticleFeature.h"
#include "ParticleSystem/ParticleDataStreams.h"

namespace pfx2
{

using CParamModColor = CParamMod<EDD_ParticleUpdate, UColor>;

class CFeatureFieldColor : public CParticleFeature, CParamModColor
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void Serialize(Serialization::IArchive& ar) override;
	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override;
	virtual void InitParticles(CParticleComponentRuntime& runtime) override;
	virtual void UpdateParticles(CParticleComponentRuntime& runtime) override;
};

}
