// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleComponentRuntime.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.h"
#include "ParticleProfiler.h"

namespace pfx2
{

MakeDataType(EPVF_PositionPrev, Vec3);

extern TDataType<Vec3> EPVF_Acceleration, EPVF_VelocityField;


CParticleComponentRuntime::CParticleComponentRuntime(CParticleEmitter* pEmitter, CParticleComponent* pComponent)
	: m_pEmitter(pEmitter)
	, m_pComponent(pComponent)
	, m_bounds(AABB::RESET)
	, m_alive(true)
	, m_deltaTime(-1.0f)
	, m_isPreRunning(false)
	, m_chaos(0)
	, m_chaosV(0)
{
	Initialize();
	if (pComponent->UsesGPU())
	{
		m_pGpuRuntime = gEnv->pRenderer->GetGpuParticleManager()->CreateParticleContainer(
			pComponent->GPUComponentParams(),
			pComponent->GetGpuFeatures());
	}
}

CParticleComponentRuntime::~CParticleComponentRuntime()
{
	if (GetComponent()->DestroyParticles.size() && m_container.GetNumParticles())
		GetComponent()->DestroyParticles(*this);
}

bool CParticleComponentRuntime::IsValidForComponent() const
{
	if (!m_pComponent->UsesGPU())
		return !m_pGpuRuntime;
	else
		return m_pGpuRuntime && m_pGpuRuntime->IsValidForParams(m_pComponent->GPUComponentParams());
}

uint CParticleComponentRuntime::GetNumParticles() const
{
	if (m_pGpuRuntime)
	{
		SParticleStats stats;
		m_pGpuRuntime->AccumStats(stats);
		return stats.particles.alive;
	}
	else
	{
		return m_container.GetRealNumParticles();
	}
}

void CParticleComponentRuntime::AddBounds(const AABB& bounds)
{
	m_bounds.Add(bounds);
	m_pEmitter->AddBounds(bounds);
}

CParticleContainer& CParticleComponentRuntime::GetParentContainer()
{
	if (CParticleComponent* pParent = m_pComponent->GetParentComponent())
		if (CParticleComponentRuntime* pParentRuntime = GetEmitter()->GetRuntimeFor(pParent))
			return pParentRuntime->GetContainer();
	return GetEmitter()->GetParentContainer();
}

const CParticleContainer& CParticleComponentRuntime::GetParentContainer() const
{
	return non_const(this)->GetParentContainer();
}

void CParticleComponentRuntime::Initialize()
{
	m_container.SetUsedData(m_pComponent->GetUseData());
	if (m_pComponent->OnPreRun.size())
		PreRun();
}

void CParticleComponentRuntime::Clear()
{
	m_container.Clear();
	RemoveAllSubInstances();
	m_deltaTime = -1.0f;
}

void CParticleComponentRuntime::PreRun()
{
	if (GetGpuRuntime())
		return;

	// Save and reset all particle data
	TSaveRestore<CParticleContainer> saveContainer(m_container, m_pComponent->GetUseData());
	TSaveRestore<TDynArray<SInstance>> saveInstances(m_subInstances);
	TSaveRestore<TDynArray<byte>> saveInstanceData(m_subInstanceData);

	m_deltaTime = ComponentParams().m_stableTime;
	m_isPreRunning = true;

	AddInstances();
	GetComponent()->SpawnParticles(*this);
	InitParticles();

	CalculateBounds();
	m_pEmitter->UpdatePhysEnv();

	UpdateParticles();
	CalculateBounds();
	m_isPreRunning = false;

	m_pComponent->OnPreRun(*this);

	m_deltaTime = -1.0f;
}

void CParticleComponentRuntime::UpdateAll()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	m_alive = false;

	uint32 emitterSeed = m_pEmitter->GetCurrentSeed();
	uint32 componentId = m_pComponent->GetComponentId() << 16;
	m_chaos = SChaosKey(emitterSeed + componentId + m_container.GetNumParticles());
	m_chaosV = SChaosKeyV(m_chaos);

	if (GetGpuRuntime())
		return UpdateGPURuntime();

	AddRemoveParticles();
	if (HasParticles())
	{
		SetAlive();
		UpdateParticles();
		CalculateBounds();
	}
	else
	{
		m_bounds.Reset();
	}
	AccumStats();
}

