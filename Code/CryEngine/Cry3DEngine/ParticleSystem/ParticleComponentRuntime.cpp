// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     02/04/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleComponentRuntime.h"
#include "ParticleEmitter.h"
#include "ParticleFeature.h"

namespace pfx2
{

CParticleComponentRuntime::CParticleComponentRuntime(CParticleEffect* pEffect, CParticleEmitter* pEmitter, CParticleComponent* pComponent)
	: m_pEffect(pEffect)
	, m_pEmitter(pEmitter)
	, m_pComponent(pComponent)
	, m_bounds(AABB::RESET)
	, m_active(false)
{
}

CParticleContainer& CParticleComponentRuntime::GetParentContainer()
{
	const SComponentParams& params = GetComponentParams();
	if (!params.IsSecondGen())
		return GetEmitter()->GetParentContainer();
	else
		return GetEmitter()->GetRuntimes()[params.m_parentId].pRuntime->GetCpuRuntime()->GetContainer();
}

const CParticleContainer& CParticleComponentRuntime::GetParentContainer() const
{
	const SComponentParams& params = GetComponentParams();
	if (!params.IsSecondGen())
		return GetEmitter()->GetParentContainer();
	else
		return GetEmitter()->GetRuntimes()[params.m_parentId].pRuntime->GetCpuRuntime()->GetContainer();
}

void CParticleComponentRuntime::Initialize()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	CParticleComponent* pcomponent = GetComponent();

	m_container.ResetUsedData();
	for (size_t i = 0; i < EPDT_Count; ++i)
	{
		EParticleDataType type = EParticleDataType(i);
		if (pcomponent->UseParticleData(type))
			m_container.AddParticleData(type);
	}
	m_container.Trim();
}

void CParticleComponentRuntime::Reset()
{
	m_container.Clear();
	m_subInstances.clear();
	m_subInstances.shrink_to_fit();
	m_subInstanceData.clear();
	m_subInstanceData.shrink_to_fit();
}

void CParticleComponentRuntime::SetActive(bool active)
{
	if (active == m_active)
		return;
	m_active = active;
}

void CParticleComponentRuntime::UpdateAll(const SUpdateContext& context)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	AddRemoveNewBornsParticles(context);
	UpdateParticles(context);
	CalculateBounds();
}

void CParticleComponentRuntime::AddRemoveNewBornsParticles(const SUpdateContext& context)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), this, EPS_NewBornTime);

	m_container.RemoveNewBornFlags();
	AddRemoveParticles(context);
	UpdateNewBorns(context);
	m_container.ResetSpawnedParticles();
}

void CParticleComponentRuntime::UpdateParticles(const SUpdateContext& context)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), this, EPS_UpdateTime);

	m_container.FillData<EPDT_AccelerationX>(context, 0.0f);
	m_container.FillData<EPDT_AccelerationY>(context, 0.0f);
	m_container.FillData<EPDT_AccelerationZ>(context, 0.0f);
	m_container.FillData<EPDT_VelocityFieldX>(context, 0.0f);
	m_container.FillData<EPDT_VelocityFieldY>(context, 0.0f);
	m_container.FillData<EPDT_VelocityFieldZ>(context, 0.0f);

	UpdateFeatures(context);
	PostUpdateFeatures(context);
	AgeUpdate(context);
}

void CParticleComponentRuntime::ComputeVertices(const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels)
{
	if (GetComponent()->IsVisible())
	{
		FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
		CTimeProfiler profile(GetPSystem()->GetProfiler(), this, EPS_ComputeVerticesTime);

		const SComponentParams& params = GetComponentParams();
		if (!params.IsValid())
			return;

		for (auto& it : GetComponent()->GetUpdateList(EUL_Render))
			it->ComputeVertices(this, camInfo, pRE, uRenderFlags, fMaxPixels);
	}
}

void CParticleComponentRuntime::AddSubInstances(SInstance* pInstances, size_t count)
{
	CRY_PFX2_PROFILE_DETAIL;

	const SComponentParams& params = GetComponentParams();

	size_t firstInstance = m_subInstances.size();
	size_t lastInstance = firstInstance + count;
	m_subInstances.insert(m_subInstances.end(), pInstances, pInstances + count);
	m_subInstanceData.resize(params.m_instanceDataStride * m_subInstances.size());
	AlignInstances();

	for (auto& it : GetComponent()->GetUpdateList(EUL_InitSubInstance))
		it->InitSubInstance(this, firstInstance, lastInstance);

	DebugStabilityCheck();
}

