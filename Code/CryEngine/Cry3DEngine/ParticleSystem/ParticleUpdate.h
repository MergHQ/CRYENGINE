// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     25/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEUPDATE_H
#define PARTICLEUPDATE_H

#pragma once

namespace pfx2
{

class CParticleComponentRuntime;
class CParticleContainer;

struct SUpdateContext
{
	explicit SUpdateContext(CParticleComponentRuntime* pRuntime);
	SUpdateContext(CParticleComponentRuntime* pRuntime, const SUpdateRange& updateRange);
	CParticleSystem*           m_pSystem;
	CParticleComponentRuntime& m_runtime;
	CParticleContainer&        m_container;
	CParticleContainer&        m_parentContainer;
	const SComponentParams&    m_params;
	const SUpdateRange         m_updateRange;
	TParticleHeap*             m_pMemHeap;
	const float                m_deltaTime;
	const float                m_time;
	mutable SChaosKey          m_spawnRng;
	mutable SChaosKeyV         m_spawnRngv;
	mutable SChaosKey          m_updateRng;
	mutable SChaosKeyV         m_updateRngv;
};

#define CRY_PFX2_FOR_RANGE_PARTICLES(updateRange) \
  {                                               \
    { for (TParticleId particleId = updateRange.m_firstParticleId; particleId < updateRange.m_lastParticleId; ++particleId) {

#define CRY_PFX2_FOR_RANGE_PARTICLESGROUP(updateRange)                                                                                          \
  {                                                                                                                                             \
    { const TParticleGroupId lastParticleId = CRY_PFX2_PARTICLESGROUP_LOWER(updateRange.m_lastParticleId + CRY_PFX2_PARTICLESGROUP_STRIDE - 1); \
      for (TParticleGroupId particleGroupId = updateRange.m_firstParticleId; particleGroupId < lastParticleId; particleGroupId += CRY_PFX2_PARTICLESGROUP_STRIDE) {

#define CRY_PFX2_FOR_ACTIVE_PARTICLES(updateContext) \
  CRY_PFX2_FOR_RANGE_PARTICLES((updateContext).m_updateRange)

#define CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(updateContext) \
  CRY_PFX2_FOR_RANGE_PARTICLESGROUP((updateContext).m_updateRange)

#define CRY_PFX2_FOR_SPAWNED_PARTICLES(updateContext) \
  CRY_PFX2_FOR_RANGE_PARTICLES(SUpdateRange((updateContext).m_container.GetFirstSpawnParticleId(), (updateContext).m_container.GetLastParticleId()))

#define CRY_PFX2_FOR_SPAWNED_PARTICLEGROUP(updateContext) \
  CRY_PFX2_FOR_RANGE_PARTICLESGROUP(SUpdateRange((updateContext).m_container.GetFirstSpawnParticleId(), (updateContext).m_container.GetLastParticleId()))

#define CRY_PFX2_FOR_END } \
  }                        \
  }

}

#endif // PARTICLEUPDATE_H