void CParticleComponentRuntime::AddRemoveParticles()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), *this, EPS_NewBornTime);

	m_container.BeginAddParticles();
	RemoveParticles();
	AgeUpdate();

	AddInstances();
	if (GetNumInstances())
		GetComponent()->SpawnParticles(*this);
	InitParticles();
}

void CParticleComponentRuntime::UpdateParticles()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), *this, EPS_UpdateTime);

	m_container.FillData(EPVF_Acceleration, Vec3(0), FullRange());
	m_container.FillData(EPVF_VelocityField, Vec3(0), FullRange());
	m_container.CopyData(EPVF_PositionPrev, EPVF_Position, FullRange());

	for (EParticleDataType type(0); type < EParticleDataType::size(); type = type + type.info().step())
	{
		if (type.info().domain & EDD_HasUpdate)
			m_container.CopyData(type, InitType(type), FullRange());
	}

	GetComponent()->PreUpdateParticles(*this);
	GetComponent()->UpdateParticles(*this);
	GetComponent()->PostUpdateParticles(*this);
}

void CParticleComponentRuntime::ComputeVertices(const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), *this, EPS_ComputeVerticesTime);

	GetEmitter()->SyncUpdateParticles();

	if (camInfo.pCamera->IsAABBVisible_E(m_bounds))
		GetComponent()->ComputeVertices(*this, camInfo, pRE, uRenderFlags, fMaxPixels);
}

void CParticleComponentRuntime::RemoveAllSubInstances()
{
	m_subInstances.clear();
	m_subInstanceData.clear();

	m_container.FillData(EPDT_ParentId, gInvalidId, FullRange());
	DebugStabilityCheck();
}

void CParticleComponentRuntime::RenderAll(const SRenderContext& renderContext)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (auto* pGPURuntime = GetGpuRuntime())
	{
		SParticleStats stats;
		pGPURuntime->AccumStats(stats);
		auto& statsGPU = GetPSystem()->GetThreadData().statsGPU;
		statsGPU.components.rendered += stats.components.rendered;
		statsGPU.particles.rendered += stats.particles.rendered;
	}

	m_pComponent->Render(*this, renderContext);
}

void CParticleComponentRuntime::ReparentParticles(TConstArray<TParticleId> swapIds)
{
	CRY_PFX2_PROFILE_DETAIL;

	IOPidStream parentIds = m_container.GetIOPidStream(EPDT_ParentId);

	for (auto particleId : FullRange())
	{
		const TParticleId parentId = parentIds.Load(particleId);
		if (parentId != gInvalidId)
		{
			CRY_PFX2_ASSERT(parentId < swapIds.size());   // this particle was pointing to the wrong parentId already
			parentIds.Store(particleId, swapIds[parentId]);
		}
	}

	size_t dataStride = ComponentParams().m_instanceDataStride;
	byte* pBytes = m_subInstanceData.data();
	uint toCopy = 0;
	for (uint i = 0; pBytes && i < m_subInstances.size(); ++i)
	{
		m_subInstances[i].m_parentId = swapIds[m_subInstances[i].m_parentId];
		if (m_subInstances[i].m_parentId == gInvalidId)
			continue;
		m_subInstances[toCopy] = m_subInstances[i];
		memcpy(pBytes + dataStride * toCopy, pBytes + dataStride * i, dataStride);
		++toCopy;
	}
	m_subInstances.erase(m_subInstances.begin() + toCopy, m_subInstances.end());
	m_subInstanceData.resize(ComponentParams().m_instanceDataStride * m_subInstances.size());

	DebugStabilityCheck();
}

void CParticleComponentRuntime::GetEmitLocations(TVarArray<QuatTS> locations, uint firstInstance) const
{
	auto const& parentContainer = GetParentContainer();
	auto parentPositions = parentContainer.GetIVec3Stream(EPVF_Position, GetEmitter()->GetLocation().t);
	auto parentRotations = parentContainer.GetIQuatStream(EPQF_Orientation, GetEmitter()->GetLocation().q);

	THeapArray<Vec3> offsets(MemHeap(), locations.size());
	offsets.fill(Vec3(0));
	GetComponent()->GetEmitOffsets(*this, offsets, firstInstance);

	for (uint idx = 0; idx < locations.size(); ++idx)
	{
		TParticleId parentId = GetInstance(firstInstance + idx).m_parentId;

		QuatTS& loc = locations[idx];
		loc.t = parentPositions.SafeLoad(parentId);
		loc.q = parentRotations.SafeLoad(parentId);
		loc.s = 1.0f;

		loc.t = loc * offsets[idx];
	}
}