void CParticleComponentRuntime::RemoveSubInstance(size_t instanceId)
{
	CRY_PFX2_PROFILE_DETAIL;

	const SComponentParams& params = GetComponentParams();
	size_t dataStride = params.m_instanceDataStride;
	size_t toCopy = m_subInstances.size() - 1;
	byte* pBytes = m_subInstanceData.data();
	m_subInstances[instanceId] = m_subInstances[toCopy];
	memcpy(pBytes + dataStride * instanceId, pBytes + dataStride * toCopy, dataStride);
	m_subInstances.pop_back();
	AlignInstances();

	DebugStabilityCheck();
}

void CParticleComponentRuntime::RemoveAllSubInstances()
{
	m_subInstances.clear();
	AlignInstances();
	DebugStabilityCheck();
	m_container.FillData<EPDT_ParentId>(SUpdateContext(this), gInvalidId);
}

void CParticleComponentRuntime::ReparentParticles(const uint* swapIds, const uint numSwapIds)
{
	CRY_PFX2_PROFILE_DETAIL;

	const SComponentParams& params = GetComponentParams();
	IOPidStream parentIds = m_container.GetIOPidStream(EPDT_ParentId);

	for (TParticleId particleId = 0; particleId != m_container.GetLastParticleId(); ++particleId)
	{
		const TParticleId parentId = parentIds.Load(particleId);
		if (parentId != gInvalidId)
		{
			CRY_PFX2_ASSERT(parentId < numSwapIds);   // this particle was pointing to the wrong parentId already
			parentIds.Store(particleId, swapIds[parentId]);
		}
	}

	size_t dataStride = params.m_instanceDataStride;
	byte* pBytes = m_subInstanceData.data();
	size_t toCopy = 0;
	for (size_t i = 0; pBytes && i < m_subInstances.size(); ++i)
	{
		m_subInstances[i].m_parentId = swapIds[m_subInstances[i].m_parentId];
		if (m_subInstances[i].m_parentId == gInvalidId)
			continue;
		m_subInstances[toCopy] = m_subInstances[i];
		memcpy(pBytes + dataStride * toCopy, pBytes + dataStride * i, dataStride);
		++toCopy;
	}
	m_subInstances.erase(m_subInstances.begin() + toCopy, m_subInstances.end());
	AlignInstances();

	DebugStabilityCheck();
}

bool CParticleComponentRuntime::IsValidRuntimeForInitializationParameters(const SRuntimeInitializationParameters& parameters)
{
	return parameters.usesGpuImplementation == false;
}

void CParticleComponentRuntime::MainPreUpdate()
{
	for (auto& it : GetComponent()->GetUpdateList(EUL_MainPreUpdate))
		it->MainPreUpdate(this);
}

void CParticleComponentRuntime::GetSpatialExtents(const SUpdateContext& context, Array<const float, uint> scales, Array<float, uint> extents)
{
	for (auto& it : GetComponent()->GetUpdateList(EUL_GetExtents))
		it->GetSpatialExtents(context, scales, extents);
}

void CParticleComponentRuntime::SpawnParticles(CParticleContainer::SSpawnEntry const& entry)
{
	if (entry.m_count > 0)
		m_spawnEntries.push_back(entry);
}

