// Copyright 2014-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     06/04/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem.h"
#include "ParticleEffect.h"
#include "ParticleEmitter.h"
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/ZLib/IZLibCompressor.h>
#include "CryExtension/CryCreateClassInstance.h"
#include <CryString/StringUtils.h>
#include <CrySerialization/SmartPtr.h>

namespace pfx2
{

CRYREGISTER_SINGLETON_CLASS(CParticleSystem)

CParticleSystem::CParticleSystem()
	: m_threadData(gEnv->pJobManager->GetNumWorkerThreads() + 2) // 1 for main thread, 1 for each job thread, 1 for sum stats
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

PParticleEffect CParticleSystem::FindEffect(cstr name, bool bAllowLoad)
{
	auto it = m_effects.find(name);
	if (it != m_effects.end())
		return it->second;
	if (bAllowLoad)
		return LoadEffect(name);
	return nullptr;
}

PParticleEmitter CParticleSystem::CreateEmitter(PParticleEffect pEffect)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "pfx2::ParticleEmitter");
	PARTICLE_LIGHT_PROFILER();

	CRY_PFX2_ASSERT(pEffect.get());
	CParticleEffect* pCEffect = CastEffect(pEffect);
	pCEffect->Compile();

	m_newEmitters.emplace_back(new CParticleEmitter(pCEffect, m_nextEmitterId++));
	return m_newEmitters.back();
}

TParticleAttributesPtr CParticleSystem::CreateParticleAttributes() const
{
	return TParticleAttributesPtr(new CAttributeInstance());
}

void CParticleSystem::OnFrameStart()
{
}

void CParticleSystem::TrimEmitters(bool finished_only)
{
	stl::find_and_erase_all_if(m_emitters, [=](const _smart_ptr<CParticleEmitter>& emitter) -> bool
	{
		if (!emitter->IsIndependent())
			return false;
		if (finished_only)
		{
			if (emitter->IsAlive())
				return false;
		}
		return true;
	});
}

void CParticleSystem::InvalidateCachedRenderObjects()
{
	// Render objects and PSOs need to be updated when sys_spec changes.
	static ICVar* pSysSpecCVar = gEnv->pConsole->GetCVar("sys_spec");
	if (pSysSpecCVar)
	{
		const int32 sysSpec = pSysSpecCVar->GetIVal();

		CRY_ASSERT(sysSpec != END_CONFIG_SPEC_ENUM);
		const bool bInvalidate = ((m_lastSysSpec != END_CONFIG_SPEC_ENUM) && (m_lastSysSpec != sysSpec));
		m_lastSysSpec = sysSpec;

		if (bInvalidate)
		{
			for (auto& pEmitter : m_emitters)
			{
				pEmitter->Restart();
			}
		}
	}
}

void CParticleSystem::Update()
{
	CRY_PFX2_PROFILE_DETAIL;
	PARTICLE_LIGHT_PROFILER();

	const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
	const QuatT currentCameraPose = QuatT(camera.GetMatrix());
	if (m_cameraMotion.q.GetLength() == 0.0f)
	{
		m_cameraMotion = IDENTITY;
	}
	else
	{
		m_cameraMotion.t = currentCameraPose.t - m_lastCameraPose.t;
		m_cameraMotion.q = currentCameraPose.q * m_lastCameraPose.q.GetInverted();
	}

	if (GetCVars()->e_Particles)
	{
		auto gpuMan = gEnv->pRenderer->GetGpuParticleManager();
		gpuMan->BeginFrame();

		// Accumulate thread stats from last frame
		auto& sumData = GetSumData();
		sumData.statsCPU = {};
		sumData.statsGPU = {};
		for (auto& data : m_threadData)
		{
			CRY_PFX2_ASSERT(data.memHeap.GetTotalMemory().nUsed == 0);  // some emitter leaked memory on mem stack
			if (&data != &sumData)
			{
				sumData.statsCPU += data.statsCPU;
				data.statsCPU = {};
				sumData.statsGPU += data.statsGPU;
				data.statsGPU = {};
			}
		}

		TrimEmitters(!m_bResetEmitters);
		m_bResetEmitters = false;
		m_emitters.append(m_newEmitters);
		m_newEmitters.clear();

		InvalidateCachedRenderObjects();

		// Init stats for current frame
		auto& mainData = GetMainData();
		mainData.statsCPU = {};
		mainData.statsGPU = {};

		mainData.statsCPU.emitters.alloc = m_emitters.size();
		for (auto& pEmitter : m_emitters)
		{
			mainData.statsCPU.components.alloc += pEmitter->GetRuntimes().size();
			if (pEmitter->IsAlive())
			{
				mainData.statsCPU.emitters.alive++;
				mainData.statsCPU.emitters.updated++;
				pEmitter->Update();
				m_jobManager.AddEmitter(pEmitter);
			}
			else
			{
				pEmitter->Unregister();
			}
		}

		m_jobManager.ScheduleUpdates();
	}
}

void CParticleSystem::FinishUpdate()
{
	CRY_PFX2_PROFILE_DETAIL;

	m_jobManager.SynchronizeUpdates();

	const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
	m_lastCameraPose = QuatT(camera.GetMatrix());
}

void CParticleSystem::DeferredRender()
{
	m_jobManager.DeferredRender();
	DebugParticleSystem(m_emitters);

	const bool debugBBox = (GetCVars()->e_ParticlesDebug & AlphaBit('b')) != 0;
	if (debugBBox)
	{
		for (auto& pEmitter : m_emitters)
			pEmitter->DebugRender();
	}
}

