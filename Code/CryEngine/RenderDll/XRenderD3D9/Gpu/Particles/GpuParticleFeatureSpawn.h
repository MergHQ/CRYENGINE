// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     02/10/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GpuParticleFeatureBase.h"

namespace gpu_pfx2
{

struct CFeatureSpawnBase : public CFeatureWithParameterStruct<SFeatureParametersSpawn>
{
	virtual void InitSubInstance(IParticleComponentRuntime* pRuntime, SSpawnData* pInstances, size_t count) override;
protected:
	bool         HasDuration() { return m_useDuration; }
private:
	bool m_useDuration;
};

struct CFeatureSpawnCount : public CFeatureSpawnBase
{
	static const EGpuFeatureType type = eGpuFeatureType_SpawnCount;
	virtual void SpawnParticles(const gpu_pfx2::SUpdateContext& context) override;
};

struct CFeatureSpawnRate : public CFeatureSpawnBase
{
	static const EGpuFeatureType type = eGpuFeatureType_SpawnRate;
	virtual void InternalSetParameters(const EParameterType type, const SFeatureParametersBase& p) override;
	virtual void SpawnParticles(const gpu_pfx2::SUpdateContext& context) override;
private:
	ESpawnRateMode m_mode;
};

}
