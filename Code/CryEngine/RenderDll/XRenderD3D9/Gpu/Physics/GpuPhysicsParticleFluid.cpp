// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuPhysicsParticleFluid.h"

#include <CryPhysics/physinterface.h>

namespace gpu_physics
{
void SetupAdjacency(int* adjescency, int resX, int resY)
{
	// Build Adjacency Lookup
	int cell = 0;
	for (int y = -1; y <= 1; y++)
		for (int z = -1; z <= 1; z++)
			for (int x = -1; x <= 1; x++)
				adjescency[cell++] = (z * resY + y) * resX + x;
}

CParticleFluidSimulation::CParticleFluidSimulation(const int maxBodies)
	: m_maxBodies(maxBodies)
{
	assert(maxBodies % kThreadsInBlock == 0);
	m_points.resize(maxBodies);
	memset(&m_params.GetHostData(), 0x00, sizeof(SParticleFluidParametersInternal));

	m_params->numberOfIterations = 2;
	m_params->deltaTime = 0.008f;
	m_params->stiffness = 60.0f;
	m_params->gravityConstant = -1.f;
	m_params->h = 0.5f;
	m_params->r0 = 32.0f;
	m_params->mass = 1.0f;
	m_params->maxVelocity = 3.0f;
	m_params->maxForce = 10.f;
	m_params->gridSizeX = 16;
	m_params->gridSizeY = 16;
	m_params->gridSizeZ = 16;
	m_params->particleInfluence = 0.1f;
	m_params->numberOfBodies = 0;
	m_params->numberBodiesInject = 0;
	m_params->worldOffsetX = 0;
	m_params->worldOffsetY = 0;
	m_params->worldOffsetZ = 0;
	m_params->atmosphericDrag = 25.f;
	m_params->cohesion = 2.0f;
	m_params->baroclinity = 0.25f;

	CreateResources();
}

CParticleFluidSimulation::~CParticleFluidSimulation()
{
}

void CParticleFluidSimulation::InternalSetParameters(EParameterType type, const SParameterBase* p)
{
	if (type == EParameterType::ParticleFluid)
	{
		const SParticleFluidParameters* params = static_cast<const SParticleFluidParameters*>(p);

		m_params->deltaTime = params->deltaTime;
		m_params->numberOfIterations = params->numberOfIterations;
		m_params->stiffness = params->stiffness;
		m_params->gravityConstant = params->gravityConstant;
		m_params->h = params->h;
		m_params->r0 = params->r0;
		m_params->mass = params->mass;
		m_params->maxVelocity = params->maxVelocity;
		m_params->maxForce = params->maxForce;
		m_params->atmosphericDrag = params->atmosphericDrag;
		m_params->cohesion = params->cohesion;
		m_params->baroclinity = params->baroclinity;
		m_params->worldOffsetX = params->worldOffsetX;
		m_params->worldOffsetY = params->worldOffsetY;
		m_params->worldOffsetZ = params->worldOffsetZ;
		m_params->particleInfluence = params->particleInfluence;
	}
}

void CParticleFluidSimulation::CreateResources()
{
	m_params.CreateDeviceBuffer();

	const int totalGridSize = m_params->gridSizeX * m_params->gridSizeY * m_params->gridSizeZ;
	m_pData = std::unique_ptr<SSimulationData>(
	  new SSimulationData(m_maxBodies, totalGridSize));

	int adjacency[27];
	SetupAdjacency(adjacency, m_params->gridSizeX, m_params->gridSizeY);

	m_pData->adjacencyList.UpdateBufferContent(adjacency, 27);
}

void CParticleFluidSimulation::SetUAVsAndConstants(gpu::CComputeBackend& backend)
{
	ID3D11UnorderedAccessView* pUAV[] =
	{
		m_pData->bodies.GetUAV(),
		m_pData->bodiesTemp.GetUAV(),
		m_pData->bodiesOffsets.GetUAV(),
		m_pData->grid.GetUAV()
	};
	backend.SetUAVs(0, 4, pUAV);
	m_params.Bind();
}

void CParticleFluidSimulation::UpdateDynamicBuffers()
{
	if (m_bodiesInject.size() > 0)
	{
		m_params->numberBodiesInject = m_bodiesInject.size();
		m_pData->bodiesInject.UpdateBufferContent(&m_bodiesInject[0], m_params->numberBodiesInject);
	}
}

void CParticleFluidSimulation::RetrieveCounter()
{
	PROFILE_LABEL_SCOPE("RETRIEVE COUNTER");
	const int counter = m_pData->bodies.RetrieveCounter();
	m_params->numberOfBodies = counter;
}

void CParticleFluidSimulation::UpdateSimulationBuffers(gpu::CComputeBackend& backend, const int cellBlock)
{
	PROFILE_LABEL_SCOPE("INJECT BODIES");

	m_params.CopyToDevice();

	ID3D11ShaderResourceView* srv[] = { m_pData->bodiesInject.GetSRV() };
	backend.SetSRVs(0u, 1u, srv);
	SetUAVsAndConstants(backend);

	if (m_params->numberBodiesInject > 0)
	{
		if (!backend.RunTechnique("BodiesInject",
		                          (int)ceil((float)m_params->numberBodiesInject / cellBlock), 1, 1))
			return;
		m_params->numberOfBodies += m_params->numberBodiesInject;
		m_params->numberBodiesInject = 0;
		m_params.CopyToDevice();
	}
}

void CParticleFluidSimulation::SortSimulationBodies(
  gpu::CComputeBackend& backend,
  const int gridCells,
  const int cellBlock,
  const int blocks)
{
	PROFILE_LABEL_SCOPE("SORT");

	{
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("ClearGrid", gridCells / cellBlock, 1, 1))
			return;
	}
	{
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("AssignAndCount", blocks, 1, 1))
			return;
	}
	{
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("PrefixSumBlocks", gridCells / (2 * 512), 1, 1))
			return;
	}
	{
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("BuildGridIndices", gridCells / cellBlock, 1, 1))
			return;
	}
	{
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("RearrangeParticles", blocks, 1, 1))
			return;
	}
}

