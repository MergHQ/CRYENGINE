// Copyright 2015-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     13/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleJobManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleComponentRuntime.h"
#include "ParticleSystem.h"
#include "ParticleProfiler.h"
#include <CryRenderer/IGpuParticles.h>

static const uint MaxJobsPerThread = 16;

DECLARE_JOB("Particles : ScheduleUpdates"    , TScheduleUpdatesJob    , pfx2::CParticleJobManager::Job_ScheduleUpdates);
DECLARE_JOB("Particles : UpdateEmitters"     , TUpdateEmittersJob     , pfx2::CParticleJobManager::Job_UpdateEmitters);

namespace pfx2
{

void CParticleJobManager::AddDeferredRender(CParticleComponentRuntime* pRuntime, const SRenderContext& renderContext)
{
	SDeferredRender render(pRuntime, renderContext);
	m_deferredRenders.push_back(render);
}

void CParticleJobManager::ScheduleComputeVertices(CParticleComponentRuntime* pComponentRuntime, CRenderObject* pRenderObject, const SRenderContext& renderContext)
{
	CParticleManager* pPartManager = static_cast<CParticleManager*>(gEnv->pParticleManager);

	SAddParticlesToSceneJob& job = pPartManager->GetParticlesToSceneJob(renderContext.m_passInfo);
	auto pGpuRuntime = pComponentRuntime->GetGpuRuntime();
	auto pCpuRuntime = pComponentRuntime->GetCpuRuntime();
	if (pGpuRuntime)
		job.pGpuRuntime = pGpuRuntime;
	else if (pCpuRuntime)
		job.pVertexCreator = pCpuRuntime;
	job.pRenderObject = pRenderObject;
	job.pShaderItem = &pRenderObject->m_pCurrMaterial->GetShaderItem();
	job.nCustomTexId = renderContext.m_renderParams.nTextureID;
}

#pragma optimize("", off)

void CParticleJobManager::ScheduleUpdates()
{
	if (m_emitterRefs.empty())
		return;

	CRY_PFX2_PROFILE_DETAIL;

	CRY_PFX2_ASSERT(!m_updateState.IsRunning());
	if (!Cry3DEngineBase::GetCVars()->e_ParticlesThread)
	{
		for (auto pEmitter : m_emitterRefs)
			pEmitter->UpdateAll();
		return;
	}

	m_updateState.SetRunning();

	// Split emitter list into jobs
	const uint maxJobs = gEnv->pJobManager->GetNumWorkerThreads() * MaxJobsPerThread;
	const uint numJobs = min(m_emitterRefs.size(), maxJobs);

	m_updateState.SetRunning(numJobs);
	uint e = 0;

	std::vector<uint> jobParticles(numJobs);
	auto frameRate = gEnv->pTimer->GetFrameRate();

	for (uint j = 0; j < numJobs; ++j)
	{
		uint e2 = (j+1) * m_emitterRefs.size() / numJobs;
		TUpdateEmittersJob job(m_emitterRefs(e, e2 - e));
		for (; e < e2; ++e)
		{
			CParticleEmitter* pEmitter = m_emitterRefs[e];
			uint numParticles = 0;
			for (auto const& runtime : pEmitter->GetRuntimes())
			{
				CParticleComponentRuntime* pRuntime = runtime;
				int total, perframe; 
				pRuntime->GetComponent()->GetMaxParticleCounts(total, perframe, frameRate, frameRate);
				numParticles += total;
			}
			jobParticles[j] += numParticles;
		}
		e = e2;
		job.SetClassInstance(this);
		job.Run();
	}
	CRY_PFX2_ASSERT(e == m_emitterRefs.size());
	m_updateState.SetStopped();
}

void CParticleJobManager::SynchronizeUpdates()
{
	CRY_PFX2_PROFILE_DETAIL;
	gEnv->pJobManager->WaitForJob(m_updateState);
}

void CParticleJobManager::DeferredRender()
{
	CRY_PFX2_PROFILE_DETAIL;

	for (const SDeferredRender& render : m_deferredRenders)
	{
		CParticleComponentRuntime* pRuntime = render.m_pRuntime;
		CParticleComponent* pComponent = pRuntime->GetComponent();
		CParticleEmitter* pEmitter = pRuntime->GetEmitter();
		SRenderContext renderContext(render.m_rParam, render.m_passInfo);
		renderContext.m_distance = render.m_distance;
		renderContext.m_lightVolumeId = render.m_lightVolumeId;
		renderContext.m_fogVolumeId = render.m_fogVolumeId;
		pRuntime->GetComponent()->RenderDeferred(pEmitter, pRuntime, pComponent, renderContext);
	}

	ClearAll();
}

void CParticleJobManager::Job_UpdateEmitters(TVarArray<CParticleEmitter*> emitters)
{
	for (auto pEmitter : emitters)
		pEmitter->UpdateAll();
	m_updateState.SetStopped();
}

void CParticleJobManager::ClearAll()
{
	CRY_PFX2_PROFILE_DETAIL;

	CRY_PFX2_ASSERT(!m_updateState.IsRunning());
	m_deferredRenders.clear();
	m_emitterRefs.clear();
}

}
