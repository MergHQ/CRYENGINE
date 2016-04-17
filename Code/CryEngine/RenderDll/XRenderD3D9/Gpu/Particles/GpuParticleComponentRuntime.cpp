// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuParticleComponentRuntime.h"
#include "GpuParticleManager.h"
#include "CREGpuParticle.h"
#include "Gpu/GpuMergeSort.h"

#include "Gpu/Physics/GpuPhysicsParticleFluid.h"

#include <CryCore/BitFiddling.h>

namespace gpu_pfx2
{
const int CParticleComponentRuntime::kThreadsInBlock = 1024;

static pfx2::EUpdateList s_updateListmapping[] = { pfx2::EUL_Spawn, pfx2::EUL_InitUpdate, pfx2::EUL_Update, pfx2::EUL_InitSubInstance };

void CParticleComponentRuntime::AddRemoveNewBornsParticles(SUpdateContext& context)
{
	if (!m_container.HasDefaultParticleData())
		return;

	// this means that the pipeline has already run, we get the kill list
	// and the amount of dead particles from the last frame, kill the particles
	// and swap them out by living ones at the end
	if (m_parameters->numParticles)
		SwapToEnd(context);

	{
		// this is the only place where there needs to be a lock during the render
		// thread processing, because here, the sub instance data is actually passed to the
		// render thread
		CryAutoLock<CryCriticalSection> lock(m_cs);
		m_subInstancesRenderThread = m_subInstances;

		m_parentDataSizeRenderThread = m_parentData.size();
		if (m_parentDataSizeRenderThread)
		{
			m_parentDataRenderThread.UpdateBufferContent(&m_parentData[0], m_parentDataSizeRenderThread);
		}
	}

	AddRemoveParticles(context);
	if (m_parameters->numNewBorns)
		UpdateNewBorns(context);
}

void CParticleComponentRuntime::UpdateParticles(SUpdateContext& context)
{
	if (!m_parameters->numParticles)
		return;

	m_parameters->deltaTime = context.deltaTime;
	m_parameters->currentTime = gEnv->pTimer->GetCurrTime();
	m_parameters->physAccel = m_envParams.physAccel;
	m_parameters->physWind = m_envParams.physWind;
	m_parameters->viewProjection = gcpRendD3D->m_CameraProjMatrixPrev;
	m_parameters->sortMode = m_sortMode;
	m_vertexShaderParams->axisScale = m_axisScale;

	const CCamera& cam = gEnv->p3DEngine->GetRenderingCamera();
	float zn = cam.GetNearPlane();
	float zf = cam.GetFarPlane();
	m_parameters->farToNearDistance = zf - zn;
	m_parameters->cameraPosition = cam.GetPosition();
	m_parameters.CopyToDevice();
	m_vertexShaderParams.CopyToDevice();

	if (m_sortMode != pfx2::eGpuSortMode_None)
		Sort(context);

	UpdateFeatures(context);
	FillKillList(context);
}

void CParticleComponentRuntime::CalculateBounds(ID3D11UnorderedAccessView* pReadbackUAV)
{
	assert(!!pReadbackUAV);
	if (!pReadbackUAV)
		return;

	if (CRenderer::CV_r_GpuParticlesConstantRadiusBoundingBoxes > 0)
	{
		m_bounds = AABB(m_parameters->emitterPosition, (float) CRenderer::CV_r_GpuParticlesConstantRadiusBoundingBoxes);
		return;
	}

	if (!m_parameters->numParticles)
	{
		m_bounds = AABB::RESET;
		return;
	}

	ID3D11UnorderedAccessView* pUAV[] =
	{
		m_container.GetDefaultParticleDataUAV(),
		nullptr,
		nullptr,
		pReadbackUAV
	};

	m_pBackend->SetUAVs(0, 4, pUAV);
	m_parameters.Bind();

	if (!m_pBackend->RunTechnique("CalculateBounds", kCalcBoundsBlocks, 1, 1))
		return;
}

void CParticleComponentRuntime::SetBoundsFromManager(const SReadbackData* pData)
{
	if (m_parameters->numParticles && m_parameters->managerSlot >= 0)
	{
		m_bounds.min.x = (float)pData[m_parameters->managerSlot].min.x / kBoundsScale;
		m_bounds.min.y = (float)pData[m_parameters->managerSlot].min.y / kBoundsScale;
		m_bounds.min.z = (float)pData[m_parameters->managerSlot].min.z / kBoundsScale;
		m_bounds.max.x = (float)pData[m_parameters->managerSlot].max.x / kBoundsScale;
		m_bounds.max.y = (float)pData[m_parameters->managerSlot].max.y / kBoundsScale;
		m_bounds.max.z = (float)pData[m_parameters->managerSlot].max.z / kBoundsScale;
	}
	else
	{
		m_bounds = AABB::RESET;
	}
}

void CParticleComponentRuntime::SetCounterFromManager(const uint32* pCounter)
{
	if (m_parameters->managerSlot >= 0)
		m_parameters->numKilled = pCounter[m_parameters->managerSlot];
	else
		m_parameters->numKilled = 0;
}

void CParticleComponentRuntime::SpawnParticles(int instanceId, uint32 count)
{
	if (count == 0)
		return;

	SSpawnEntry entry;
	entry.m_count = count;
	entry.m_instanceId = instanceId;
	m_spawnEntries.push_back(entry);
}

void CParticleComponentRuntime::Render(CRenderObject* pRenderObject, const SRenderingPassInfo& passInfo, const SRendParams& renderParams)
{
	IRenderer* pRenderer = gEnv->pRenderer;
	const int customTexId = renderParams.nTextureID;

	if (m_pRendElement)
	{
		if (customTexId > 0)
		{
			m_pRendElement->m_CustomTexBind[0] = customTexId;
		}
		else
		{
			if (passInfo.IsAuxWindow())
				m_pRendElement->m_CustomTexBind[0] = CTexture::s_ptexDefaultProbeCM->GetID();
			else
				m_pRendElement->m_CustomTexBind[0] = CTexture::s_ptexBlackCM->GetID();
		}
	}

	SRendParams rParams;
	pRenderer->EF_AddEf(
	  m_pRendElement.get(), pRenderObject->m_pCurrMaterial->GetShaderItem(), pRenderObject,
	  passInfo, EFSLIST_TRANSP, 1);
}

void CParticleComponentRuntime::RunTechnique(CCryNameTSCRC tech)
{
	ID3D11UnorderedAccessView* pUAV[] = { m_container.GetDefaultParticleDataUAV() };
	m_pBackend->SetUAVs(0, 1, pUAV);
	m_parameters.Bind();
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(
	  GetNumParticles(),
	  kThreadsInBlock);

	if (!m_pBackend->RunTechnique(tech, blocks, 1, 1))
		return;
}

void CParticleComponentRuntime::SetSRVs(const int numSRVs, ID3D11ShaderResourceView** SRVs)
{
	m_pBackend->SetSRVs(0u, numSRVs, SRVs);
}

void CParticleComponentRuntime::SetUpdateSRV(EFeatureUpdateSrvSlot slot, ID3D11ShaderResourceView* pSRV)
{
	m_updateSrvSlots[slot] = pSRV;
}

void CParticleComponentRuntime::SetUpdateFlags(uint64 flags)
{
	m_updateShaderFlags |= flags;
}

void CParticleComponentRuntime::AddRemoveParticles(const SUpdateContext& context)
{
	// spawn
	if (m_active)
	{
		for (CFeature* feature : m_gpuUpdateLists[eGpuUpdateList_Spawn])
		{
			feature->SpawnParticles(context);
		}
	}

	// now we have a spawn list in m_spawnEntries
	m_parameters->numNewBorns = 0;
	for (int i = 0; i < m_spawnEntries.size(); ++i)
	{
		m_parameters->numNewBorns += m_spawnEntries[i].m_count;
	}

	std::vector<uint> newBornData;
	newBornData.resize(m_parameters->numNewBorns);
	int index = 0;
	for (int i = 0; i < m_spawnEntries.size(); ++i)
	{
		for (int c = 0; c < m_spawnEntries[i].m_count; ++c)
		{
			newBornData[index] = std::min(m_spawnEntries[i].m_instanceId, m_parentDataSizeRenderThread - 1);
			index++;
		}
	}
	// cap because of size of newborn array
	m_parameters->numNewBorns =
	  min(m_maxNewBorns, m_parameters->numNewBorns);
	// cap global
	m_parameters->numNewBorns =
	  min(m_maxParticles - m_parameters->numParticles, m_parameters->numNewBorns);
	m_parameters->numParticles += m_parameters->numNewBorns;

	if (m_parameters->numNewBorns)
		m_newBornIndices.UpdateBufferContent(&newBornData[0], m_parameters->numNewBorns);

	m_parameters.CopyToDevice();
	m_spawnEntries.resize(0);
}

void CParticleComponentRuntime::UpdateNewBorns(const SUpdateContext& context)
{
	m_initializationShaderFlags = 0;

	m_particleInitializationParameters->velocityScale = 0.0f;

	for (CFeature* feature : m_gpuUpdateLists[eGpuUpdateList_InitUpdate])
	{
		feature->InitParticles(context);
	}

	m_initializationSrvSlots[eFeatureInitializationSrvSlot_newBornIndices] = m_newBornIndices.GetSRV();
	m_initializationSrvSlots[eFeatureInitializationSrvSlot_parentData] = m_parentDataRenderThread.GetSRV();

	m_pBackend->SetSRVs(0u, eFeatureInitializationSrvSlot_COUNT, m_initializationSrvSlots);
	ID3D11UnorderedAccessView* pUAV[] = { m_container.GetDefaultParticleDataUAV(), context.pReadbackUAV };
	m_pBackend->SetUAVs(0, 2, pUAV);
	m_parameters.CopyToDevice();
	m_parameters.Bind();
	m_particleInitializationParameters.CopyToDevice();
	m_particleInitializationParameters.Bind();
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(
	  m_parameters->numNewBorns,
	  kThreadsInBlock);
	ID3D11SamplerState* sampler[] =
	{
		(ID3D11SamplerState*) CTexture::s_TexStates[m_texSampler].m_pDeviceState,
		(ID3D11SamplerState*) CTexture::s_TexStates[m_texPointSampler].m_pDeviceState
	};
	gcpRendD3D->m_DevMan.BindSampler(CDeviceManager::TYPE_CS, sampler, 0, 2);

	if (!m_pBackend->RunTechnique("FeatureInitialization", blocks, 1, 1, m_initializationShaderFlags))
		return;
}

void CParticleComponentRuntime::FillKillList(const SUpdateContext& context)
{
	assert(!!context.pCounterUAV);
	if (!context.pCounterUAV)
		return;

	ID3D11UnorderedAccessView* pUAV[] =
	{
		m_container.GetDefaultParticleDataUAV(),
		m_killList.GetUAV(),
		context.pCounterUAV,
		context.pReadbackUAV
	};

	m_pBackend->SetUAVs(0, 4, pUAV);
	m_parameters.CopyToDevice();
	m_parameters.Bind();

	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(
	  GetNumParticles(),
	  kThreadsInBlock);

	if (!m_pBackend->RunTechnique("FillKillList", blocks, 1, 1))
		return;
}

void CParticleComponentRuntime::UpdateFeatures(const SUpdateContext& context)
{
	m_updateShaderFlags = 0;

	for (CFeature* feature : m_gpuUpdateLists[eGpuUpdateList_Update])
	{
		feature->Update(context);
	}

	m_pBackend->SetSRVs(0u, eFeatureUpdateSrvSlot_COUNT, m_updateSrvSlots);
	ID3D11UnorderedAccessView* pUAV[] = { m_container.GetDefaultParticleDataUAV() };
	m_pBackend->SetUAVs(0, 1, pUAV);
	m_parameters.Bind();
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(
	  GetNumParticles(),
	  kThreadsInBlock);
	ID3D11SamplerState* sampler[] =
	{
		(ID3D11SamplerState*)CTexture::s_TexStates[m_texSampler].m_pDeviceState,
		(ID3D11SamplerState*)CTexture::s_TexStates[m_texPointSampler].m_pDeviceState
	};
	gcpRendD3D->m_DevMan.BindSampler(CDeviceManager::TYPE_CS, sampler, 0, 2);

	if (!m_pBackend->RunTechnique("FeatureUpdate", blocks, 1, 1, m_updateShaderFlags))
		return;
}

void CParticleComponentRuntime::Sort(const SUpdateContext& context)
{
	{
		ID3D11UnorderedAccessView* pUAV[] =
		{
			m_container.GetDefaultParticleDataUAV(),
			m_pMergeSort->GetUAV()
		};
		m_pBackend->SetUAVs(0, 2, pUAV);
		m_parameters.CopyToDevice();
		m_parameters.Bind();

		const int processSlots = NextPower2(GetNumParticles());
		const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(processSlots, kThreadsInBlock);

		if (!m_pBackend->RunTechnique("PrepareSort", blocks, 1, 1))
			return;

		m_pMergeSort->Sort(processSlots);
	}
	{
		ID3D11ShaderResourceView* pSRV[] = { m_pMergeSort->GetResultSRV() };
		m_pBackend->SetSRVs(1, 1, pSRV);
		ID3D11UnorderedAccessView* pUAV[] =
		{
			m_container.GetDefaultParticleDataUAV(),
			m_container.GetDefaultParticleDataBackbufferUAV()
		};
		m_pBackend->SetUAVs(0, 2, pUAV);
		m_parameters.Bind();

		const int blocks =
		  gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(GetNumParticles(), kThreadsInBlock);

		if (!m_pBackend->RunTechnique("ReorderParticles", blocks, 1, 1))
			return;

		m_container.Swap();
	}
}

bool CParticleComponentRuntime::IsValidRuntimeForInitializationParameters(const pfx2::SRuntimeInitializationParameters& parameters)
{
	if (
	  parameters.maxNewBorns == m_maxNewBorns &&
	  parameters.maxParticles == m_maxParticles &&
	  parameters.usesGpuImplementation &&
	  parameters.sortMode == m_sortMode &&
	  parameters.facingMode == m_facingMode &&
	  parameters.axisScale == m_axisScale &&
	  parameters.isSecondGen == m_isSecondGen &&
	  parameters.parentId == m_parentId &&
	  parameters.version == m_version
	  )
		return true;
	return false;
}

const AABB& CParticleComponentRuntime::GetBounds() const
{
	return m_bounds;
}

void CParticleComponentRuntime::SetInitializationSRV(EFeatureInitializationSrvSlot slot, ID3D11ShaderResourceView* pSRV)
{
	m_initializationSrvSlots[slot] = pSRV;
}

void CParticleComponentRuntime::SetInitializationFlags(uint64 flags)
{
	m_initializationShaderFlags |= flags;
}

CParticleComponentRuntime::CParticleComponentRuntime(pfx2::IParticleComponent* pComponent, const pfx2::SRuntimeInitializationParameters& params)
	: m_bounds(AABB::RESET)
	, m_active(true)
	, m_state(IParticleComponentRuntime::EState::Uninitialized)
	, m_blockSums(params.maxParticles)
	, m_killList(params.maxParticles)
	, m_newBornIndices(params.maxNewBorns)
	, m_parentDataRenderThread(params.maxNewBorns)
	, m_maxParticles(params.maxParticles)
	, m_maxNewBorns(params.maxNewBorns)
	, m_container(params.maxParticles)
	, m_texSampler(0xFFFFFFFF)
	, m_texPointSampler(0xFFFFFFFF)
	, m_sortMode(params.sortMode)
	, m_facingMode(params.facingMode)
	, m_axisScale(params.axisScale)
{
	for (int i = 0; i < eGpuUpdateList_COUNT; ++i)
	{
		int size;
		m_gpuUpdateLists[i].resize(0);
		CFeature** list = reinterpret_cast<CFeature**>(pComponent->GetGpuUpdateList(s_updateListmapping[i], size));
		for (int j = 0; j < size; ++j)
			m_gpuUpdateLists[i].push_back(list[j]);
	}
	m_parentId = params.parentId;
	m_isSecondGen = params.isSecondGen;
	m_version = params.version;
}

void CParticleComponentRuntime::SetEmitterData(::IParticleEmitter* pEmitter)
{
	CryAutoLock<CryCriticalSection> lock(m_cs);
	m_parameters->emitterPosition = pEmitter->GetLocation().t;
	m_parameters->emitterOrientation = pEmitter->GetLocation().q;
	m_parentData.resize(m_subInstances.size());
	if (m_subInstances.size())
	{
		pEmitter->GetParentData(m_parentId, &m_subInstances[0], m_parentData.size(),
		                        &m_parentData[0]);
	}
}

void CParticleComponentRuntime::AddSubInstances(SInstance* pInstances, size_t count)
{
	CryAutoLock<CryCriticalSection> lock(m_cs);
	count = min(m_maxNewBorns - m_subInstances.size(), count);
	for (int i = 0; i < count; ++i)
	{
		m_subInstances.push_back(pInstances[i].m_parentId);
		m_spawnData.push_back(SSpawnData());
	}

	for (CFeature* feature : m_gpuUpdateLists[eGpuUpdateList_InitSubInstance])
	{
		feature->InitSubInstance(this, &m_spawnData[m_spawnData.size() - count], count);
	}
}

void CParticleComponentRuntime::RemoveSubInstance(size_t instanceId)
{
	CryAutoLock<CryCriticalSection> lock(m_cs);
	size_t toCopy = m_subInstances.size() - 1;
	m_subInstances[instanceId] = m_subInstances[toCopy];
	m_spawnData[instanceId] = m_spawnData[toCopy];
	m_subInstances.pop_back();
	m_spawnData.pop_back();
}

void CParticleComponentRuntime::RemoveAllSubInstances()
{
	CryAutoLock<CryCriticalSection> lock(m_cs);
	m_subInstances.clear();
	m_spawnData.clear();
}

void CParticleComponentRuntime::ReparentParticles(const uint* swapIds, const uint numSwapIds)
{
	CryAutoLock<CryCriticalSection> lock(m_cs);
	size_t toCopy = 0;
	for (size_t i = 0; i < m_subInstances.size(); ++i)
	{
		m_subInstances[i] = swapIds[m_subInstances[i]];
		if (m_subInstances[i] == -1)
			continue;
		m_subInstances[toCopy] = m_subInstances[i];
		++toCopy;
	}
	m_subInstances.erase(m_subInstances.begin() + toCopy, m_subInstances.end());
}

void CParticleComponentRuntime::SwapToEnd(SUpdateContext& context)
{
	assert(!!context.pCounterUAV);
	if (!context.pCounterUAV)
		return;

	CRY_ASSERT(m_parameters->numKilled <= m_parameters->numParticles);

	m_parameters.CopyToDevice();

	ID3D11UnorderedAccessView* pUAV[] =
	{
		m_container.GetDefaultParticleDataUAV(),
		m_killList.GetUAV(),
		context.pCounterUAV
	};

	uint nulls[4] = { 0 };
	gcpRendD3D->GetDeviceContext().ClearUnorderedAccessViewUint(context.pCounterUAV, nulls);

	m_pBackend->SetUAVs(0, 3, pUAV);
	m_parameters.Bind();
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(
	  m_parameters->numKilled,
	  kThreadsInBlock);
	if (!m_pBackend->RunTechnique("SwapToEnd", blocks, 1, 1))
		return;
	m_parameters->numParticles -= m_parameters->numKilled;
}

gpu_physics::CParticleFluidSimulation* CParticleComponentRuntime::CreateFluidSimulation()
{
	// [PFX2_TODO_GPU] : The maximum fluid particle number needs to be adjustable
	m_pFluidSimulation = std::unique_ptr<gpu_physics::CParticleFluidSimulation>(new gpu_physics::CParticleFluidSimulation(32 * 1024));
	return m_pFluidSimulation.get();
}

void CParticleComponentRuntime::FluidCollisions()
{
	if (m_parameters->numParticles)
	{
		gpu::CComputeBackend backend("GpuPhysicsParticleFluid");
		m_parameters.Bind();
		ID3D11SamplerState* sampler[] =
		{
			(ID3D11SamplerState*)CTexture::s_TexStates[m_texSampler].m_pDeviceState,
			(ID3D11SamplerState*)CTexture::s_TexStates[m_texPointSampler].m_pDeviceState
		};
		CTexture* zTarget = CTexture::s_ptexZTarget;
		ID3D11ShaderResourceView* srvs[] = { (ID3D11ShaderResourceView*) zTarget->GetShaderResourceView() };
		backend.SetSRVs(eFeatureUpdateSrvSlot_depthBuffer, 1u, srvs);
		gcpRendD3D->m_DevMan.BindSampler(CDeviceManager::TYPE_CS, sampler, 0, 2);
		m_pFluidSimulation->FluidCollisions(backend);
	}
}

void CParticleComponentRuntime::EvolveParticles()
{
	if (m_parameters->numParticles)
	{
		gpu::CComputeBackend backend("GpuPhysicsParticleFluid");

		ID3D11UnorderedAccessView* pUAV[] = { m_container.GetDefaultParticleDataUAV() };
		backend.SetUAVs(5, 1, pUAV);
		m_parameters.Bind();
		m_pFluidSimulation->EvolveParticles(backend, m_parameters->numParticles);
	}
}

void CParticleComponentRuntime::Initialize()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	memset(m_updateSrvSlots, 0, sizeof(m_updateSrvSlots));
	memset(m_initializationSrvSlots, 0, sizeof(m_initializationSrvSlots));