void CParticleFluidSimulation::EvolveParticles(gpu::CComputeBackend& backend, int numParticles)
{
	ID3D11ShaderResourceView* pSRV[] = { m_pData->adjacencyList.GetSRV() };
	backend.SetSRVs(0u, 1u, pSRV);
	const int blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(numParticles, kThreadsInBlock);
	SetUAVsAndConstants(backend);
	if (!backend.RunTechnique("EvolveExternalParticles",
	                          blocks, 1, 1))
		return;
}

void CParticleFluidSimulation::FluidCollisions(gpu::CComputeBackend& backend)
{
	ID3D11ShaderResourceView* pSRV[] = { m_pData->adjacencyList.GetSRV() };
	backend.SetSRVs(0u, 1u, pSRV);
	const uint blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(m_params->numberOfBodies, kThreadsInBlock);
	SetUAVsAndConstants(backend);
	if (!backend.RunTechnique("CollisionScreenSpace",
	                          blocks, 1, 1))
		return;
}

void CParticleFluidSimulation::IntegrateBodies(gpu::CComputeBackend& backend, const int blocks)
{
	PROFILE_LABEL_SCOPE("INTEGRATE");

	Vec3 minV(m_params->worldOffsetX, m_params->worldOffsetY, m_params->worldOffsetZ);
	Vec3 maxV(m_params->gridSizeX * m_params->h, m_params->gridSizeY * m_params->h, m_params->gridSizeZ * m_params->h);
	maxV += minV;

	if (CRenderer::CV_r_GpuPhysicsFluidDynamicsDebug)
	{
		gcpRendD3D.GetIRenderAuxGeom()->SetRenderFlags(e_Def3DPublicRenderflags);
		gcpRendD3D.GetIRenderAuxGeom()->DrawAABB(AABB(minV, maxV), false, RGBA8(0xff, 0xff, 0xff, 0xff), eBBD_Faceted);
	}

	// Position Based Update
#if 0
	for (int i = 0; i < m_params->numberOfIterations; ++i)
	{
		{
			backend.BeginPass("CalcLambda");
			backend.Dispatch(blocks, 1, 1);
			backend.EndPass();
		}
		{
			backend.BeginPass("DeltaPos");
			backend.Dispatch(blocks, 1, 1);
			backend.EndPass();
		}
	}
#else
	for (int i = 0; i < m_params->numberOfIterations; ++i)
	{
		{
			PROFILE_LABEL_SCOPE("Predict Density");
			{
				ID3D11ShaderResourceView* srv[] = { m_pData->adjacencyList.GetSRV() };
				backend.SetSRVs(0u, 1u, srv);
				SetUAVsAndConstants(backend);
			}
			if (!backend.RunTechnique("PredictDensity", blocks, 1, 1))
				return;
		}
		{
			PROFILE_LABEL_SCOPE("CorrectDensityError");
			{
				ID3D11ShaderResourceView* srv[] = { m_pData->adjacencyList.GetSRV() };
				backend.SetSRVs(0u, 1u, srv);
				SetUAVsAndConstants(backend);
			}
			if (!backend.RunTechnique("CorrectDensityError", blocks, 1, 1))
				return;
		}
		{
			PROFILE_LABEL_SCOPE("CorrectDivergenceError");
			{
				ID3D11ShaderResourceView* srv[] = { m_pData->adjacencyList.GetSRV() };
				backend.SetSRVs(0u, 1u, srv);
				SetUAVsAndConstants(backend);
			}
			if (!backend.RunTechnique("CorrectDivergenceError", blocks, 1, 1))
				return;
		}
	}
#endif
	{
		PROFILE_LABEL_SCOPE("POSITION UPDATE");
		SetUAVsAndConstants(backend);
		if (!backend.RunTechnique("PositionUpdate", blocks, 1, 1))
			return;
	}
}

