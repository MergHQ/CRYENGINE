// Copyright 2015-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     13/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CryThreading/IJobManager_JobDelegator.h>
#include "ParticleEmitter.h"

class CRenderObject;
namespace JobManager {
namespace Detail {
class SGenericJobTPostUpdateParticlesJob;
}
}

namespace pfx2
{

class CParticleComponent;
class CParticleComponentRuntime;
class CParticleEffect;
struct SRenderContext;

// Schedules particle update and render jobs
class CParticleJobManager
{
public:
	struct SComponentRef
	{
		SComponentRef()
			: m_pComponentRuntime(0)
			, m_pPostSubUpdates(0)
			, m_firstChild(0)
			, m_numChildren(0) {}
		SComponentRef(CParticleComponentRuntime* pComponentRuntime)
			: m_pComponentRuntime(pComponentRuntime)
			, m_pPostSubUpdates(0)
			, m_firstChild(0)
			, m_numChildren(0) {}

		CParticleComponentRuntime* m_pComponentRuntime;
		JobManager::SJobState      m_subUpdateState;
		JobManager::Detail::SGenericJobTPostUpdateParticlesJob* m_pPostSubUpdates;
		size_t                     m_firstChild;
		size_t                     m_numChildren;
	};

	struct SDeferredRender
	{
		SDeferredRender(CParticleComponentRuntime* pRuntime, const SRenderContext& renderContext)
			: m_pRuntime(pRuntime)
			, m_rParam(renderContext.m_renderParams)
			, m_passInfo(renderContext.m_passInfo)
			, m_distance(renderContext.m_distance)
			, m_lightVolumeId(renderContext.m_lightVolumeId)
			, m_fogVolumeId(renderContext.m_fogVolumeId) {}
		CParticleComponentRuntime* m_pRuntime;
		SRendParams                m_rParam;
		SRenderingPassInfo         m_passInfo;
		float                      m_distance;
		uint16                     m_lightVolumeId;
		uint16                     m_fogVolumeId;
	};

public:
	void AddEmitter(CParticleEmitter* pEmitter) { m_emitterRefs.push_back(pEmitter); }
	void AddDeferredRender(CParticleComponentRuntime* pRuntime, const SRenderContext& renderContext);
	void ScheduleComputeVertices(CParticleComponentRuntime* pComponentRuntime, CRenderObject* pRenderObject, const SRenderContext& renderContext);
	void ScheduleUpdates();
	void SynchronizeUpdates();
	void DeferredRender();

	// job entry points
	void Job_ScheduleUpdates(TVarArray<CParticleEmitter*> emitters);
	void Job_UpdateEmitters(TVarArray<CParticleEmitter*> emitters);
	// ~job entry points

private:
	void ClearAll();

	TDynArray<CParticleEmitter*> m_emitterRefs;
	TDynArray<SDeferredRender>   m_deferredRenders;
	JobManager::SJobState        m_updateState;
};

}

