// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/IGpuParticles.h>
#include "Gpu/GpuComputeBackend.h"
#include <memory.h>

namespace gpu_pfx2
{

// this structure is mirrored on GPU
struct SDefaultParticleData
{
	Vec3   Position;
	Vec3   Velocity;
	uint32 Color;
	uint32 AuxData;
};

class CParticleContainer
{
public:
	CParticleContainer(int maxParticles) : m_defaultData(maxParticles), m_counter(4) {}
	void                       Initialize(bool isDoubleBuffered);
	size_t                     GetMaxParticles() const  { return m_defaultData.Get().GetSize(); }
	bool                       HasDefaultParticleData() { return m_defaultData.Get().IsDeviceBufferAllocated(); }

	void                       Clear()                  { m_defaultData.Reset(); }
	void                       Swap()                   { m_defaultData.Swap(); }

	void                       ReadbackCounter();
	int                        RetrieveCounter();

	ID3D11ShaderResourceView*  GetDefaultParticleDataSRV()           { return m_defaultData.Get().GetSRV(); };
	ID3D11UnorderedAccessView* GetDefaultParticleDataUAV()           { return m_defaultData.Get().GetUAV(); };
	ID3D11UnorderedAccessView* GetDefaultParticleDataBackbufferUAV() { return m_defaultData.GetBackBuffer().GetUAV(); };

private:
	// this will only be double buffered when needed (i.e. when the particles get sorted)
	gpu::CDoubleBuffered<gpu::CTypedResource<SDefaultParticleData, gpu::BufferFlagsReadWrite>> m_defaultData;
	gpu::CTypedResource<uint, gpu::BufferFlagsReadWriteReadback>                               m_counter;
};
}