void CParticleComponentRuntime::EmitParticle()
{
	SSpawnEntry spawn = {1, GetParentContainer().GetNumParticles()};
	AddParticles({&spawn, 1});
	m_container.ResetSpawnedParticles();
}

void CParticleComponentRuntime::AddInstances()
{
	CRY_PFX2_PROFILE_DETAIL;

	TDynArray<SInstance> instances;
	GetComponent()->AddSubInstances(*this, instances);
	GetComponent()->CullSubInstances(*this, instances);

	if (instances.empty())
		return;

	uint firstInstance = m_subInstances.size();
	uint lastInstance = firstInstance + instances.size();
	m_subInstances.append(instances);
	m_subInstanceData.resize(ComponentParams().m_instanceDataStride * m_subInstances.size());

	SUpdateRange instanceRange(firstInstance, lastInstance);
	GetComponent()->InitSubInstances(*this, instanceRange);

	DebugStabilityCheck();
}

void CParticleComponentRuntime::AddParticles(TConstArray<SSpawnEntry> spawnEntries)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (m_pGpuRuntime)
	{
		gpu_pfx2::SUpdateParams params;

		params.emitterPosition    = m_pEmitter->GetLocation().t;
		params.emitterOrientation = m_pEmitter->GetLocation().q;
		params.physAccel          = m_pEmitter->GetPhysicsEnv().m_UniformForces.vAccel;
		params.physWind           = m_pEmitter->GetPhysicsEnv().m_UniformForces.vWind;

		GetComponent()->UpdateGPUParams(*this, params);

		// Get data of parent particles
		const auto& parentContainer = GetParentContainer();
		IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);

		THeapArray<SParentData> parentData(MemHeap(), parentContainer.GetNumParticles());

		for (auto parentId : parentContainer.GetFullRange())
		{
			parentData[parentId].position = parentPositions.Load(parentId);
			parentData[parentId].velocity = parentVelocities.Load(parentId);
		}

		m_pGpuRuntime->UpdateData(params, spawnEntries, parentData);
		return;
	}

	const uint MaxSpawn = m_isPreRunning ? 16 : ~0;
	uint newCount = 0;
	for (const auto& spawnEntry : spawnEntries)
		newCount += min(spawnEntry.m_count, MaxSpawn);
	if (newCount == 0)
		return;

	uint32 spawnId = m_container.GetNextSpawnId();

	m_container.AddParticles(newCount);

	auto parentIds = m_container.IOStream(EPDT_ParentId);
	auto spawnIds = m_container.IOStream(EPDT_SpawnId);
	auto normAges = m_container.IOStream(EPDT_NormalAge);
	auto fractions = m_container.IOStream(EPDT_SpawnFraction);

	SUpdateRange range = m_container.GetSpawnedRange();
	for (const auto& spawnEntry : spawnEntries)
	{
		uint entryCount = min(spawnEntry.m_count, MaxSpawn);
		range.m_end = CRY_PFX2_GROUP_ALIGN(range.m_begin + entryCount);

		if (parentIds.IsValid())
		{
			for (auto id : range)
				parentIds[id] = spawnEntry.m_parentId;
		}
		if (spawnIds.IsValid())
		{
			for (auto id : range)
				spawnIds[id] = spawnId++;
		}
		if (normAges.IsValid())
		{
			float age = spawnEntry.m_ageBegin;
			for (auto id : range)
			{
				normAges[id] = max(age, 0.0f);
				age += spawnEntry.m_ageIncrement;
			}
		}
		if (fractions.IsValid())
		{
			float fraction = spawnEntry.m_fractionBegin;
			for (auto id : range)
			{
				fractions[id] = min(fraction, 1.0f);
				fraction += spawnEntry.m_fractionIncrement;
			}
		}

		range.m_begin += entryCount;
	}
}

