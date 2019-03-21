// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "ParticleSystem.h"

#include "3dEngine.h"
#include "FogVolumeRenderNode.h"
#include "LightVolumeManager.h"
#include "Material.h"
#include "ObjMan.h"

#include <CryRenderer/IRenderAuxGeom.h>

namespace pfx2
{

//////////////////////////////////////////////////////////////////////////
// CParticleEmitter


// Common data for top-level emitters
struct SEmitterData : SUseData
{
	SEmitterData()
	{
		AddData(EPVF_Position);
		AddData(EPQF_Orientation);
		AddData(EPVF_Velocity);
		AddData(EPVF_AngularVelocity);
		AddData(EPDT_NormalAge);
	}
};

CParticleEmitter::CParticleEmitter(CParticleEffect* pEffect, uint emitterId)
	: m_pEffect(pEffect)
	, m_pEffectOriginal(pEffect)
	, m_registered(false)
	, m_boundsChanged(false)
	, m_realBounds(AABB::RESET)
	, m_nextBounds(AABB::RESET)
	, m_bounds(AABB::RESET)
	, m_viewDistRatio(1.0f)
	, m_active(false)
	, m_alive(true)
	, m_location(IDENTITY)
	, m_emitterEditVersion(-1)
	, m_effectEditVersion(-1)
	, m_entityOwner(nullptr)
	, m_entitySlot(-1)
	, m_emitterGeometrySlot(-1)
	, m_time(0.0f)
	, m_timeCreated(0.0f)
	, m_timeUpdated(0.0f)
	, m_timeStable(0.0f)
	, m_initialSeed(0)
	, m_emitterId(emitterId)
	, m_unrendered(0)
{
	m_currentSeed = m_initialSeed;
	m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
	m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;

	auto smoothstep = [](float x) { return x*x*(3 - 2 * x); };
	auto contrast = [](float x) { return x * 0.75f + 0.25f; };
	float x = smoothstep(cry_random(0.0f, 1.0f));
	float r = contrast(smoothstep(max(0.0f, -x * 2 + 1)));
	float g = contrast(smoothstep(1 - abs((x - 0.5f) * 2)));
	float b = contrast(smoothstep(max(0.0f, x * 2 - 1)));
	m_profilerColor = ColorF(r, g, b);

	if (m_pEffect)
		m_attributeInstance.Reset(m_pEffect->GetAttributeTable());

	static PUseData s_emitterData = std::make_shared<SEmitterData>();
	m_parentContainer.SetUsedData(s_emitterData, EDD_Particle);
	m_parentContainer.AddElement();

	m_debug = DebugVar();
}

CParticleEmitter::~CParticleEmitter()
{
	Unregister();
	gEnv->p3DEngine->FreeRenderNodeState(this);
	ResetRenderObjects();
}

const char* CParticleEmitter::GetEntityClassName() const
{
	return "ParticleEmitter";
}

const char* CParticleEmitter::GetName() const
{
	return "";
}

void CParticleEmitter::SetMatrix(const Matrix34& mat)
{
	if (mat.IsValid())
		SetLocation(QuatTS(mat));
}

Vec3 CParticleEmitter::GetPos(bool bWorldOnly) const
{
	return m_location.t;
}

void CParticleEmitter::Render(const struct SRendParams& rParam, const SRenderingPassInfo& passInfo)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	PARTICLE_LIGHT_PROFILER();

	DBG_LOCK_TO_THREAD(this);

	if (!passInfo.RenderParticles() || passInfo.IsRecursivePass())
		return;
	if (passInfo.IsShadowPass())
		return;

	CParticleJobManager& jobManager = GetPSystem()->GetJobManager();
	if (ThreadMode() >= 4 && !WasRenderedLastFrame())
	{
		// Not currently scheduled for high priority update
		jobManager.ScheduleUpdateEmitter(this);
	}

