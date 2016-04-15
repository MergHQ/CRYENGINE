// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     13/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEKERNEL_H
#define PARTICLEKERNEL_H

#pragma once

#include <CryThreading/IJobManager_JobDelegator.h>
#include "ParticleContainer.h"

class CRenderObject;
namespace JobManager {
namespace Detail {
class SGenericJobTKernelPostUpdateParticlesJob;
}
}

namespace pfx2
{

class CParticleComponent;
class CParticleComponentRuntime;
class CParticleEffect;
struct SRenderContext;

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
		JobManager::Detail::SGenericJobTKernelPostUpdateParticlesJob* m_pPostSubUpdates;
		size_t                     m_firstChild;
		size_t                     m_numChildren;
	};

public:
	void AddEmitter(CParticleEmitter* pEmitter);
	void ScheduleComputeVertices(CParticleComponentRuntime* pComponentRuntime, CRenderObject* pRenderObject, const SRenderContext& renderContext);
	void KernelUpdateAll();
	void SynchronizeUpdate();

	// job entry points
	void Job_AddRemoveParticles(uint componentRefIdx);
	void Job_UpdateParticles(uint componentRefIdx, SUpdateRange updateRange);
	void Job_PostUpdateParticles(uint componentRefIdx);
	void Job_CalculateBounds(uint componentRefIdx);
	// ~job entry points

private:
	void AddComponentRecursive(CParticleEmitter* pEmitter, size_t parentRefIdx);

	void ScheduleUpdateParticles(uint componentRefIdx);
	void ScheduleChildrenComponents(SComponentRef& componentRef);
	void ScheduleCalculateBounds(uint componentRefIdx);

	void DoAddRemove(const SComponentRef& componentRef);
	void DoCalculateBounds(const SComponentRef& componentRef);

	void ClearAll();

	std::vector<SComponentRef> m_componentRefs;
	std::vector<size_t>        m_firstGenComponentsRef;
	JobManager::SJobState      m_updateState;
};

}

#endif // PARTICLEKERNEL_H