	m_particleInitializationParameters->velocityScale = 0.0;
	m_particleInitializationParameters->color = Vec3(1.0f, 1.0f, 1.0f);
	m_particleInitializationParameters->size = 1.0f;
	m_particleInitializationParameters->opacity = 1.0f;
	m_particleInitializationParameters.CreateDeviceBuffer();

	m_parameters.CreateDeviceBuffer();
	m_vertexShaderParams.CreateDeviceBuffer();

	m_parameters->deltaTime = 0.016f;
	m_parameters->numNewBorns = 0;
	m_parameters->numParticles = 0;
	m_parameters->managerSlot = -1;

	m_parameters.CopyToDevice();

	bool sorting = m_sortMode != pfx2::eGpuSortMode_None;
	m_container.Initialize(sorting);
	if (sorting)
		m_pMergeSort = std::unique_ptr<gpu::CMergeSort>(new gpu::CMergeSort(m_maxParticles));

	m_pBackend = std::unique_ptr<gpu::CComputeBackend>(new gpu::CComputeBackend("GpuParticles"));

	// Alloc GPU resources
	m_blockSums.CreateDeviceBuffer();
	m_killList.CreateDeviceBuffer();
	m_parentDataRenderThread.CreateDeviceBuffer();
	m_newBornIndices.CreateDeviceBuffer();