	// TODO: make it threadsafe and add it to e_ExecuteRenderAsJobMask
	CLightVolumesMgr& lightVolumeManager = m_p3DEngine->GetLightVolumeManager();
	SRenderContext renderContext(rParam, passInfo);
	renderContext.m_lightVolumeId = lightVolumeManager.RegisterVolume(GetPos(), GetBBox().GetRadius() * 0.5f, rParam.nClipVolumeStencilRef, passInfo);
	renderContext.m_distance = GetPos().GetDistance(passInfo.GetCamera().GetPosition());
	ColorF fogVolumeContrib;
	CFogVolumeRenderNode::TraceFogVolumes(GetPos(), fogVolumeContrib, passInfo);
	renderContext.m_fogVolumeId = passInfo.GetIRenderView()->PushFogVolumeContribution(fogVolumeContrib, passInfo);

	if (m_pEffect->RenderDeferred.size())
	{
		jobManager.AddDeferredRender(this, renderContext);
	}
	
	for (auto& pRuntime : m_componentRuntimes)
	{
		if (pRuntime->GetComponent()->IsVisible())
			pRuntime->RenderAll(renderContext);
	}
	
	m_unrendered = 0;
	GetPSystem()->GetThreadData().statsCPU.emitters.rendered ++;
}

void CParticleEmitter::CheckUpdated()
{
	CRY_PFX2_PROFILE_DETAIL;
	if (m_effectEditVersion != m_pEffectOriginal->GetEditVersion() + m_emitterEditVersion)
	{
		m_attributeInstance.Reset(m_pEffectOriginal->GetAttributeTable());
		UpdateRuntimes();
	}
}

bool CParticleEmitter::UpdateState()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (m_time > m_timeDeath && m_timeUpdated < m_time)
		m_alive = false;

	if (ThreadMode() >= 3 && IsStable())
		// Update only for last frame, even if update skipped for longer
		m_timeUpdated = m_time;

	const float frameTime = gEnv->pTimer->GetFrameTime() * GetTimeScale();
	m_time += frameTime;
	++m_currentSeed;
	m_unrendered++;

	if (!m_alive)
	{
		if (!m_componentRuntimesFor.empty())
			Clear();
		return false;
	}

	if (m_attributeInstance.WasChanged())
		SetChanged();

	if (m_componentRuntimesFor.empty())
	{
		if (!m_active)
			return false;
		UpdateRuntimes();
	}

	UpdateFromEntity();

	for (auto const& pComponent: GetCEffect()->MainPreUpdate)
	{
		if (auto pRuntime = GetRuntimeFor(pComponent))
		{
			pComponent->MainPreUpdate(*pRuntime);
		}
	}

	if (m_boundsChanged)
	{
		m_boundsChanged = false;
		m_bounds = m_nextBounds;
		if (!HasBounds())
			Unregister();
		else
			Register();
	}

	if (HasBounds())
		UpdatePhysEnv();

	return true;
}

namespace Bounds
{
	float Expansion       = 1.125f;
	float ShrinkThreshold = 0.125f;
}

void CParticleEmitter::UpdateBounds()
{
	CRY_PFX2_PROFILE_DETAIL;

	const bool allowShrink = ThreadMode() < 3 || !IsStable();
	if (!m_registered || (allowShrink && m_realBounds.GetVolume() <= m_bounds.GetVolume() * Bounds::ShrinkThreshold))
	{
		m_boundsChanged = true;
		m_nextBounds = m_realBounds;
	}
	else if (!m_bounds.ContainsBox(m_realBounds))
	{
		m_boundsChanged = true;
		m_nextBounds.Add(m_realBounds);
	}

	if (m_boundsChanged)
	{
		if (!m_nextBounds.IsReset())
		{
			// Expand bounds to avoid frequent re-registering
			Vec3 center = m_nextBounds.GetCenter();
			Vec3 extent = m_nextBounds.GetSize() * (Bounds::Expansion * 0.5f);
			m_nextBounds = AABB(center - extent, center + extent);
		}
	}
	m_realBounds.Reset();
}

void CParticleEmitter::AddBounds(const AABB& bb)
{
	m_realBounds.Add(bb);
}

