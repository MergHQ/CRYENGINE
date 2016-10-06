// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     06/04/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/ZLib/IZLibCompressor.h>
#include "CryExtension/CryCreateClassInstance.h"
#include <CryString/StringUtils.h>
#include "ParticleSystem.h"
#include "ParticleEffect.h"
#include "ParticleEmitter.h"

CRY_PFX2_DBG

namespace pfx2
{

CRYREGISTER_SINGLETON_CLASS(CParticleSystem)

std::vector<SParticleFeatureParams> &GetFeatureParams()
{
	static std::vector<SParticleFeatureParams> featureParams;
	return featureParams;
}

CParticleSystem* GetPSystem()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	static std::shared_ptr<IParticleSystem> pSystem(GetIParticleSystem());
	return static_cast<CParticleSystem*>(pSystem.get());
};

CParticleSystem::CParticleSystem()
	: m_memHeap(gEnv->pJobManager->GetNumWorkerThreads() + 1)
{
}

CParticleSystem::~CParticleSystem()
{
}

PParticleEffect CParticleSystem::CreateEffect()
{
	return new pfx2::CParticleEffect();
}

void CParticleSystem::RenameEffect(PParticleEffect pEffect, cstr name)
{
	CRY_PFX2_ASSERT(pEffect.get());
	CParticleEffect* pCEffect = CastEffect(pEffect);

	if (pCEffect->GetName())
		m_effects[pCEffect->GetName()] = nullptr;
	pCEffect->SetName(name);
	m_effects[pCEffect->GetName()] = pCEffect;
}

PParticleEffect CParticleSystem::FindEffect(cstr name)
{
	auto it = m_effects.find(name);
	if (it != m_effects.end())
		return it->second;
	return LoadEffect(name);
}

PParticleEmitter CParticleSystem::CreateEmitter(PParticleEffect pEffect)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "pfx2::ParticleEmitter");
	PARTICLE_LIGHT_PROFILER();

	CRY_PFX2_ASSERT(pEffect.get());
	CParticleEffect* pCEffect = CastEffect(pEffect);
	pCEffect->Compile();

	_smart_ptr<CParticleEmitter> pEmitter = new CParticleEmitter(m_nextEmitterId++);
	pEmitter->SetCEffect(pCEffect);
	m_emitters.push_back(pEmitter);
	return pEmitter;
}

uint CParticleSystem::GetNumFeatureParams() const
{
	return uint(GetFeatureParams().size());
}

SParticleFeatureParams& CParticleSystem::GetFeatureParam(uint featureIdx) const
{
	return GetFeatureParams()[featureIdx];
}

void CParticleSystem::OnFrameStart()
{
}

void CParticleSystem::TrimEmitters()
{
	for (auto& pEmitter : m_emitters)
	{
		bool independent = pEmitter->IsIndependent();
		bool hasParticles = pEmitter->HasParticles();
		bool isAlive = pEmitter->IsAlive();
		bool expired = isAlive && independent && !hasParticles;
		if (!expired)
			continue;
		pEmitter = 0;
	}

	m_emitters.erase(
	  std::remove(m_emitters.begin(), m_emitters.end(), _smart_ptr<CParticleEmitter>(0)),
	  m_emitters.end());
}

void CParticleSystem::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	PARTICLE_LIGHT_PROFILER();

	const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
	const QuatT currentCameraPose = QuatT(camera.GetMatrix());
	m_cameraMotion.t = currentCameraPose.t - m_lastCameraPose.t;
	m_cameraMotion.q = currentCameraPose.q * m_lastCameraPose.q.GetInverted();
	m_lastCameraPose = currentCameraPose;

	if (GetCVars()->e_Particles)
	{
		auto gpuMan = gEnv->pRenderer->GetGpuParticleManager();
		gpuMan->BeginFrame();

		for (auto& it : m_memHeap)
			CRY_PFX2_ASSERT(it.GetTotalMemory().nUsed == 0);  // some emitter leaked memory on mem stack

		m_counts = SParticleCounts();
		for (auto& pEmitter : m_emitters)
			pEmitter->AccumCounts(m_counts);
		m_profiler.Display();

		TrimEmitters();

		for (auto& pEmitter : m_emitters)
		{
			pEmitter->Update();
			m_jobManager.AddEmitter(pEmitter);
		}

		for (auto& pEmitter : m_emitters)
			UpdateGpuRuntimesForEmitter(pEmitter);

		m_jobManager.KernelUpdateAll();
	}
}

void CParticleSystem::SyncronizeUpdateKernels()
{
	FUNCTION_PROFILER_3DENGINE;

	m_jobManager.SynchronizeUpdate();
	for (auto& pEmitter : m_emitters)
		pEmitter->PostUpdate();
}

void CParticleSystem::DeferredRender()
{
	m_jobManager.DeferredRender();
	DebugParticleSystem(m_emitters);
}

void CParticleSystem::ClearRenderResources()
{
	m_emitters.clear();
	for (auto it = m_effects.begin(); it != m_effects.end(); )
	{
		if (!it->second || it->second->Unique())
			it = m_effects.erase(it);
		else
			++it;
	}
}

void CParticleSystem::Reset()
{
}

void CParticleSystem::Serialize(TSerialize ser)
{
}

PParticleEffect CParticleSystem::LoadEffect(cstr effectName)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "pfx2::LoadEffect");
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleLibrary, 0, "Particle effect (%s)", effectName);

	if (gEnv->pCryPak->IsFileExist(effectName))
	{
		PParticleEffect pEffect = CreateEffect();
		if (Serialization::LoadJsonFile(*CastEffect(pEffect), effectName))
		{
			RenameEffect(pEffect, effectName);
			return pEffect;
		}
	}

	return PParticleEffect();
}

void CParticleSystem::UpdateGpuRuntimesForEmitter(CParticleEmitter* pEmitter)
{
	FUNCTION_PROFILER_3DENGINE

	auto gpuPfx = gEnv->pRenderer->GetGpuParticleManager();

	for (auto ref : pEmitter->GetRuntimes())
	{
		if (auto pGpuRuntime = ref.pRuntime->GetGpuRuntime())
		{
			const bool isActive = pGpuRuntime->IsActive();
			if (isActive)
			{
				gpu_pfx2::SEnvironmentParameters params;
				params.physAccel = pEmitter->GetPhysicsEnv().m_UniformForces.vAccel;
				params.physWind = pEmitter->GetPhysicsEnv().m_UniformForces.vWind;
				pGpuRuntime->SetEnvironmentParameters(params);
				pGpuRuntime->SetEmitterData(pEmitter);
			}
		}
	}
}

void CParticleSystem::GetCounts(SParticleCounts& counts)
{
	counts = m_counts;
}

void CParticleSystem::GetMemoryUsage(ICrySizer* pSizer) const
{
}

ILINE CParticleEffect* CParticleSystem::CastEffect(const PParticleEffect& pEffect) const
{
	return static_cast<CParticleEffect*>(pEffect.get());
}

ILINE CParticleEmitter* CParticleSystem::CastEmitter(const PParticleEmitter& pEmitter) const
{
	return static_cast<CParticleEmitter*>(pEmitter.get());
}

uint GetVersion(Serialization::IArchive& ar)
{
	SSerializationContext* pContext = ar.context<SSerializationContext>();
	if (!pContext)
		return gCurrentVersion;
	return pContext->m_documentVersion;
}

}