void CParticleComponentRuntime::AddRemoveParticles(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	CParticleEffect* pEffect = GetEffect();
	CParticleEmitter* pEmitter = GetEmitter();
	const SComponentParams& params = GetComponentParams();
	const bool hasChildren = params.HasChildren();
	const bool hasKillFeatures = !GetComponent()->GetUpdateList(EUL_KillUpdate).empty();
	const size_t maxParticles = m_container.GetMaxParticles();
	const uint32 numParticles = m_container.GetNumParticles();
	TIStream<uint8> states = m_container.GetTIStream<uint8>(EPDT_State);

	//////////////////////////////////////////////////////////////////////////

	if (hasKillFeatures)
	{
		TParticleIdArray particleIds(*context.m_pMemHeap);
		particleIds.reserve(numParticles);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const uint8 state = states.Load(particleId);
			if (state == ES_Expired)
				particleIds.push_back(particleId);
		}
		CRY_PFX2_FOR_END;
		if (!particleIds.empty())
		{
			for (auto& it : GetComponent()->GetUpdateList(EUL_KillUpdate))
				it->KillParticles(context, &particleIds[0], particleIds.size());
		}
	}

	//////////////////////////////////////////////////////////////////////////

	m_spawnEntries.clear();

	for (auto& it : GetComponent()->GetUpdateList(EUL_Spawn))
		it->SpawnParticles(context);

	TParticleIdArray particleIds(*context.m_pMemHeap);
	particleIds.reserve(numParticles);

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		const uint8 state = states.Load(particleId);
		if (state == ES_Dead)
			particleIds.push_back(particleId);
	}
	CRY_PFX2_FOR_END;

	if (!m_spawnEntries.empty() || !particleIds.empty())
	{
		const bool hasSwapIds = (hasChildren && !particleIds.empty());
		TParticleIdArray swapIds(*context.m_pMemHeap);
		swapIds.resize(hasSwapIds ? numParticles : 0);

		m_container.AddRemoveParticles(m_spawnEntries.data(), m_spawnEntries.size(), &particleIds, &swapIds);

		if (hasSwapIds)
		{
			for (auto& subComponentId : params.m_subComponentIds)
			{
				ICommonParticleComponentRuntime* pSubRuntime = pEmitter->GetRuntimes()[subComponentId].pRuntime;
				if (pSubRuntime->IsActive())
					pSubRuntime->ReparentParticles(&swapIds[0], swapIds.size());
			}
		}
	}
}

void CParticleComponentRuntime::UpdateNewBorns(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (!m_container.HasSpawnedParticles())
		return;

	const float deltaTime = context.m_deltaTime;
	const floatv deltaTimev = ToFloatv(deltaTime);
	const SComponentParams& params = GetComponentParams();
	CParticleContainer& parentContainer = GetParentContainer();

	// interpolate position and normAge over time and velocity
	IPidStream parentIds = m_container.GetIPidStream(EPDT_ParentId);
	IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
	IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
	IOVec3Stream positions = m_container.GetIOVec3Stream(EPVF_Position);
	IOFStream normAges = m_container.GetIOFStream(EPDT_NormalAge);
	CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
	{
		const float frameAge = normAges.Load(particleId);
		const TParticleId parentId = parentIds.Load(particleId);
		const Vec3 wParentPos = parentPositions.Load(parentId);
		const Vec3 wParentVel = parentVelocities.Load(parentId);
		const Vec3 wPosition = wParentPos + wParentVel * (frameAge * deltaTime);
		positions.Store(particleId, wPosition);
	}
	CRY_PFX2_FOR_END;

	// update orientation if any
	if (m_container.HasData(EPDT_OrientationX))
	{
		const Quat defaultQuat = GetEmitter()->GetLocation().q;
		IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		IOQuatStream quats = m_container.GetIOQuatStream(EPQF_Orientation);
		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const Quat wParentQuat = parentQuats.SafeLoad(parentId);
			quats.Store(particleId, wParentQuat);
		}
		CRY_PFX2_FOR_END;
	}

	// neutral velocity
	m_container.FillSpawnedData<EPDT_VelocityX>(context, 0.0f);
	m_container.FillSpawnedData<EPDT_VelocityY>(context, 0.0f);
	m_container.FillSpawnedData<EPDT_VelocityZ>(context, 0.0f);

	// initialize unormrand
	if (m_container.HasData(EPDT_UNormRand))
	{
		IOFStream unormRands = m_container.GetIOFStream(EPDT_UNormRand);
		SChaosKey chaos;
		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		{
			const float unormRand = chaos.RandUNorm();
			unormRands.Store(particleId, unormRand);
		}
		CRY_PFX2_FOR_END;
	}

	// feature init particles
	m_container.FillSpawnedData<EPDT_State>(context, uint8(ES_NewBorn));
	for (auto& it : GetComponent()->GetUpdateList(EUL_InitUpdate))
		it->InitParticles(context);

	// calculate inv lifetimes
	IOFStream invLifeTimes = m_container.GetIOFStream(EPDT_InvLifeTime);
	if (params.m_baseParticleLifeTime > 0.0f)
	{
		IFStream lifeTimes = m_container.GetIFStream(EPDT_LifeTime);
		CRY_PFX2_FOR_SPAWNED_PARTICLEGROUP(context)
		{
			const floatv lifetime = lifeTimes.Load(particleGroupId);
			const floatv invLifeTime = rcp_fast(max(lifetime, deltaTimev));
			invLifeTimes.Store(particleGroupId, invLifeTime);
		}
		CRY_PFX2_FOR_END;
	}
	else
	{
		CRY_PFX2_FOR_SPAWNED_PARTICLEGROUP(context)
		invLifeTimes.Store(particleGroupId, ToFloatv(0.0f));
		CRY_PFX2_FOR_END;
	}

	// feature post init particles
	for (auto& it : GetComponent()->GetUpdateList(EUL_PostInitUpdate))
		it->PostInitParticles(context);
}

