// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuParticleManager.h"
#include "GpuParticleFeatureFactory.h"
#include "GpuParticleComponentRuntime.h"

#include <algorithm>

namespace gpu_pfx2
{

static const int kMaxRuntimes = 4096;

CManager::CManager()
	: m_state(EState::Uninitialized)
	, m_readback(kMaxRuntimes)
	, m_counter(kMaxRuntimes)
{
}

void CManager::Initialize()
{
	m_state = EState::Ready;
}

void CManager::BeginFrame()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	if (m_state == EState::Uninitialized)
		Initialize();
}

_smart_ptr<IParticleComponentRuntime>
CManager::CreateParticleComponentRuntime(
  pfx2::IParticleComponent* pComponent,
  const pfx2::SRuntimeInitializationParameters& params)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	CryAutoLock<CryCriticalSection> lock(m_cs);

	CParticleComponentRuntime* pRuntime = new CParticleComponentRuntime(pComponent, params);
	_smart_ptr<IParticleComponentRuntime> result(pRuntime);
	GetWriteRuntimes().push_back(pRuntime);
	return result;
}

void CManager::RT_GpuKernelUpdateAll()
{
	if (!CRenderer::CV_r_GpuParticles)
		return;
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	PROFILE_LABEL_SCOPE("GPU_PARTICLES");

	if (!m_readback.IsDeviceBufferAllocated())
	{
		m_readback.CreateDeviceBuffer();
		m_counter.CreateDeviceBuffer();
	}

	// only at this point, GPU resources are actually freed.
	ProcessResources();

	gpu_pfx2::SUpdateContext context;
	context.pCounterUAV = m_counter.GetUAV();
	context.pReadbackUAV = m_readback.GetUAV();
	context.deltaTime = gEnv->pTimer->GetFrameTime();

	for (auto& pRuntime : GetReadRuntimes())
	{
		if (pRuntime->GetState() == CParticleComponentRuntime::EState::Uninitialized)
			pRuntime->Initialize();
	}

#if defined(CRY_USE_DX12)
	reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext())->BeginBatchingComputeTasks();
#endif

	{
		PROFILE_LABEL_SCOPE("MAP READBACK");

		if (CRenderer::CV_r_GpuParticlesConstantRadiusBoundingBoxes == 0)
		{
			const SReadbackData* pData = m_readback.Map();
			if (pData)
			{
				for (auto& pRuntime : GetReadRuntimes())
				{
					context.pRuntime = pRuntime;
					pRuntime->SetBoundsFromManager(pData);
				}
				m_readback.Unmap();
			}
		}

		if (const uint* pCounter = m_counter.Map())
		{
			for (auto& pRuntime : GetReadRuntimes())
			{
				pRuntime->SetCounterFromManager(pCounter);
			}
			m_counter.Unmap();
		}
	}

	if (m_counter.GetUAV())
	{
		uint nulls[4] = { 0 };
		gcpRendD3D->GetDeviceContext().ClearUnorderedAccessViewUint(m_counter.GetUAV(), nulls);
	}

	{
		PROFILE_LABEL_SCOPE("ADD REMOVE NEWBORNS");
		for (int i = 0; i < GetReadRuntimes().size(); ++i)
		{
			auto& pRuntime = GetReadRuntimes()[i];
			context.pRuntime = pRuntime;
			pRuntime->SetManagerSlot(i);
			pRuntime->AddRemoveNewBornsParticles(context);
		}
	}

	if (m_counter.GetUAV())
	{
		uint nulls[4] = { 0 };
		gcpRendD3D->GetDeviceContext().ClearUnorderedAccessViewUint(m_counter.GetUAV(), nulls);
	}

	{
		PROFILE_LABEL_SCOPE("UPDATE");
		for (auto& pRuntime : GetReadRuntimes())
		{
			context.pRuntime = pRuntime;
			pRuntime->UpdateParticles(context);
			pRuntime->CalculateBounds(m_readback.GetUAV());
		}
	}

#if defined(CRY_USE_DX12)
	reinterpret_cast<CCryDX12DeviceContext*>(gcpRendD3D->GetDeviceContext().GetRealDeviceContext())->EndBatchingComputeTasks();
#endif

	if (GetReadRuntimes().size())
	{
		PROFILE_LABEL_SCOPE("READBACK");
		if (CRenderer::CV_r_GpuParticlesConstantRadiusBoundingBoxes == 0)
			m_readback.Readback();
		m_counter.Readback();
	}
}

gpu::CBitonicSort* CManager::GetBitonicSort()
{
	if (!m_pBitonicSort)
		m_pBitonicSort = std::unique_ptr<gpu::CBitonicSort>(new gpu::CBitonicSort());
	return m_pBitonicSort.get();
}

void CManager::ProcessResources()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CryAutoLock<CryCriticalSection> lock(m_cs);

	PROFILE_LABEL_SCOPE("PROCESS RESOURCES");

	GetReadRuntimes() = GetWriteRuntimes();
	GetWriteRuntimes().erase(
	  std::remove_if(
	    GetWriteRuntimes().begin(),
	    GetWriteRuntimes().end(),
	    [](const _smart_ptr<CParticleComponentRuntime>& runtime)
		{
			return runtime->NumRefs() <= 2;
	  }),
	  GetWriteRuntimes().end());

	m_particleFeatureGpuInterfaces.erase(
	  std::remove_if(
	    m_particleFeatureGpuInterfaces.begin(),
	    m_particleFeatureGpuInterfaces.end(),
	    [](const _smart_ptr<CFeature>& feature)
		{
			return feature->NumRefs() == 1;
	  }),
	  m_particleFeatureGpuInterfaces.end());

	// initialize needed constant buffers and SRVs, upload data to gpu
	for (auto& feature : m_particleFeatureGpuInterfacesInitialization)
	{
		feature->Initialize();
	}

	m_particleFeatureGpuInterfacesInitialization.resize(0);
}

_smart_ptr<IParticleFeatureGpuInterface>
CManager::CreateParticleFeatureGpuInterface(EGpuFeatureType feature)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	CryAutoLock<CryCriticalSection> lock(m_cs);

	_smart_ptr<CFeature> result =
	  m_gpuInterfaceFactory.CreateInstance(feature);
	if (result != nullptr)
	{
		m_particleFeatureGpuInterfaces.push_back(result);
		m_particleFeatureGpuInterfacesInitialization.push_back(result);
	}
	return result;
}

void CManager::RT_CleanupResources()
{
	for (auto& runtime : GetWriteRuntimes())
		runtime->PrepareRelease();
	ProcessResources();

	if (m_readback.IsDeviceBufferAllocated())
	{
		m_readback.FreeDeviceBuffer();
		m_counter.FreeDeviceBuffer();
	}

	GetReadRuntimes().clear();
	if (GetWriteRuntimes().size() > 0)
		CryFatalError("There are still GPU runtimes living past the render thread.");
}

}