void CParticleComponentRuntime::RemoveParticles()
{
	CRY_PFX2_PROFILE_DETAIL;

	const uint32 numParticles = m_container.GetNumParticles();
	IOFStream normAges = m_container.GetIOFStream(EPDT_NormalAge);

	TParticleIdArray removeIds(MemHeap());
	removeIds.reserve(numParticles);

	GetComponent()->KillParticles(*this);

	for (auto particleId : FullRange())
	{
		const float normalAge = normAges.Load(particleId);
		if (IsExpired(normalAge))
			removeIds.push_back(particleId);
	}

	if (!removeIds.empty())
	{
		TParticleIdArray swapIds(MemHeap());
		const bool hasChildren = !m_pComponent->GetChildComponents().empty();
		if (hasChildren)
			swapIds.resize(numParticles);

		m_container.RemoveParticles(removeIds, swapIds);

		if (hasChildren)
		{
			for (const auto& pChild : m_pComponent->GetChildComponents())
			{
				if (auto pSubRuntime = m_pEmitter->GetRuntimeFor(pChild))
					pSubRuntime->ReparentParticles(swapIds);
			}
		}
	}
}

extern TDataType<Vec3> EPVF_ParentPosition;
extern TDataType<Quat> EPQF_ParentOrientation;

void CParticleComponentRuntime::InitParticles()
{
	CRY_PFX2_PROFILE_DETAIL;

	if (!m_container.HasNewBorns())
		return;

	GetComponent()->PreInitParticles(*this);

	if (!ComponentParams().m_positionsPreInit)
	{
		CParticleContainer& parentContainer = GetParentContainer();

		// interpolate position and normAge over time and velocity
		const IPidStream parentIds = m_container.GetIPidStream(EPDT_ParentId);
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position);
		const IVec3Stream parentVelocities = parentContainer.GetIVec3Stream(EPVF_Velocity);
		const IQuatStream parentOrientations = parentContainer.GetIQuatStream(EPQF_Orientation, GetEmitter()->GetLocation().q);
		const IVec3Stream parentAngularVelocities = parentContainer.GetIVec3Stream(EPVF_AngularVelocity);
		const IFStream parentNormAges = parentContainer.GetIFStream(EPDT_NormalAge);
		const IFStream parentLifeTimes = parentContainer.GetIFStream(EPDT_LifeTime);

		IOVec3Stream parentPrevPositions = m_container.IOStream(EPVF_ParentPosition);
		IOQuatStream parentPrevOrientations = m_container.IOStream(EPQF_ParentOrientation);

		IOVec3Stream positions = m_container.GetIOVec3Stream(EPVF_Position);
		IOQuatStream orientations = m_container.GetIOQuatStream(EPQF_Orientation);

		IOFStream normAges = m_container.GetIOFStream(EPDT_NormalAge);
		IFStream lifeTimes = m_container.GetIFStream(EPDT_LifeTime);

		const bool checkParentLife = IsChild();

		for (auto particleGroupId : SpawnedRangeV())
		{
			// Convert absolute spawned particle age to normal age / life
			floatv backTime = -normAges.Load(particleGroupId) * lifeTimes.Load(particleGroupId);

			// Set initial position and orientation from parent
			const uint32v parentGroupId = parentIds.Load(particleGroupId);
			if (checkParentLife)
			{
				const floatv parentNormAge = parentNormAges.SafeLoad(parentGroupId);
				const floatv parentLifeTime = parentLifeTimes.SafeLoad(parentGroupId);
				const floatv parentOverAge = max(parentNormAge * parentLifeTime - parentLifeTime, convert<floatv>());
				backTime = min(backTime + parentOverAge, convert<floatv>());
			}

			const Vec3v wParentPos = parentPositions.SafeLoad(parentGroupId);
			const Vec3v wParentVel = parentVelocities.SafeLoad(parentGroupId);
			const Vec3v wPosition = MAdd(wParentVel, backTime, wParentPos);
			positions.Store(particleGroupId, wPosition);

			if (m_container.HasData(EPVF_ParentPosition))
				parentPrevPositions.Store(particleGroupId, wPosition);

			if (m_container.HasData(EPQF_Orientation) || m_container.HasData(EPQF_ParentOrientation))
			{
				Quatv wParentQuat = parentOrientations.SafeLoad(parentGroupId);
				if (m_container.HasData(EPVF_AngularVelocity))
				{
					const Vec3v parentAngularVelocity = parentAngularVelocities.SafeLoad(parentGroupId);
					wParentQuat = AddAngularVelocity(wParentQuat, parentAngularVelocity, backTime);
				}
				if (m_container.HasData(EPQF_Orientation))
					orientations.Store(particleGroupId, wParentQuat);

				if (m_container.HasData(EPQF_ParentOrientation))
					parentPrevOrientations.Store(particleGroupId, wParentQuat);
			}
		}

		// neutral velocity
		m_container.FillData(EPVF_Velocity, Vec3(0), SpawnedRange());
	}

	// initialize random
	if (m_container.HasData(EPDT_Random))
	{
		IOFStream unormRands = m_container.GetIOFStream(EPDT_Random);
		for (auto particleGroupId : SpawnedRangeV())
		{
			const floatv unormRand = ChaosV().RandUNorm();
			unormRands.Store(particleGroupId, unormRand);
		}
	}

	// feature init particles
	GetComponent()->InitParticles(*this);

	// modify with spawn params
	const SpawnParams& spawnParams = GetEmitter()->GetSpawnParams();
	if (spawnParams.fSizeScale != 1.0f && m_container.HasData(EPDT_Size))
	{
		const floatv scalev = ToFloatv(spawnParams.fSizeScale);
		IOFStream sizes = m_container.GetIOFStream(EPDT_Size);
		for (auto particleGroupId : SpawnedRangeV())
		{
			const floatv size0 = sizes.Load(particleGroupId);
			const floatv size1 = size0 * scalev;
			sizes.Store(particleGroupId, size1);
		}
		m_container.CopyData(EPDT_Size.InitType(), EPDT_Size, SpawnedRange());
	}
	if (spawnParams.fSpeedScale != 1.0f && m_container.HasData(EPVF_Velocity))
	{
		const floatv scalev = ToFloatv(spawnParams.fSpeedScale);
		IOVec3Stream velocities = m_container.GetIOVec3Stream(EPVF_Velocity);
		for (auto particleGroupId : SpawnedRangeV())
		{
			const Vec3v velocity0 = velocities.Load(particleGroupId);
			const Vec3v velocity1 = velocity0 * scalev;
			velocities.Store(particleGroupId, velocity1);
		}
	}

	// feature post init particles
	GetComponent()->PostInitParticles(*this);

	if (!m_isPreRunning)
		m_deltaTime = max(GetEmitter()->GetDeltaTime(), ComponentParams().m_maxParticleLife);
	if (ComponentParams().m_isPreAged)
		GetComponent()->PastUpdateParticles(*this);
	if (!m_isPreRunning)
		m_deltaTime = -1.0f;

	m_container.ResetSpawnedParticles();
}