void DisplayStatsHeader(Vec2& displayLocation, float lineHeight, cstr name)
{
	const char* titleLabel = "%-12s: Rendered /  Updated /    Alive /    Alloc";
	gEnv->p3DEngine->DrawTextRightAligned(
		displayLocation.x, displayLocation.y,
		titleLabel,
		name);
	displayLocation.y += lineHeight;
}

void DisplayElementStats(Vec2& displayLocation, float lineHeight, cstr name, const TElementCounts<float>& stats, int prec = 0)
{
	if (stats.alloc)
	{
		const char* emittersLabel = " %-11s: %8.*f / %8.*f / %8.*f / %8.*f";
		gEnv->p3DEngine->DrawTextRightAligned(
			displayLocation.x, displayLocation.y,
			emittersLabel,
			name, prec, stats.rendered, prec, stats.updated, prec, stats.alive, prec, stats.alloc, prec);
		displayLocation.y += lineHeight;
	}
}

template<typename TStats>
void DisplayParticleStats(Vec2& displayLocation, float lineHeight, cstr name, const TStats& stats)
{
	DisplayStatsHeader(displayLocation, lineHeight, name);
	DisplayElementStats(displayLocation, lineHeight, "Emitters", stats.emitters);
	DisplayElementStats(displayLocation, lineHeight, "Components", stats.components);
	DisplayElementStats(displayLocation, lineHeight, "Particles", reinterpret_cast<const TElementCounts<float>&>(stats.particles));
}

float CParticleSystem::DisplayDebugStats(Vec2 displayLocation, float lineHeight)
{
	float blendTime = GetTimer()->GetCurrTime();
	int blendMode = 0;
	float blendCur = GetTimer()->GetProfileFrameBlending(&blendTime, &blendMode);

	static TParticleStats<float> statsCPUAvg, statsGPUAvg;
	TParticleStats<float> statsCPUCur, statsGPUCur; 
	statsCPUCur.Set(GetSumData().statsCPU);
	statsCPUAvg = Lerp(statsCPUAvg, statsCPUCur, blendCur);
	statsGPUCur.Set(GetSumData().statsGPU);
	statsGPUAvg = Lerp(statsGPUAvg, statsGPUCur, blendCur);

	if (statsCPUAvg.emitters.alloc)
		DisplayParticleStats(displayLocation, lineHeight, "Wavicle CPU", statsCPUAvg);
	if (statsGPUAvg.components.alloc)
		DisplayParticleStats(displayLocation, lineHeight, "Wavicle GPU", statsGPUAvg);

	if (m_pPartManager)
	{
		static SParticleCounts countsAvg;
		SParticleCounts counts;
		m_pPartManager->GetCounts(counts);
		countsAvg = Lerp(countsAvg, counts, blendCur);

		if (countsAvg.emitters.alloc)
		{
			DisplayParticleStats(displayLocation, lineHeight, "Particles V1", countsAvg);

			TElementCounts<float> fill;
			float screenPix = (float)(GetRenderer()->GetWidth() * GetRenderer()->GetHeight());
			fill.rendered = countsAvg.pixels.rendered / screenPix;
			fill.updated = countsAvg.pixels.updated / screenPix;
			fill.alloc = 1.0f;
			DisplayElementStats(displayLocation, lineHeight, "Scr Fill", fill, 3);
		}
	}

	return displayLocation.y;
}

void CParticleSystem::ClearRenderResources()
{
#if !defined(_RELEASE)
	for (const auto& pEmitter : m_emitters)
	{
		CRY_PFX2_ASSERT(pEmitter->Unique()); // All external references need to be released before this point to prevent leaks
	}
#endif

	m_emitters.clear();

	for (auto it = m_effects.begin(); it != m_effects.end(); )
	{
		if (!it->second || it->second->Unique())
			it = m_effects.erase(it);
		else
			++it;
	}
}

float CParticleSystem::GetMaxAngularDensity(const CCamera& camera)
{
	return camera.GetAngularResolution() / max(GetCVars()->e_ParticlesMinDrawPixels, 0.125f) * 2.0f;
}

IMaterial* CParticleSystem::GetFlareMaterial()
{
	if (!m_pFlareMaterial)
	{
		const char* flareMaterialName = "%ENGINE%/EngineAssets/Materials/lens_optics";
		m_pFlareMaterial = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(flareMaterialName);
	}
	return m_pFlareMaterial;
}

void CParticleSystem::Reset()
{
	m_bResetEmitters = true;
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
		RenameEffect(pEffect, effectName);
		if (Serialization::LoadJsonFile(*CastEffect(pEffect), effectName))
			return pEffect;
	}

	m_effects[effectName] = _smart_ptr<CParticleEffect>();
	return PParticleEffect();
}

const SParticleFeatureParams* CParticleSystem::GetDefaultFeatureParam(EFeatureType type)
{
	for (const auto& feature : GetFeatureParams())
		if (feature.m_defaultForType == type)
			return &feature;
	return nullptr;
}

bool CParticleSystem::SerializeFeatures(IArchive& ar, TParticleFeatures& features, cstr name, cstr label) const
{
	if (ar.isInput())
		features.m_editVersion++;
	return ar(features, name, label);
}

void CParticleSystem::GetStats(SParticleStats& stats)
{
	stats = GetSumData().statsCPU + GetSumData().statsGPU;
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
