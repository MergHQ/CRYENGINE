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
	, m_scratch(kMaxRuntimes)
	, m_numRuntimesReadback(0)
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

void CManager::RenderThreadUpdate()
{
	const bool bAsynchronousCompute = CRenderer::CV_r_D3D12AsynchronousCompute & BIT((eStage_GpuParticles - eStage_FIRST_ASYNC_COMPUTE)) ? true : false;
	const bool bReadbackBoundingBox = CRenderer::CV_r_GpuParticlesConstantRadiusBoundingBoxes ? false : true;

	if (!CRenderer::CV_r_GpuParticles)
		return;

	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
	PROFILE_LABEL_SCOPE("GPU_PARTICLES");

	if (!m_readback.IsDeviceBufferAllocated())
	{
		m_readback.CreateDeviceBuffer();
		m_counter.CreateDeviceBuffer();
		m_scratch.CreateDeviceBuffer();

		// Full clear
		UINT nulls[4] = { 0 };

		CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_counter.GetBuffer().GetDeviceUAV(), nulls, 0, nullptr);
		CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_scratch.GetBuffer().GetDeviceUAV(), nulls, 0, nullptr);

		// initialize readback staging buffer
		m_counter.Readback(kMaxRuntimes);
		m_readback.Readback(kMaxRuntimes);
	}

	// only at this point, GPU resources are actually freed.
	ProcessResources();

	if (uint32 numRuntimes = uint32(GetReadRuntimes().size()))
	{
		gpu_pfx2::SUpdateContext context;
		context.pCounterBuffer = &m_counter.GetBuffer();
		context.pScratchBuffer = &m_scratch.GetBuffer();
		context.pReadbackBuffer = &m_readback.GetBuffer();
		context.deltaTime = gEnv->pTimer->GetFrameTime();

		{
			for (auto& pRuntime : GetReadRuntimes())
			{
				if (pRuntime->GetState() == CParticleComponentRuntime::EState::Uninitialized)
					pRuntime->Initialize();
			}
		}

		{
			PROFILE_LABEL_SCOPE("ADD REMOVE NEWBORNS & UPDATE");

			const uint* pCounter = m_counter.Map(m_numRuntimesReadback);
			const SReadbackData* pData = nullptr;

			if (bReadbackBoundingBox)
				pData = m_readback.Map(m_numRuntimesReadback);

			for (uint32 i = 0; i < numRuntimes; ++i)
			{
				// TODO: convert to array of command-lists pattern
				SScopedComputeCommandList pComputeInterface(bAsynchronousCompute);

				auto& pRuntime = GetReadRuntimes()[i];
				context.pRuntime = pRuntime;

				if (pData)
					pRuntime->SetBoundsFromManager(pData);

				pRuntime->SetCounterFromManager(pCounter);
				pRuntime->SetManagerSlot(i);
				pRuntime->AddRemoveNewBornsParticles(context, pComputeInterface);
				pRuntime->UpdateParticles(context, pComputeInterface);
				pRuntime->CalculateBounds(context, pComputeInterface);
			}

			m_counter.Unmap();
			if (pData)
				m_readback.Unmap();
		}

		{
			PROFILE_LABEL_SCOPE("READBACK");

			m_counter.Readback(numRuntimes);
			if (bReadbackBoundingBox)
				m_readback.Readback(numRuntimes);
			m_numRuntimesReadback = numRuntimes;
		}
	}
}

void CManager::RenderThreadPostUpdate()
{
	if (uint32 numRuntimes = uint32(GetReadRuntimes().size()))
	{
		{
			std::vector<CGpuBuffer*> UAVs;

			UAVs.reserve(numRuntimes);
			for (auto& pRuntime : GetReadRuntimes())
				UAVs.emplace_back(&pRuntime->PrepareForUse());

			// Prepare particle buffers which have been used in the vertex shader for compute use
			CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetGraphicsInterface()->PrepareUAVsForUse(numRuntimes, &UAVs[0]);
		}

		{
			// Minimal clear
			UINT nulls[4] = { 0 };
#if defined(DEVICE_SUPPORTS_D3D11_1)
			const UINT numRanges = 1;
			const D3D11_RECT uavRange = { 0, 0, numRuntimes, 0 };
			CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_counter.GetBuffer().GetDeviceUAV(), nulls, numRanges, &uavRange);
			CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_scratch.GetBuffer().GetDeviceUAV(), nulls, numRanges, &uavRange);
#else
			CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_counter.GetBuffer().GetDeviceUAV(), nulls, 0, nullptr);
			CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(m_scratch.GetBuffer().GetDeviceUAV(), nulls, 0, nullptr);
#endif
		}
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

void CManager::CleanupResources()
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
