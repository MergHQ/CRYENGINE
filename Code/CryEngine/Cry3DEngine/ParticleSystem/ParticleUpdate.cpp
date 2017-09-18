// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     27/10/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem.h"
#include "ParticleUpdate.h"
#include "ParticleComponentRuntime.h"
#include "ParticleEmitter.h"
#include "ParticleEmitter.h"

CRY_PFX2_DBG

namespace pfx2
{

SUpdateContext::SUpdateContext(CParticleComponentRuntime* pRuntime)
	: SUpdateContext(pRuntime, pRuntime->GetContainer().GetFullRange())
{
}

SUpdateContext::SUpdateContext(CParticleComponentRuntime* pRuntime, const SUpdateRange& updateRange)
	: m_pSystem(GetPSystem())
	, m_runtime(*pRuntime)
	, m_container(pRuntime->GetContainer())
	, m_parentContainer(pRuntime->GetParentContainer())
	, m_params(pRuntime->GetComponentParams())
	, m_updateRange(updateRange)
	, m_deltaTime(pRuntime->GetEmitter()->GetDeltaTime())
	, m_time(pRuntime->GetEmitter()->GetTime())
	, m_spawnRng(pRuntime->MakeSeed(GetSpawnedRange().m_begin))
	, m_spawnRngv(m_spawnRng)
	, m_updateRng(pRuntime->MakeSeed(GetUpdateRange().m_begin))
	, m_updateRngv(m_updateRng)
{
	const uint32 threadId = JobManager::GetWorkerThreadId();
	m_pMemHeap = &m_pSystem->GetMemHeap(threadId);
}

}