bool CParticleEmitter::UpdateParticles()
{
	// Multithread support
	stl::AutoLock<stl::PSyncMultiThread> lock(m_lock);
	if (m_timeUpdated >= m_time)
		return false;

	// Update all components, and accumulate bounds and stats
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (m_spawnParams.bPrime && !DebugMode('p'))
	{
		if (m_timeUpdated == m_timeCreated)
			m_time = m_timeStable;
	}

	auto& stats = GetPSystem()->GetThreadData().statsCPU;
	stats.emitters.updated ++;

	m_alive = false;
	for (auto& pRuntime : m_componentRuntimes)
	{
		pRuntime->UpdateAll();
		m_realBounds.Add(pRuntime->GetBounds());
		m_alive |= pRuntime->IsAlive();
	}

	PostUpdate();

	UpdateBounds();
	m_timeUpdated = m_time;
	return true;
}

void CParticleEmitter::SyncUpdateParticles()
{
	if (ThreadMode() >= 1)
	{
		auto& statsSync = GetPSystem()->GetThreadData().statsSync;
		statsSync.alive += m_componentRuntimes.size();
		statsSync.rendered += m_pEffect->RenderDeferred.size();
		if (UpdateParticles())
			statsSync.updated += m_componentRuntimes.size();
	}
	CRY_ASSERT(m_timeUpdated >= m_time);
}

void CParticleEmitter::RenderDeferred(const SRenderContext& renderContext)
{
	SyncUpdateParticles();

	for (auto const& pComponent: m_pEffect->RenderDeferred)
	{
		if (auto pRuntime = GetRuntimeFor(pComponent))
		{
			pComponent->RenderDeferred(*pRuntime, renderContext);
		}
	}
}

void CParticleEmitter::DebugRender(const SRenderingPassInfo& passInfo) const
{
	if (!GetRenderer())
		return;
	if (!DebugMode('b'))
		return;
	if (!IsAlive() && !HasParticles())
		return;

	IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

	const bool visible = WasRenderedLastFrame();
	const bool stable = IsStable();

	const float maxDist = non_const(*this).GetMaxViewDist();
	const float dist = (GetPos() - passInfo.GetCamera().GetPosition()).GetLength();
	const float alpha = sqr(crymath::saturate(1.0f - dist / maxDist));
	const float ac = alpha * 0.75f + 0.25f;
	const ColorF alphaColor(ac, ac, ac, alpha);

	if (visible)
	{
		// Draw component boxes and labels
		uint emitterParticles = 0;
		for (auto& pRuntime : m_componentRuntimes)
		{
			if (!pRuntime->GetBounds().IsReset())
			{
				uint numParticles = pRuntime->GetNumParticles();
				emitterParticles += numParticles;
				const float volumeRatio = div_min(pRuntime->GetBounds().GetVolume(), m_realBounds.GetVolume(), 1.0f);
				const ColorB componentColor = ColorF(1, 0.5, 0) * (alphaColor * sqrt(sqrt(volumeRatio)));
				pRenderAux->DrawAABB(pRuntime->GetBounds(), false, componentColor, eBBD_Faceted);
				string label = string().Format("%s #%d", pRuntime->GetComponent()->GetName(), numParticles);
				IRenderAuxText::DrawLabelEx(pRuntime->GetBounds().GetCenter(), 1.5f, componentColor, true, true, label);
			}
		}
		string label = string().Format("%s #%d Age %.3f", m_pEffect->GetShortName().c_str(), emitterParticles, GetAge());
		IRenderAuxText::DrawLabelEx(m_location.t, 1.5f, (float*)&alphaColor, true, true, label);
	}
	if (!m_bounds.IsReset())
	{
		const ColorB emitterColor = ColorF(1, stable, visible) * alphaColor;
		pRenderAux->DrawAABB(m_bounds, false, emitterColor, eBBD_Faceted);
	}
}

void CParticleEmitter::PostUpdate()
{
	m_parentContainer.GetIOVec3Stream(EPVF_Velocity).Store(TParticleGroupId(0), Vec3v(ZERO));
	m_parentContainer.GetIOVec3Stream(EPVF_AngularVelocity).Store(TParticleGroupId(0), Vec3v(ZERO));
}