void CParticleComponentRuntime::CalculateBounds()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CTimeProfiler profile(GetPSystem()->GetProfiler(), *this, EPS_UpdateTime);

	IVec3Stream positions = m_container.GetIVec3Stream(EPVF_Position);
	IFStream sizes = m_container.GetIFStream(EPDT_Size);
	const float sizeScale = ComponentParams().m_scaleParticleSize;

	SUpdateRange range = m_container.GetFullRange();

#ifdef CRY_PFX2_USE_SSE
	const floatv fMin = ToFloatv(std::numeric_limits<float>::max());
	const floatv fMax = ToFloatv(-std::numeric_limits<float>::max());
	Vec3v bbMin = Vec3v(fMin, fMin, fMin);
	Vec3v bbMax = Vec3v(fMax, fMax, fMax);

	// vector part
	const floatv scalev = convert<floatv>(sizeScale);
	const TParticleId lastParticleId = m_container.GetNumParticles();
	const TParticleGroupId lastParticleGroupId { lastParticleId & ~(CRY_PFX2_GROUP_STRIDE - 1) };
	for (auto particleGroupId : SGroupRange(TParticleGroupId(0), lastParticleGroupId))
	{
		const floatv size = sizes.Load(particleGroupId) * scalev;
		const Vec3v position = positions.Load(particleGroupId);
		bbMin = min(bbMin, Sub(position, size));
		bbMax = max(bbMax, Add(position, size));
	}
	m_bounds.min = HMin(bbMin);
	m_bounds.max = HMax(bbMax);

	range = SUpdateRange(+lastParticleGroupId, lastParticleId);
