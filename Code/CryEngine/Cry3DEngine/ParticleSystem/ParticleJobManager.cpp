// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleJobManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleComponentRuntime.h"
#include "ParticleSystem.h"
#include "ParticleProfiler.h"
#include <CryRenderer/IGpuParticles.h>

namespace pfx2
{

int e_ParticlesJobsPerThread = 16;

CParticleJobManager::CParticleJobManager()
{
	REGISTER_CVAR(e_ParticlesJobsPerThread, 16, 0, "Maximum particle jobs to assign per worker thread");
}

void CParticleJobManager::AddDeferredRender(CParticleEmitter* pEmitter, const SRenderContext& renderContext)
{
	SDeferredRender render(pEmitter, renderContext);
	m_deferredRenders.push_back(render);
}

void CParticleJobManager::ScheduleComputeVertices(CParticleComponentRuntime& runtime, CRenderObject* pRenderObject, const SRenderContext& renderContext)
{
	CParticleManager* pPartManager = static_cast<CParticleManager*>(gEnv->pParticleManager);

	SAddParticlesToSceneJob& job = pPartManager->GetParticlesToSceneJob(renderContext.m_passInfo);
	if (auto pGpuRuntime = runtime.GetGpuRuntime())
		job.pGpuRuntime = pGpuRuntime;
	else
		job.pVertexCreator = &runtime;
	job.pRenderObject = pRenderObject;
	job.pShaderItem = &pRenderObject->m_pCurrMaterial->GetShaderItem();
	job.nCustomTexId = renderContext.m_renderParams.nTextureID;
}

void CParticleJobManager::AddUpdateEmitter(CParticleEmitter* pEmitter)
{
	CRY_PFX2_ASSERT(!m_updateState.IsRunning());
	if (ThreadMode() == 0)
	{
		// Update synchronously in main thread
		pEmitter->UpdateParticles();
	}
	else
	{
		// Schedule emitters rendered last frame first
		if (ThreadMode() >= 2 && pEmitter->WasRenderedLastFrame())
			m_emitterRefsFast.push_back(pEmitter);
		else if (ThreadMode() < 3 || !pEmitter->IsStable())
			m_emitterRefs.push_back(pEmitter);
	}
}

void CParticleJobManager::ScheduleUpdateEmitter(CParticleEmitter* pEmitter)
{
	auto job = [pEmitter]()
	{
		pEmitter->UpdateParticles();
	};
	auto priority = ThreadMode() >= 4 && pEmitter->GetCEffect()->RenderDeferred.size() ? JobManager::eHighPriority : JobManager::eRegularPriority;
	gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitter", job, priority, &m_updateState);
}

void CParticleJobManager::ScheduleUpdates()
{
	if (m_emitterRefs.size() + m_emitterRefsFast.size() == 0)
		return;

	CRY_PFX2_ASSERT(!m_updateState.IsRunning());
	auto job = [this]()
	{
		Job_ScheduleUpdates();
	};
	gEnv->pJobManager->AddLambdaJob("job:pfx2:ScheduleUpdates", job, JobManager::eRegularPriority, &m_updateState);
}

void CParticleJobManager::Job_ScheduleUpdates()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (m_emitterRefsFast.size())
	{
		// Sort fast (visible) emitters by camera Z, and update in individual jobs
		const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
		Vec3 sortDir = -camera.GetViewdir();
		stl::sort(m_emitterRefsFast, [sortDir](const CParticleEmitter* pe)
		{
			return pe->GetLocation().t | sortDir;
		});

		for (auto pEmitter : m_emitterRefsFast)
		{
			ScheduleUpdateEmitter(pEmitter);
		}
		m_emitterRefsFast.clear();
	}

	// Batch slow emitters into jobs
	const uint maxJobs = gEnv->pJobManager->GetNumWorkerThreads() * e_ParticlesJobsPerThread;
	const uint numJobs = min(m_emitterRefs.size(), maxJobs);

	uint e = 0;
	for (uint j = 0; j < numJobs; ++j)
	{
		uint e2 = (j + 1) * m_emitterRefs.size() / numJobs;
		auto emitters = m_emitterRefs(e, e2 - e);
		e = e2;

		auto job = [emitters]()
		{
			for (auto pEmitter : emitters)
				pEmitter->UpdateParticles();
		};
		gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitters", job, JobManager::eStreamPriority, &m_updateState);
	}
	CRY_PFX2_ASSERT(e == m_emitterRefs.size());
}

void CParticleJobManager::SynchronizeUpdates()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	m_updateState.Wait();
	m_emitterRefs.clear();
}

void CParticleJobManager::DeferredRender()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	for (const SDeferredRender& render : m_deferredRenders)
	{
		CParticleEmitter* pEmitter = render.m_pEmitter;
		pEmitter->SyncUpdateParticles();
		SRenderContext renderContext(render.m_rParam, render.m_passInfo);
		renderContext.m_distance = render.m_distance;
		renderContext.m_lightVolumeId = render.m_lightVolumeId;
		renderContext.m_fogVolumeId = render.m_fogVolumeId;

		for (auto const& component: pEmitter->GetCEffect()->RenderDeferred)
		{
			if (auto pRuntime = pEmitter->GetRuntimeFor(component))
			{
				component->RenderDeferred(*pRuntime, renderContext);
			}
		}
	}
	m_deferredRenders.clear();
}


}