float CParticleEmitter::GetMaxViewDist() const
{
	IRenderer* pRenderer = GetRenderer();
	const float angularDensity =
		(pRenderer ? GetPSystem()->GetMaxAngularDensity(GetISystem()->GetViewCamera()) : 1080.0f)
		* m_viewDistRatio;

	float maxViewDist = 0.0f;
	for (auto& pRuntime : m_componentRuntimes)
	{
		const auto& params = pRuntime->ComponentParams();
		const float sizeDist = params.m_maxParticleSize * angularDensity * params.m_visibility.m_viewDistanceMultiple;
		const float dist = min(sizeDist, +params.m_visibility.m_maxCameraDistance);
		SetMax(maxViewDist, dist);
	}
	return maxViewDist;
}

void CParticleEmitter::GetMemoryUsage(ICrySizer* pSizer) const
{
}

void CParticleEmitter::SetViewDistRatio(int nViewDistRatio)
{
	IRenderNode::SetViewDistRatio(nViewDistRatio);
	m_viewDistRatio = GetViewDistRatioNormilized();
}

void CParticleEmitter::ReleaseNode(bool bImmediate)
{
	CRY_ASSERT((m_dwRndFlags & ERF_PENDING_DELETE) == 0);

	Activate(false);
	Unregister();
}

void CParticleEmitter::InitSeed()
{
	const int forcedSeed = GetCVars()->e_ParticlesForceSeed;
	if (m_spawnParams.nSeed != -1)
	{
		m_initialSeed = uint32(m_spawnParams.nSeed);
		m_time = 0.0f;
	}
	else if (forcedSeed != 0)
	{
		m_initialSeed = forcedSeed;
		m_time = 0.0f;
	}
	else
	{
		m_initialSeed = cry_random_uint32();
		m_time = gEnv->pTimer->GetCurrTime();
	}
	m_timeCreated = m_time;
	m_currentSeed = m_initialSeed;
}

void CParticleEmitter::Activate(bool activate)
{
	if (!m_pEffect || activate == m_active)
		return;

	const auto& timings = m_pEffect->GetTimings();
	m_active = activate;
	if (activate)
	{
		InitSeed();
		m_timeUpdated = m_time;
		m_timeStable = m_time + timings.m_equilibriumTime;
		m_timeDeath = m_time + timings.m_maxTotalLIfe;
		m_bounds = m_realBounds = m_nextBounds = AABB::RESET;
		m_boundsChanged = false;
		m_alive = true;

		m_effectEditVersion = -1; // Force creation of Runtimes next Update
	}
	else
	{
		int hasParticles = 0;
		for (auto& pRuntime : m_componentRuntimes)
		{
			if (!pRuntime->IsChild())
				pRuntime->RemoveAllSpawners();
			hasParticles += pRuntime->HasParticles();
		}
		m_timeDeath = m_time;
		if (hasParticles)
			m_timeDeath += timings.m_stableTime;
		m_timeStable = m_timeDeath;
	}
}

void CParticleEmitter::Restart()
{
	if (m_time != m_timeCreated)
		Activate(false);
	Activate(true);
}

void CParticleEmitter::SetChanged()
{
	// Update stability time
	if (m_pEffect)
	{
		const auto& timings = m_pEffect->GetTimings();
		const float frameTime = gEnv->pTimer->GetFrameTime() * GetTimeScale();
		m_timeStable = max(timings.m_equilibriumTime, m_time + timings.m_stableTime) + frameTime;
	}
}

void CParticleEmitter::Kill()
{
	m_active = false;
	m_alive = false;
}

bool CParticleEmitter::IsActive() const
{
	return m_active;
}