#else
	m_bounds.Reset();
#endif

	// linear part
	for (auto particleId : range)
	{
		const float size = sizes.Load(particleId) * sizeScale;
		const Vec3 position = positions.Load(particleId);
		m_bounds.Add(position, size);
	}

	CRY_PFX2_ASSERT(m_bounds.GetRadius() < 1000000.f);
}

void CParticleComponentRuntime::AgeUpdate()
{
	CRY_PFX2_PROFILE_DETAIL;

	IFStream invLifeTimes = m_container.GetIFStream(EPDT_InvLifeTime);
	IOFStream normAges = m_container.GetIOFStream(EPDT_NormalAge);
	const floatv frameTime = ToFloatv(DeltaTime());

	for (auto particleGroupId : FullRangeV())
	{
		const floatv invLifeTime = invLifeTimes.Load(particleGroupId);
		const floatv normAge0 = normAges.Load(particleGroupId);
		const floatv normAge1 = normAge0 + frameTime * invLifeTime;
		normAges.Store(particleGroupId, normAge1);
	}
}

void CParticleComponentRuntime::UpdateGPURuntime()
{
	if (!m_pGpuRuntime)
		return;

	AddInstances();
	GetComponent()->SpawnParticles(*this);

	// Accum stats
	SParticleStats stats;
	m_pGpuRuntime->AccumStats(stats);
	stats.particles.rendered = stats.components.rendered = 0;
	GetPSystem()->GetThreadData().statsGPU += stats;

	if (stats.particles.alive)
		SetAlive();
}

void CParticleComponentRuntime::DebugStabilityCheck()
{
#ifdef CRY_PFX2_DEBUG
	const CParticleContainer& parentContainer = GetParentContainer();
	const TParticleId parentCount = parentContainer.GetRealNumParticles();
	IPidStream parentIds = m_container.GetIPidStream(EPDT_ParentId);

	if (m_container.HasNewBorns())
	{
		for (auto particleId : SpawnedRange())
		{
			parentIds.Load(particleId);
			CRY_PFX2_ASSERT(parentIds.Load(particleId) != gInvalidId);      // recently spawn particles are not supposed to be orphan
		}
	}

	for (auto particleId : FullRange())
	{
		TParticleId parentId = parentIds.Load(particleId);
		CRY_PFX2_ASSERT(parentId < parentCount || parentId == gInvalidId);    // this particle is not pointing to the correct parent
	}

	for (auto instance : m_subInstances)
	{
		CRY_PFX2_ASSERT(instance.m_parentId < parentCount);    // this instance is not pointing to the correct parent
	}
#endif
}

bool CParticleComponentRuntime::HasParticles() const
{
	return m_pGpuRuntime ? m_pGpuRuntime->HasParticles() : m_container.GetRealNumParticles() != 0; 
}

void CParticleComponentRuntime::AccumStats()
{
	const uint allocParticles = m_container.GetMaxParticles();
	const uint aliveParticles = m_container.GetNumParticles();

	auto& stats = GetPSystem()->GetThreadData().statsCPU;
	stats.components.alive += IsAlive();
	stats.components.updated ++;
	stats.particles.alloc += allocParticles;
	stats.particles.alive += aliveParticles;
	stats.particles.updated += aliveParticles;

	CParticleProfiler& profiler = GetPSystem()->GetProfiler();
	profiler.AddEntry(*this, EPS_ActiveParticles, aliveParticles);
	profiler.AddEntry(*this, EPS_AllocatedParticles, allocParticles);
}


pfx2::TParticleHeap& CParticleComponentRuntime::MemHeap()
{
	return GetPSystem()->GetThreadData().memHeap;
}

float CParticleComponentRuntime::DeltaTime() const
{
	return m_deltaTime >= 0.0f ? m_deltaTime : m_pEmitter->GetDeltaTime();
}

}
