// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     06/04/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#pragma once

#include <CryExtension/ClassWeaver.h>
#include "ParticleCommon.h"
#include "ParticleDebug.h"
#include "ParticleJobManager.h"
#include "ParticleProfiler.h"

class CParticleEffect;

namespace pfx2
{

class CParticleEffect;
class CParticleEmitter;

class CParticleSystem : public Cry3DEngineBase, public IParticleSystem
{
	CRYINTERFACE_SIMPLE(IParticleSystem)
	CRYGENERATE_SINGLETONCLASS(CParticleSystem, "CryEngine_ParticleSystem", 0xCD8D738D54B446F7, 0x82BA23BA999CF2AC)

private:
	typedef std::vector<_smart_ptr<CParticleEmitter>>                                                                     TParticleEmitters;
	typedef std::unordered_map<string, _smart_ptr<CParticleEffect>, stl::hash_stricmp<string>, stl::hash_stricmp<string>> TEffectNameMap;

public:
	// IParticleSystem
	PParticleEffect         CreateEffect() override;
	PParticleEffect         ConvertEffect(const ::IParticleEffect* pOldEffect, bool bReplace) override;
	void                    RenameEffect(PParticleEffect pEffect, cstr name) override;
	PParticleEffect         FindEffect(cstr name) override;
	PParticleEmitter        CreateEmitter(PParticleEffect pEffect) override;
	uint                    GetNumFeatureParams() const override;
	SParticleFeatureParams& GetFeatureParam(uint featureIdx) const override;

	void                    OnFrameStart() override;
	void                    Update() override;
	void                    Reset() override;

	void                    Serialize(TSerialize ser) override;

	void                    GetCounts(SParticleCounts& counts) override;
	void                    GetMemoryUsage(ICrySizer* pSizer) const override;
	// ~IParticleSystem

	float                GetMaxAngularDensity() const     { return m_maxAngularDensity; }
	PParticleEffect      LoadEffect(cstr effectName);
	SParticleCounts&     GetCounts()                      { return m_counts; }
	TParticleHeap&       GetMemHeap(uint32 threadId = ~0) { return m_memHeap[threadId + 1]; }
	CParticleJobManager& GetJobManager()                  { return m_jobManager; }
	CParticleProfiler&   GetProfiler()                    { return m_profiler; }
	void                 SyncronizeUpdateKernels();

	void                 ClearRenderResources();

private:
	void              UpdateGpuRuntimesForEmitter(CParticleEmitter* pEmitter);
	void              UpdateEngineData();
	void              TrimEmitters();
	CParticleEffect*  CastEffect(const PParticleEffect& pEffect) const;
	CParticleEmitter* CastEmitter(const PParticleEmitter& pEmitter) const;

	// PFX1 to PFX2
	string ConvertPfx1Name(cstr oldEffectName);
	// ~PFX1 to PFX2

	SParticleCounts            m_counts;

	CParticleJobManager        m_jobManager;
	CParticleProfiler          m_profiler;
	TEffectNameMap             m_effects;
	TParticleEmitters          m_emitters;
	std::vector<TParticleHeap> m_memHeap;
	float                      m_maxAngularDensity;
};

std::vector<SParticleFeatureParams>& GetFeatureParams();

CParticleSystem*                     GetPSystem();

uint                                 GetVersion(Serialization::IArchive& ar);

}

#endif // PARTICLESYSTEM_H