void CParticleEmitter::SetLocation(const QuatTS& loc)
{
	CRY_PFX2_PROFILE_DETAIL;

	QuatTS prevLoc = m_location;
	m_location = loc;
	if (m_spawnParams.bPlaced && m_pEffect->IsSubstitutedPfx1())
	{
		ParticleLoc::RotateYtoZ(m_location.q);
	}

	m_parentContainer.GetIOVec3Stream(EPVF_Position).Store(0, m_location.t);
	m_parentContainer.GetIOQuatStream(EPQF_Orientation).Store(0, m_location.q);

	if (IsEquivalent(m_location, prevLoc, RAD_EPSILON, 0.001f))
		return;

	if (m_registered)
	{
		const float deltaTime = gEnv->pTimer->GetFrameTime();
		const float invDeltaTime = abs(deltaTime) > FLT_EPSILON ? rcp_fast(deltaTime) : 0.0f;
		const Vec3 velocity0 = m_parentContainer.GetIOVec3Stream(EPVF_Velocity).Load(0);
		const Vec3 angularVelocity0 = m_parentContainer.GetIOVec3Stream(EPVF_AngularVelocity).Load(0);

		const Vec3 velocity1 =
		  velocity0 +
		  (m_location.t - prevLoc.t) * invDeltaTime;
		const Vec3 angularVelocity1 =
		  angularVelocity0 +
		  2.0f * Quat::log(m_location.q * prevLoc.q.GetInverted()) * invDeltaTime;

		m_parentContainer.GetIOVec3Stream(EPVF_Velocity).Store(0, velocity1);
		m_parentContainer.GetIOVec3Stream(EPVF_AngularVelocity).Store(0, angularVelocity1);
		m_visEnviron.Invalidate();
	}

	SetChanged();
}

void CParticleEmitter::EmitParticle(const EmitParticleData* pData)
{
	// #PFX2_TODO : handle EmitParticleData (create new spawners)
	for (auto pRuntime: m_componentRuntimes)
	{
		if (!pRuntime->IsChild())
		{
			pRuntime->EmitParticle();
			SetChanged();
		}
	}
}

void CParticleEmitter::SetEmitterFeatures(TParticleFeatures& features)
{
	if (features.m_editVersion != m_emitterEditVersion)
	{
		m_emitterFeatures = features;
		m_emitterEditVersion = features.m_editVersion;
	}
}