void CParticleComponentRuntime::UpdateFeatures(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

#define PFX2_FLOAT_FIELD_COPY(NAME) \
  m_container.CopyData<EPDT_ ## NAME, EPDT_ ## NAME ## Init>(context);
	PFX2_FLOAT_FIELDS(PFX2_FLOAT_FIELD_COPY)
	m_container.CopyData<EPDT_Color, EPDT_ColorInit>(context);

	for (auto& it : GetComponent()->GetUpdateList(EUL_Update))
		it->Update(context);
}

void CParticleComponentRuntime::PostUpdateFeatures(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	for (auto& it : GetComponent()->GetUpdateList(EUL_PostUpdate))
		it->PostUpdate(context);
}

void CParticleComponentRuntime::CalculateBounds()
{
	CRY_PFX2_PROFILE_DETAIL;
	CTimeProfiler profile(GetPSystem()->GetProfiler(), this, EPS_UpdateTime);

	IVec3Stream positions = m_container.GetIVec3Stream(EPVF_Position);
	IFStream sizes = m_container.GetIFStream(EPDT_Size);
	const floatv fMin = ToFloatv(std::numeric_limits<float>::max());
	const floatv fMax = ToFloatv(-std::numeric_limits<float>::max());

	Vec3v bbMin = Vec3v(fMin, fMin, fMin);
	Vec3v bbMax = Vec3v(fMax, fMax, fMax);

	// PFX2_TODO : clean up this mess
#ifdef CRY_PFX2_USE_SSE
	// vector part
	const TParticleId lastParticleId = m_container.GetLastParticleId();
	const TParticleGroupId lastParticleGroupId = CRY_PFX2_PARTICLESGROUP_UPPER(lastParticleId) + 1 - CRY_PFX2_PARTICLESGROUP_STRIDE;
	for (TParticleGroupId particleGroupId = 0; particleGroupId < lastParticleGroupId; particleGroupId += CRY_PFX2_PARTICLESGROUP_STRIDE)
	{
		const floatv size = sizes.Load(particleGroupId);
		const Vec3v position = positions.Load(particleGroupId);
		bbMin = min(bbMin, Sub(position, size));
		bbMax = max(bbMax, Add(position, size));
	}
	m_bounds.min = HMin(bbMin);
	m_bounds.max = HMax(bbMax);

	// linear part
	for (TParticleId particleId = lastParticleGroupId.Get(); particleId < lastParticleId; ++particleId)
	{
		const float size = sizes.Load(particleId);
		const Vec3 sizev = Vec3(size, size, size);
		const Vec3 position = positions.Load(particleId);
		m_bounds.min = min(m_bounds.min, position - sizev);
		m_bounds.max = max(m_bounds.max, position + sizev);
	}
#else
	const TParticleId lastParticleId = m_container.GetLastParticleId();
	for (TParticleId particleId = 0; particleId < lastParticleId; ++particleId)
	{
		const float size = sizes.Load(particleId);
		const Vec3 sizev = Vec3(size, size, size);
		const Vec3 position = positions.Load(particleId);
		m_bounds.min = min(m_bounds.min, position - sizev);
		m_bounds.max = max(m_bounds.max, position + sizev);
	}
#endif
}