	STexState ts1(FILTER_BILINEAR, true);
	m_texSampler = CTexture::GetTexState(ts1);
	STexState ts2(FILTER_POINT, true);
	m_texPointSampler = CTexture::GetTexState(ts2);

	if (!m_pRendElement)
	{
		m_pRendElement = std::unique_ptr<CREGpuParticle>(new CREGpuParticle);
	}
	m_pRendElement->SetRuntime(this);
	m_pRendElement->m_CustomTexBind[0] = CTexture::s_ptexBlackCM->GetID(); // @filipe/ben TODO: set up proper cubemap
	m_pFluidSimulation.reset();

	m_state = EState::Ready;
}

void CParticleComponentRuntime::Reset()
{
	m_container.Clear();
	m_subInstances.clear();
	m_subInstances.shrink_to_fit();
	m_spawnData.clear();
	m_spawnData.shrink_to_fit();

	m_parameters->numParticles = 0;
	m_parameters->numNewBorns = 0;

	m_bounds = AABB::RESET;

	m_state = EState::Uninitialized;
}

void CParticleComponentRuntime::PrepareRelease()
{
	for (int i = 0; i < eGpuUpdateList_COUNT; ++i)
	{
		m_gpuUpdateLists[i].resize(0);
	}
}

void CParticleComponentRuntime::AccumCounts(SParticleCounts& counts)
{
	counts.EmittersAlloc += 1.0f;
	counts.ParticlesAlloc += m_parameters->numParticles;
	counts.EmittersActive += 1.0f;
	counts.SubEmittersActive += 1.0f;
	counts.ParticlesActive += m_parameters->numParticles;
}
}
