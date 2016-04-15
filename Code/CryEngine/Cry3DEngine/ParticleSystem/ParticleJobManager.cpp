// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     13/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleComponentRuntime.h"
#include "ParticleSystem.h"
#include "ParticleProfiler.h"
#include "ParticleJobManager.h"
#include <CryRenderer/IGpuParticles.h>

CRY_PFX2_DBG

  DECLARE_JOB("ParticleKernel:AddRemoveParticles", TKernelAddRemoveJob, pfx2::CParticleJobManager::Job_AddRemoveParticles);
DECLARE_JOB("ParticleKernel:UpdateParticles", TKernelUpdateParticlesJob, pfx2::CParticleJobManager::Job_UpdateParticles);
DECLARE_JOB("ParticleKernel:PostUpdateParticles", TKernelPostUpdateParticlesJob, pfx2::CParticleJobManager::Job_PostUpdateParticles);
DECLARE_JOB("ParticleKernel:CalculateBounds", TKernelCalculateBoundsJob, pfx2::CParticleJobManager::Job_CalculateBounds);

namespace pfx2
{

stl::TPoolAllocator<TKernelPostUpdateParticlesJob>& GetPostJobPool()
{
	static stl::TPoolAllocator<TKernelPostUpdateParticlesJob> pool;
	return pool;
}

void CParticleJobManager::AddEmitter(CParticleEmitter* pEmitter)
{
	CRY_PFX2_ASSERT(!m_updateState.IsRunning());

	auto gpuPfx = gEnv->pRenderer->GetGpuParticleManager();

	for (auto ref : pEmitter->GetRuntimes())
	{
		if (auto pCpuRuntime = ref.pRuntime->GetCpuRuntime())
		{
			const SComponentParams& params = pCpuRuntime->GetComponentParams();
			const bool isActive = pCpuRuntime->IsActive();
			const bool isSecondGen = params.IsSecondGen();
			if (isActive && !isSecondGen)
			{
				size_t refIdx = m_componentRefs.size();
				m_firstGenComponentsRef.push_back(refIdx);
				m_componentRefs.push_back(SComponentRef(pCpuRuntime));
				SComponentRef& componentRef = m_componentRefs.back();
				componentRef.m_firstChild = m_componentRefs.size();
				componentRef.m_pPostSubUpdates = GetPostJobPool().New(m_componentRefs.size() - 1);
				componentRef.m_pPostSubUpdates->SetClassInstance(this);
				AddComponentRecursive(pEmitter, refIdx);
			}
		}
	}
}

void CParticleJobManager::AddComponentRecursive(CParticleEmitter* pEmitter, size_t parentRefIdx)
{
	const CParticleComponentRuntime* pParentComponentRuntime = m_componentRefs[parentRefIdx].m_pComponentRuntime;
	const SComponentParams& parentParams = pParentComponentRuntime->GetComponentParams();

	for (auto& childComponentId : parentParams.m_subComponentIds)
	{
		ICommonParticleComponentRuntime* pCommonRuntime =
		  pEmitter->GetRuntimes()[childComponentId].pRuntime;
		if (auto pChildComponent = pCommonRuntime->GetCpuRuntime())
		{
			const bool isActive = pChildComponent->IsActive();
			if (isActive)
			{
				const SComponentParams& childParams = pChildComponent->GetComponentParams();
				m_componentRefs.push_back(SComponentRef(pChildComponent));
				SComponentRef& componentRef = m_componentRefs.back();
				componentRef.m_pPostSubUpdates = GetPostJobPool().New(m_componentRefs.size() - 1);
				componentRef.m_pPostSubUpdates->SetClassInstance(this);
				++m_componentRefs[parentRefIdx].m_numChildren;
			}
		}
	}

	size_t numChildrend = m_componentRefs[parentRefIdx].m_numChildren;
	size_t firstChild = m_componentRefs[parentRefIdx].m_firstChild;
	for (size_t i = 0; i < numChildrend; ++i)
	{
		size_t childRefIdx = firstChild + i;
		m_componentRefs[childRefIdx].m_firstChild = m_componentRefs.size();
		AddComponentRecursive(pEmitter, childRefIdx);
	}
}

void CParticleJobManager::ScheduleComputeVertices(CParticleComponentRuntime* pComponentRuntime, CRenderObject* pRenderObject, const SRenderContext& renderContext)
{
	CParticleManager* pPartManager = static_cast<CParticleManager*>(gEnv->pParticleManager);
	const SComponentParams& params = pComponentRuntime->GetComponentParams();

	SAddParticlesToSceneJob& job = pPartManager->GetParticlesToSceneJob(renderContext.m_passInfo);
	job.pPVC = pComponentRuntime;
	job.pRenderObject = pRenderObject;
	job.pShaderItem = &params.m_pMaterial->GetShaderItem();
	job.nCustomTexId = renderContext.m_renderParams.nTextureID;
}

void CParticleJobManager::KernelUpdateAll()
{
	if (m_firstGenComponentsRef.empty())
		return;

	FUNCTION_PROFILER_3DENGINE

	  CRY_PFX2_ASSERT(!m_updateState.IsRunning());

	CVars* pCVars = static_cast<C3DEngine*>(gEnv->p3DEngine)->GetCVars();

	if (pCVars->e_ParticlesThread)
	{
		for (size_t i = 0; i < m_componentRefs.size(); ++i)
			m_updateState.SetRunning();
		for (size_t idx : m_firstGenComponentsRef)
		{
			SComponentRef& componentRef = m_componentRefs[idx];
			TKernelAddRemoveJob job(idx);
			job.SetClassInstance(this);
			job.Run();
		}
	}
	else
	{
		for (auto& componentRef : m_componentRefs)
		{
			SUpdateContext context = SUpdateContext(componentRef.m_pComponentRuntime);
			componentRef.m_pComponentRuntime->AddRemoveNewBornsParticles(context);
		}
		for (auto& componentRef : m_componentRefs)
		{
			SUpdateContext context = SUpdateContext(componentRef.m_pComponentRuntime);
			if (context.m_container.GetLastParticleId() != 0)
				componentRef.m_pComponentRuntime->UpdateParticles(context);
			componentRef.m_pComponentRuntime->CalculateBounds();
		}
	}
}

void CParticleJobManager::SynchronizeUpdate()
{
	if (m_firstGenComponentsRef.empty())
		return;

	gEnv->pJobManager->WaitForJob(m_updateState);

	ClearAll();
}

void CParticleJobManager::Job_AddRemoveParticles(uint componentRefIdx)
{
	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	CParticleComponentRuntime* pRuntime = m_componentRefs[componentRefIdx].m_pComponentRuntime;
	GetPSystem()->GetProfiler().AddEntry(pRuntime, EPS_Jobs);

	pRuntime->WaitUntilUpdateEnded();
	pRuntime->SetStateUpdateStarted();
	DoAddRemove(m_componentRefs[componentRefIdx]);
	ScheduleUpdateParticles(componentRefIdx);
}

void CParticleJobManager::Job_UpdateParticles(uint componentRefIdx, SUpdateRange updateRange)
{
	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	CParticleComponentRuntime* pRuntime = m_componentRefs[componentRefIdx].m_pComponentRuntime;
	GetPSystem()->GetProfiler().AddEntry(pRuntime, EPS_Jobs);

	SUpdateContext context = SUpdateContext(pRuntime, updateRange);
	pRuntime->UpdateParticles(context);
}

void CParticleJobManager::Job_PostUpdateParticles(uint componentRefIdx)
{
	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	CParticleComponentRuntime* pRuntime = m_componentRefs[componentRefIdx].m_pComponentRuntime;
	GetPSystem()->GetProfiler().AddEntry(pRuntime, EPS_Jobs);

	pRuntime->SetStateUpdateEnded();
	ScheduleChildrenComponents(m_componentRefs[componentRefIdx]);
	ScheduleCalculateBounds(componentRefIdx);
}

void CParticleJobManager::Job_CalculateBounds(uint componentRefIdx)
{
	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	CParticleComponentRuntime* pRuntime = m_componentRefs[componentRefIdx].m_pComponentRuntime;
	GetPSystem()->GetProfiler().AddEntry(pRuntime, EPS_Jobs);

	pRuntime->CalculateBounds();
	m_updateState.SetStopped();
}

void CParticleJobManager::ScheduleUpdateParticles(uint componentRefIdx)
{
	SComponentRef& componentRef = m_componentRefs[componentRefIdx];
	componentRef.m_subUpdateState.SetRunning();

	componentRef.m_subUpdateState.RegisterPostJob(componentRef.m_pPostSubUpdates);

	const size_t particleCountThreshold = 1024 * 8;
	const CParticleContainer& container = componentRef.m_pComponentRuntime->GetContainer();
	const TParticleId lastParticleId = container.GetLastParticleId();

	for (TParticleId pId = 0; pId < lastParticleId; pId += particleCountThreshold)
	{
		SUpdateRange range;
		range.m_firstParticleId = pId;
		range.m_lastParticleId = MIN(pId + particleCountThreshold, lastParticleId);

		TKernelUpdateParticlesJob job(componentRefIdx, range);
		job.RegisterJobState(&componentRef.m_subUpdateState);
		job.SetClassInstance(this);
		job.Run();
	}

	componentRef.m_subUpdateState.SetStopped();
}

void CParticleJobManager::ScheduleChildrenComponents(SComponentRef& componentRef)
{
	for (size_t i = 0; i < componentRef.m_numChildren; ++i)
	{
		TKernelAddRemoveJob job(componentRef.m_firstChild + i);
		job.SetClassInstance(this);
		job.Run();
	}
}

void CParticleJobManager::ScheduleCalculateBounds(uint componentRefIdx)
{
	TKernelCalculateBoundsJob calculateBoundsJob(componentRefIdx);
	calculateBoundsJob.SetClassInstance(this);
	calculateBoundsJob.Run();
}

void CParticleJobManager::DoAddRemove(const SComponentRef& componentRef)
{
	SUpdateContext context = SUpdateContext(componentRef.m_pComponentRuntime);
	componentRef.m_pComponentRuntime->AddRemoveNewBornsParticles(context);
}

void CParticleJobManager::ClearAll()
{
	CRY_PFX2_ASSERT(!m_updateState.IsRunning());
	for (auto& componentRef : m_componentRefs)
		GetPostJobPool().Delete(componentRef.m_pPostSubUpdates);
	m_firstGenComponentsRef.clear();
	m_componentRefs.clear();
}

}
