// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     27/10/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleSystem.h"
#include "ParticleComponentRuntime.h"
#include "ParticleUpdate.h"

CRY_PFX2_DBG

namespace pfx2
{

SUpdateContext::SUpdateContext(CParticleComponentRuntime* pRuntime)
	: m_pSystem(GetPSystem())
	, m_runtime(*pRuntime)
	, m_container(pRuntime->GetContainer())
	, m_parentContainer(pRuntime->GetParentContainer())
	, m_params(pRuntime->GetComponentParams())
	, m_updateRange(m_container.GetFullRange())
	, m_deltaTime(gEnv->pTimer->GetFrameTime())
{
	const uint32 threadId = JobManager::GetWorkerThreadId();
	m_pMemHeap = &m_pSystem->GetMemHeap(threadId);
}

SUpdateContext::SUpdateContext(CParticleComponentRuntime* pRuntime, const SUpdateRange& updateRange)
	: m_pSystem(GetPSystem())
	, m_runtime(*pRuntime)
	, m_container(pRuntime->GetContainer())
	, m_parentContainer(pRuntime->GetParentContainer())
	, m_params(pRuntime->GetComponentParams())
	, m_updateRange(updateRange)
	, m_deltaTime(gEnv->pTimer->GetFrameTime())
{
	const uint32 threadId = JobManager::GetWorkerThreadId();
	m_pMemHeap = &m_pSystem->GetMemHeap(threadId);
}

}
