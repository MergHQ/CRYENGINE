// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef GPU_FLUID_SIM_H
#define GPU_FLUID_SIM_H

#include <CryRenderer/IGpuPhysics.h>
#include "Gpu/GpuComputeBackend.h"

namespace gpu_physics
{
const int kThreadsInBlock = 1024u;

struct SGridCell
{
	uint count;
	uint sum;
	uint blockSum;
};

struct SParticleFluidParametersInternal : SParticleFluidParameters
{
	int numberOfBodies;
	int numberBodiesInject;
	int pad2[2];
};

struct SSimulationData
{
	SSimulationData(const int numBodies, const int totalGridSize)
		: bodies(numBodies), bodiesTemp(numBodies), bodiesOffsets(numBodies),
		grid(totalGridSize), bodiesInject(numBodies), adjacencyList(27)
	{
		bodies.CreateDeviceBuffer();
		bodiesTemp.CreateDeviceBuffer();
		bodiesOffsets.CreateDeviceBuffer();
		grid.CreateDeviceBuffer();
		bodiesInject.CreateDeviceBuffer();
		adjacencyList.CreateDeviceBuffer();
	};

	gpu::CTypedResource<SFluidBody, gpu::BufferFlagsReadWriteAppend>   bodies;
	gpu::CTypedResource<SFluidBody, gpu::BufferFlagsReadWriteReadback> bodiesTemp;
	gpu::CTypedResource<uint, gpu::BufferFlagsReadWrite>               bodiesOffsets;
	gpu::CTypedResource<SGridCell, gpu::BufferFlagsReadWrite>          grid;
	gpu::CTypedResource<SFluidBody, gpu::BufferFlagsDynamic>           bodiesInject;
	gpu::CTypedResource<int, gpu::BufferFlagsDynamic>                  adjacencyList;
};

// fwd
class CParticleEffector;

class CParticleFluidSimulation : public ISimulationInstance
{
public:
	enum { simulationType = eSimulationType_ParticleFluid };

	CParticleFluidSimulation(const int maxBodies);
	~CParticleFluidSimulation();

	// most of the simulation runs in the render thread
	void RenderThreadUpdate();

	void CreateResources();
	void EvolveParticles(gpu::CComputeBackend& backend, int numParticles);
	void FluidCollisions(gpu::CComputeBackend& backend);
protected:
	void InternalInjectBodies(const EBodyType type, const SBodyBase* b, const int numBodies);
	void InternalSetParameters(const EParameterType type, const SParameterBase* p);
	void SetUAVsAndConstants(gpu::CComputeBackend& backend);

	void RetrieveCounter();
	void UpdateDynamicBuffers();
	void UpdateSimulationBuffers(gpu::CComputeBackend& backend, const int cellBlock);
	void SortSimulationBodies(gpu::CComputeBackend& backend, const int gridCells, const int cellBlock, const int blocks);

	void IntegrateBodies(gpu::CComputeBackend& backend, const int blocks);

	void DebugDraw();

	const int m_maxBodies;

	gpu::CTypedConstantBuffer<SParticleFluidParametersInternal, 3> m_params;

	// resources
	std::unique_ptr<SSimulationData> m_pData;
	std::vector<SFluidBody>          m_bodiesInject;

	// only for debug
	std::vector<Vec3> m_points;
};
}

#endif