void CParticleComponentRuntime::WaitUntilUpdateEnded()
{
	CRY_PFX2_PROFILE_DETAIL;
	m_updateState.Wait();
}

void CParticleComponentRuntime::AgeUpdate(const SUpdateContext& context)
{
	CRY_PFX2_PROFILE_DETAIL;

	IFStream invLifeTimes = m_container.GetIFStream(EPDT_InvLifeTime);
	IOFStream normAges = m_container.GetIOFStream(EPDT_NormalAge);
	const floatv frameTime = ToFloatv(context.m_deltaTime);
	const floatv zero = ToFloatv(0.0f);

	CRY_PFX2_FOR_ACTIVE_PARTICLESGROUP(context)
	{
		const floatv invLifeTime = invLifeTimes.Load(particleGroupId);
		const floatv normAge0 = normAges.Load(particleGroupId);
		const floatv dT = DeltaTime(normAge0, frameTime);
		const floatv normalAge1 = MAdd(dT, invLifeTime, max(zero, normAge0));
		normAges.Store(particleGroupId, normalAge1);
	}
	CRY_PFX2_FOR_END;

	TIOStream<uint8> states = m_container.GetTIOStream<uint8>(EPDT_State);
	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		const float normalAge = normAges.Load(particleId);
		uint8 state = states.Load(particleId);
		if (state == ES_Expired)
			state = ES_Dead;
		if (normalAge >= 1.0f && state == ES_Alive)
			state = ES_Expired;
		states.Store(particleId, state);
	}
	CRY_PFX2_FOR_END;
}

void CParticleComponentRuntime::AlignInstances()
{
	CRY_PFX2_PROFILE_DETAIL;

	m_subInstances.reserve(CRY_PFX2_PARTICLESGROUP_UPPER(m_subInstances.size()) + 1);

	memset(
	  m_subInstances.data() + m_subInstances.size(),
	  ~0,
	  (m_subInstances.capacity() - m_subInstances.size()) * sizeof(TParticleId));
}

void CParticleComponentRuntime::DebugStabilityCheck()
{
#if 0
	#ifdef CRY_DEBUG_PARTICLE_SYSTEM
	const CParticleContainer& parentContainer = GetParentContainer();
	const TParticleId parentCount = parentContainer.GetLastParticleId();
	IPidStream parentIds = m_container.GetIPidStream(EPDT_ParentId);

	CRY_PFX2_FOR_SPAWNED_PARTICLES(m_container, m_container.GetFullRange());
	TParticleId parentId = parentIds.Load(particleId);
	CRY_PFX2_ASSERT(parentIds.Load(particleId) != gInvalidId);      // recently spawn particles are not supposed to be orphan
	CRY_PFX2_FOR_END;

	CRY_PFX2_FOR_ACTIVE_PARTICLES(m_container, m_container.GetFullRange());
	{
		TParticleId parentId = parentIds.Load(particleId);
		CRY_PFX2_ASSERT(parentId < parentCount || parentId == gInvalidId);    // this particle is not pointing to the correct parent
	}
	CRY_PFX2_FOR_END;

	for (size_t i = 0; i < m_subInstances.size(); ++i)
	{
		TParticleId parentId = m_subInstances[i].m_parentId;
		CRY_PFX2_ASSERT(parentId < parentCount);    // this instance is not pointing to the correct parent
	}
	#endif
#endif
}

void CParticleComponentRuntime::AccumCounts(SParticleCounts& counts)
{
	counts.EmittersAlloc += 1.0f;
	counts.ParticlesAlloc += m_container.GetMaxParticles();
	counts.EmittersActive += 1.0f;
	counts.SubEmittersActive += 1.0f;
	counts.ParticlesActive += m_container.GetLastParticleId();

	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	profiler.AddEntry(this, EPS_ActiveParticles, uint(m_container.GetLastParticleId()));
	profiler.AddEntry(this, EPS_AllocatedParticles, uint(m_container.GetMaxParticles()));
}

}