void CParticleEmitter::SetEntity(IEntity* pEntity, int nSlot)
{
	m_entityOwner = pEntity;
	m_entitySlot = nSlot;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEmitter::InvalidateCachedEntityData()
{
	UpdateFromEntity();
}

void CParticleEmitter::SetTarget(const ParticleTarget& target)
{
	if ((int)target.bPriority >= (int)m_target.bPriority)
	{
		m_target = target;
		SetChanged();
	}
}

void CParticleEmitter::UpdateStreamingPriority(const SUpdateStreamingPriorityContext& context)
{
	FUNCTION_PROFILER_3DENGINE;

	for (auto& pRuntime : m_componentRuntimes)
	{
		const auto* pComponent = pRuntime->GetComponent();
		const SComponentParams& params = pComponent->GetComponentParams();
		const float normalizedDist = context.distance 
			* context.pPassInfo->GetZoomFactor()
			* rcp_safe(params.m_maxParticleSize);
		const bool bHighPriority = !!(params.m_renderObjectFlags & FOB_NEAREST);

		if (CMatInfo* pMatInfo  = static_cast<CMatInfo*>(params.m_pMaterial.get()))
		{
			const float adjustedDist = normalizedDist
				* min(params.m_shaderData.m_tileSize[0], params.m_shaderData.m_tileSize[1]);
			pMatInfo->PrecacheMaterial(adjustedDist, nullptr, context.bFullUpdate, bHighPriority);
		}

		if (CStatObj* pStatObj = static_cast<CStatObj*>(params.m_pMesh.get()))
			m_pObjManager->PrecacheStatObj(pStatObj, context.lod,
				pStatObj->GetMaterial(), context.importance, normalizedDist, context.bFullUpdate, bHighPriority);
	}
}

void CParticleEmitter::UpdatePhysEnv()
{
	CRY_PFX2_PROFILE_DETAIL;
	CParticleManager::Instance()->GetPhysEnviron().Update(m_physEnviron, 
		m_bounds, m_visEnviron.OriginIndoors(), m_pEffect->GetEnvironFlags() | ENV_WATER, true, 0);
}

void CParticleEmitter::GetSpawnParams(SpawnParams& spawnParams) const
{
	spawnParams = m_spawnParams;
}

void CParticleEmitter::SetEmitGeom(const GeomRef& geom)
{
	// If emitter has an OwnerEntity, it will override this GeomRef on the next frame.
	// So SetOwnerEntity(nullptr) should be called as well.
	m_emitterGeometry = geom;
	SetChanged();
}

void CParticleEmitter::SetSpawnParams(const SpawnParams& spawnParams)
{
	if (spawnParams.bIgnoreVisAreas != m_spawnParams.bIgnoreVisAreas || spawnParams.bRegisterByBBox != m_spawnParams.bRegisterByBBox)
		Unregister();
	bool update = spawnParams.eSpec != m_spawnParams.eSpec;
	m_spawnParams = spawnParams;
	SetChanged();
	if (update)
		m_componentRuntimesFor.clear();
}

uint CParticleEmitter::GetAttachedEntityId()
{
	return m_entityOwner ? m_entityOwner->GetId() : INVALID_ENTITYID;
}

void CParticleEmitter::UpdateRuntimes()
{
	CRY_PFX2_PROFILE_DETAIL;

	m_componentRuntimesFor.clear();
	TRuntimes newRuntimes;

	m_pEffectOriginal->Compile();

	if (!m_emitterFeatures.empty())
	{
		// clone and modify effect
		m_pEffect = new CParticleEffect(*m_pEffectOriginal);
		for (auto& pComponent : m_pEffect->GetComponents())
		{
			pComponent = new CParticleComponent(*pComponent);
			pComponent->SetEffect(m_pEffect);
			pComponent->ClearChildren();
			if (auto pParentComponent = pComponent->GetParentComponent())
			{
				auto pNewParent = m_pEffect->GetComponent(pParentComponent->GetComponentId());
				pComponent->SetParent(pNewParent);
			}
			else
			{
				for (auto& feature : m_emitterFeatures)
					pComponent->AddFeature(static_cast<CParticleFeature*>(feature.get()));
			}
		}
		m_pEffect->SetChanged();
		m_pEffect->Compile();
	}
	else
	{
		m_pEffect = m_pEffectOriginal;
	}

	ResetRenderObjects();

	for (const auto& pComponent: m_pEffect->GetComponents())
	{
		if (!pComponent->CanMakeRuntime(this))
		{
			m_componentRuntimesFor.push_back(nullptr);
			continue;
		}

		CParticleComponentRuntime* pRuntime = stl::find_value_if(m_componentRuntimes,
			[&](CParticleComponentRuntime* pRuntime)
			{
				return pRuntime->GetComponent() == pComponent;
			}
		);

		if (!pRuntime || !pRuntime->IsValidForComponent())
		{
			pRuntime = new CParticleComponentRuntime(this, pComponent);
		}
		else
		{
			pRuntime->Initialize();
			if (pRuntime->HasParticles())
				pComponent->OnEdit(*pRuntime);
		}

		newRuntimes.push_back(pRuntime);
		m_componentRuntimesFor.push_back(pRuntime);
	}

	m_componentRuntimes = newRuntimes;

	m_effectEditVersion = m_pEffectOriginal->GetEditVersion() + m_emitterEditVersion;
	m_alive = true;
}

void CParticleEmitter::ResetRenderObjects()
{
	if (!GetRenderer())
		return;

	if (!m_pEffect)
		return;

	const uint numROs = m_pEffect->GetNumRenderObjectIds();
	for (uint threadId = 0; threadId < RT_COMMAND_BUF_COUNT; ++threadId)
	{
		for (auto& object : m_pRenderObjects[threadId])
		{
			if (CRenderObject* pRO = object.first)
			{
				if (pRO->m_pRE)
					pRO->m_pRE->Release();
				gEnv->pRenderer->EF_FreeObject(pRO);
			}
			object = { nullptr, nullptr };
		}
		m_pRenderObjects[threadId].resize(numROs, { nullptr, nullptr });
	}
}

ILINE void CParticleEmitter::UpdateFromEntity()
{
	if (m_pEffect->GetEnvironFlags() & ENV_TARGET)
		UpdateTarget(m_target, m_entityOwner, GetLocation().t);
}

IEntity* CParticleEmitter::GetEmitGeometryEntity() const
{
	CRY_PFX2_PROFILE_DETAIL;
	IEntity* pEntity = m_entityOwner;
	if (pEntity)
	{
		// Override m_emitterGeometry with geometry of owning or attached entity if it exists
		if (IEntity* pParent = pEntity->GetParent())
			pEntity = pParent;
	}
	return pEntity;
}

void CParticleEmitter::UpdateEmitGeomFromEntity()
{
	CRY_PFX2_PROFILE_DETAIL;
	IEntity* pEntity = GetEmitGeometryEntity();
	m_emitterGeometrySlot = m_emitterGeometry.Set(pEntity);
}

QuatTS CParticleEmitter::GetEmitterGeometryLocation() const
{
	CRY_PFX2_PROFILE_DETAIL;
	if (IEntity* pEntity = GetEmitGeometryEntity())
	{
		SEntitySlotInfo slotInfo;
		if (pEntity->GetSlotInfo(m_emitterGeometrySlot, slotInfo))
		{
			if (slotInfo.pWorldTM)
				return QuatTS(*slotInfo.pWorldTM);
		}

	}
	return m_location;
}

CRenderObject* CParticleEmitter::GetRenderObject(uint threadId, uint renderObjectIdx)
{
	CRY_PFX2_ASSERT(threadId < RT_COMMAND_BUF_COUNT);
	if (m_pRenderObjects[threadId].empty())
		return nullptr;
	return m_pRenderObjects[threadId][renderObjectIdx].first;
}

void CParticleEmitter::SetRenderObject(CRenderObject* pRenderObject, _smart_ptr<IMaterial>&& material, uint threadId, uint renderObjectIdx)
{
	CRY_PFX2_ASSERT(threadId < RT_COMMAND_BUF_COUNT);
	m_pRenderObjects[threadId][renderObjectIdx] = std::make_pair(pRenderObject, std::move(material));
}

void CParticleEmitter::Register()
{
	CRY_PFX2_PROFILE_DETAIL;

	if (m_registered)
		gEnv->p3DEngine->UnRegisterEntityDirect(this);
	if (!HasBounds())
	{
		m_registered = false;
		return;
	}

	bool posContained = GetBBox().IsContainPoint(GetPos());
	SetRndFlags(ERF_REGISTER_BY_POSITION, posContained);
	SetRndFlags(ERF_REGISTER_BY_BBOX, m_spawnParams.bRegisterByBBox);
	SetRndFlags(ERF_RENDER_ALWAYS, m_spawnParams.bIgnoreVisAreas);
	SetRndFlags(ERF_CASTSHADOWMAPS, false);
	SetRndFlags(ERF_FOB_ALLOW_TERRAIN_LAYER_BLEND, !m_spawnParams.bIgnoreTerrainLayerBlend);
	SetRndFlags(ERF_FOB_ALLOW_DECAL_BLEND, !m_spawnParams.bIgnoreDecalBlend);
	gEnv->p3DEngine->RegisterEntity(this);
	m_registered = true;

	m_visEnviron.Invalidate();
	m_visEnviron.Update(GetPos(), m_bounds);
	m_physEnviron.OnPhysAreaChange();
}

void CParticleEmitter::Unregister()
{
	if (!m_registered)
		return;
	CRY_PFX2_PROFILE_DETAIL;

	gEnv->p3DEngine->UnRegisterEntityDirect(this);
	m_registered = false;
	m_bounds.Reset();
	m_realBounds.Reset();
}

void CParticleEmitter::Clear()
{
	CRY_PFX2_PROFILE_DETAIL;
	m_alive = m_active = false;
	Unregister();
	m_componentRuntimes.clear();
	m_componentRuntimesFor.clear();
}

bool CParticleEmitter::HasParticles() const
{
	for (auto& pRuntime : m_componentRuntimes)
	{
		if (pRuntime->HasParticles())
			return true;
	}

	return false;
}

uint CParticleEmitter::GetParticleSpec() const
{
	if (m_spawnParams.eSpec != EParticleSpec::Default)
		return uint(m_spawnParams.eSpec);

	return GetPSystem()->GetParticleSpec();
}

}
