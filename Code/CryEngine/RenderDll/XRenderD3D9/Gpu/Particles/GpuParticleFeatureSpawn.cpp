// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     02/10/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GpuParticleFeatureSpawn.h"
#include "GpuParticleComponentRuntime.h"

namespace {
const float fHUGE = 1e9f;
}

namespace gpu_pfx2
{

void CFeatureSpawnBase::InitSubInstance(IParticleComponentRuntime* pRuntime, SSpawnData* pInstances, size_t count)
{
	auto pGpuRuntime = static_cast<CParticleComponentRuntime*>(pRuntime);
	for (size_t i = 0; i < count; ++i)
	{
		SSpawnData& spawn = pInstances[i];
		spawn.timer = 0.0f;
		spawn.spawned = 0.0f;
		const auto& params = GetParameters();
		m_useDuration = params.useDuration;
		spawn.amount = params.amount;
		spawn.delay = params.useDelay ? params.delay : 0.0f;
		spawn.duration = m_useDuration ? params.duration : 0.0f;
	}
}

void CFeatureSpawnCount::SpawnParticles(const gpu_pfx2::SUpdateContext& context)
{
	CParticleComponentRuntime* pRuntime = (CParticleComponentRuntime*)(context.pRuntime);

	const size_t numInstances = pRuntime->GetNumInstances();

	for (size_t i = 0; i < numInstances; ++i)
	{
		SSpawnData& spawn = pRuntime->GetSpawnData(i);

		const float amount = spawn.amount;
		const int spawnedBefore = int(spawn.spawned);
		const float spawning = spawn.timer >= spawn.delay ? 1.0f : 0.0f;
		const float rate = amount * __fres(max(FLT_EPSILON, spawn.duration)) * spawning;
		spawn.spawned = min(amount, spawn.spawned + rate * context.deltaTime);
		spawn.timer += context.deltaTime;
		const int spawnedAfter = max(spawnedBefore, int(spawn.spawned));
		pRuntime->SpawnParticles(i, spawnedAfter - spawnedBefore);
	}
}

void CFeatureSpawnRate::InternalSetParameters(const EParameterType type, const SFeatureParametersBase& p)
{
	if (type == EParameterType::SpawnMode)
	{
		const SFeatureParametersSpawnMode& parameters = p.GetParameters<SFeatureParametersSpawnMode>();
		m_mode = parameters.mode;
	}
	else
		CFeatureSpawnBase::InternalSetParameters(type, p);
}

void CFeatureSpawnRate::SpawnParticles(const gpu_pfx2::SUpdateContext& context)
{
	CParticleComponentRuntime* pRuntime = (CParticleComponentRuntime*)(context.pRuntime);
	const size_t numInstances = pRuntime->GetNumInstances();

	for (size_t i = 0; i < pRuntime->GetNumInstances(); ++i)
	{
		SSpawnData& spawn = pRuntime->GetSpawnData(i);
		const float amount = spawn.amount;
		const int spawnedBefore = int(spawn.spawned);
		const float endTime = spawn.delay + (HasDuration() ? spawn.duration : fHUGE);
		const float spawning = spawn.timer >= spawn.delay && spawn.timer < endTime ? 1.0f : 0.0f;
		const float rate = (m_mode == ESpawnRateMode::ParticlesPerSecond ? amount : __fres(amount)) * spawning;
		spawn.spawned += rate * context.deltaTime;
		spawn.timer += context.deltaTime;
		const int spawnedAfter = max(spawnedBefore, int(spawn.spawned));
		pRuntime->SpawnParticles(i, spawnedAfter - spawnedBefore);
	}
}

}
