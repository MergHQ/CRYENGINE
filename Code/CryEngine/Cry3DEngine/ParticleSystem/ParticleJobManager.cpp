// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleJobManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleComponentRuntime.h"
#include "ParticleSystem.h"
#include "ParticleProfiler.h"
#include <CryRenderer/IGpuParticles.h>
#include <CrySystem/ConsoleRegistration.h>

namespace pfx2
{

int e_ParticlesJobsPerThread = 16;

ILINE bool EmitterHasDeferred(CParticleEmitter* pEmitter)
{
	return !pEmitter->GetCEffect()->RenderDeferred.empty();
}

CParticleJobManager::CParticleJobManager()
{
	REGISTER_CVAR(e_ParticlesJobsPerThread, 16, 0, "Maximum particle jobs to assign per worker thread");
}

void CParticleJobManager::AddDeferredRender(CParticleEmitter* pEmitter, const SRenderContext& renderContext)
{
	m_deferredRenders.emplace_back(pEmitter, renderContext);
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
}

void CParticleJobManager::AddUpdateEmitter(CParticleEmitter* pEmitter)
{
	int threadMode = ThreadMode();

	if (threadMode == 0)
	{
		// Update synchronously in main thread
		pEmitter->UpdateParticles();
	}
	else if (threadMode > 0 && threadMode <= 4)
	{
		// Schedule emitters rendered last frame first
		if (threadMode >= 2 && pEmitter->WasRenderedLastFrame())
		{
			if (threadMode >= 4 && EmitterHasDeferred(pEmitter))
				m_emittersDeferred.push_back(pEmitter);
			else
				m_emittersVisible.push_back(pEmitter);
		}
		else if (threadMode < 3 || !pEmitter->IsStable())
			m_emittersInvisible.push_back(pEmitter);
	}
	else // if (threadMode >= 5)
	{
		auto job = [pEmitter]() { pEmitter->UpdateParticles(); };
		if (pEmitter->WasRenderedLastFrame())
		{
			if (EmitterHasDeferred(pEmitter))
			{
				gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitter (deferred)", job, JobManager::eHighPriority, &m_updateState);
			}
			else
			{
				gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitter (visible)", job, JobManager::eRegularPriority, &m_updateState);
			}
		}
		else if (!pEmitter->IsStable())
		{
			gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitter (invisible)", job, JobManager::eStreamPriority, &m_updateState);
		}
	}
}

void CParticleJobManager::ScheduleUpdateEmitter(CParticleEmitter* pEmitter, JobManager::TPriorityLevel priority)
{
	auto job = [pEmitter]() { pEmitter->UpdateParticles(); };
	gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitter", job, priority, &m_updateState);
}

void CParticleJobManager::ScheduleUpdates()
{
	if (ThreadMode() >= 5)
		return;

	// Schedule jobs in a high-priority job
	CRY_PFX2_ASSERT(!m_updateState.IsRunning());

	auto jobD = [this]() { Job_ScheduleUpdates<Update_Deferred >(); };
	auto jobV = [this]() { Job_ScheduleUpdates<Update_Visible  >(); };
	auto jobI = [this]() { Job_ScheduleUpdates<Update_Invisible>(); };

	gEnv->pJobManager->AddLambdaJob("job:pfx2:ScheduleUpdates (deferred)" , jobD, JobManager::eHighPriority   , &m_updateState);
	gEnv->pJobManager->AddLambdaJob("job:pfx2:ScheduleUpdates (visible)"  , jobV, JobManager::eRegularPriority, &m_updateState);
	gEnv->pJobManager->AddLambdaJob("job:pfx2:ScheduleUpdates (invisible)", jobI, JobManager::eStreamPriority , &m_updateState);
}

template<CParticleJobManager::Update_Type type> void CParticleJobManager::Job_ScheduleUpdates()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (type == Update_Deferred)
	{
		if (m_emittersDeferred.size())
		{
			// Schedule deferred emitters in individual high-priority jobs
			// We kick of N jobs to push those jobs, as overhead is rather large
			ScheduleUpdateEmitters<true>(m_emittersDeferred, JobManager::eHighPriority);
		}
	}
	else if (type == Update_Visible)
	{
		if (m_emittersVisible.size())
		{
			// Sort fast (visible) emitters by camera Z
			const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
			Vec3 sortDir = -camera.GetViewdir();
			stl::sort(m_emittersVisible, [sortDir](const CParticleEmitter* pe)
			{
				return pe->GetLocation().t | sortDir;
			});

			ScheduleUpdateEmitters<false>(m_emittersVisible, JobManager::eRegularPriority);
		}
	}
	else // if (type == Update_Invisible)
	{
		if (m_emittersInvisible.size())
		{
			ScheduleUpdateEmitters<false>(m_emittersInvisible, JobManager::eStreamPriority);
		}
	}
}

template<bool scheduleJobs> void CParticleJobManager::ScheduleUpdateEmitters(TDynArray<CParticleEmitter*>& emitters, JobManager::TPriorityLevel priority)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CParticleJobManager* pManager = this;

	// Batch updates into jobs
	const uint maxJobs = gEnv->pJobManager->GetNumWorkerThreads() * e_ParticlesJobsPerThread;
	const uint numJobs = min(emitters.size(), maxJobs);

	uint e = 0;
	for (uint j = 0; j < numJobs; ++j)
	{
		uint e2 = (j + 1) * emitters.size() / numJobs;
		auto emitterGroup = emitters(e, e2 - e);
		e = e2;

		auto job = [pManager, priority, emitterGroup]()
		{
			for (auto pEmitter : emitterGroup)
			{
				if (scheduleJobs)
					pManager->ScheduleUpdateEmitter(pEmitter, priority);
				else
					pEmitter->UpdateParticles();
			}
		};

		gEnv->pJobManager->AddLambdaJob("job:pfx2:UpdateEmitters", job, priority, &m_updateState);
	}

	CRY_PFX2_ASSERT(e == emitters.size());
}

void CParticleJobManager::SynchronizeUpdates()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	m_updateState.Wait();
	m_emittersInvisible.resize(0);
	m_emittersVisible.resize(0);
	m_emittersDeferred.resize(0);
}

void CParticleJobManager::DeferredRender()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	for (const SDeferredRender& render : m_deferredRenders)
	{
		SRenderContext renderContext(render.m_rParam, render.m_passInfo);
		renderContext.m_distance = render.m_distance;
		renderContext.m_lightVolumeId = render.m_lightVolumeId;
		renderContext.m_fogVolumeId = render.m_fogVolumeId;
		render.m_pEmitter->RenderDeferred(renderContext);
	}
	m_deferredRenders.pop_back(m_deferredRenders.size());
}


}