void CParticleFluidSimulation::RenderThreadUpdate()
{
	const uint gridCells = m_params->gridSizeX * m_params->gridSizeY * m_params->gridSizeZ;
	const uint blocks = gpu::GetNumberOfBlocksForArbitaryNumberOfThreads(m_params->numberOfBodies, kThreadsInBlock);

	PROFILE_LABEL_SCOPE("FLUIDSIM");

	// there were already bodies injected
	if (m_params->numberOfBodies > 0)
		RetrieveCounter();

	UpdateDynamicBuffers();

	gpu::CComputeBackend backend("GpuPhysicsParticleFluid");

	UpdateSimulationBuffers(backend, kThreadsInBlock);
	SortSimulationBodies(backend, gridCells, kThreadsInBlock, blocks);
	SetUAVsAndConstants(backend);
	IntegrateBodies(backend, blocks);

	{
		PROFILE_LABEL_SCOPE("READBACK COUNTER");
		m_pData->bodies.ReadbackCounter();
	}

	m_bodiesInject.resize(0);
	DebugDraw();
}

void CParticleFluidSimulation::InternalInjectBodies(const EBodyType type, const SBodyBase* b, const int numBodies)
{
	if (type == EBodyType::Fluid)
	{
		m_bodiesInject.resize(numBodies);
		memcpy(&m_bodiesInject[0], b, sizeof(SFluidBody) * numBodies);
	}
}

void CParticleFluidSimulation::DebugDraw()
{
	if (CRenderer::CV_r_GpuPhysicsFluidDynamicsDebug)
	{
		m_pData->bodiesTemp.Readback();
		const SFluidBody* parts = m_pData->bodiesTemp.Map();

		gcpRendD3D.GetIRenderAuxGeom()->SetRenderFlags(e_Def3DPublicRenderflags);
		m_points.resize(m_params->numberOfBodies);

		for (int i = 0; i < m_params->numberOfBodies; ++i)
			m_points[i] = parts[i].x;

		m_pData->bodiesTemp.Unmap();

		if (m_params->numberOfBodies)
		{
			gcpRendD3D.GetIRenderAuxGeom()->DrawPoints(
			  &m_points[0],
			  m_params->numberOfBodies,
			  RGBA8(0xff, 0xff, 0xff, 0xff), 128);
		}
	}
}
}
