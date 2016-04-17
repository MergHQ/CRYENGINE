// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleBuffer.h"
#include "FencedBuffer.h"
#include <CryRenderer/RenderElements/CREParticle.h>
#include "DriverD3D.h"

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_LINUX || CRY_PLATFORM_APPLE
	#define PFX_32BIT_IB
#else
	#define PFX_16BIT_IB
#endif

ParticleBufferSet::ParticleBufferSet()
	: m_valid(false)
	, m_maxSpriteCount(0)
{
	Release();
}

void ParticleBufferSet::Create(uint poolSize)
{
	CreateSubBuffer(
	  EBT_Vertices, poolSize, sizeof(SVF_P3F_C4B_T4B_N3F2), DXGI_FORMAT_UNKNOWN,
	  DX11BUF_BIND_VERTEX_BUFFER | DX11BUF_DYNAMIC);

	CreateSubBuffer(
	  EBT_Indices, poolSize * 3, sizeof(uint16), DXGI_FORMAT_R16_UINT,
	  DX11BUF_BIND_INDEX_BUFFER | DX11BUF_DYNAMIC);

	CreateSubBuffer(
	  EBT_PositionsSRV, poolSize, sizeof(Vec3), DXGI_FORMAT_R32G32B32_FLOAT,
	  DX11BUF_BIND_SRV | DX11BUF_DYNAMIC);
	CreateSubBuffer(
	  EBT_AxesSRV, poolSize, sizeof(SParticleAxes), DXGI_FORMAT_UNKNOWN,
	  DX11BUF_BIND_SRV | DX11BUF_DYNAMIC | DX11BUF_STRUCTURED);
	CreateSubBuffer(
	  EBT_ColorSTsSRV, poolSize, sizeof(SParticleColorST), DXGI_FORMAT_UNKNOWN,
	  DX11BUF_BIND_SRV | DX11BUF_DYNAMIC | DX11BUF_STRUCTURED);

	CreateSpriteBuffer(poolSize);

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	m_ids[fillId] = 1;
	m_ids[processId] = 0;

	m_valid = true;
}

void ParticleBufferSet::CreateSubBuffer(ParticleBufferSet::EBufferTypes type, uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags)
{
	SubBuffer& subBuffer = m_subBuffers[type];
	for (uint i = 0; i < nBuffers; ++i)
	{
		subBuffer.m_buffers[i] = std::unique_ptr<FencedBuffer>(new FencedBuffer(elemsCount, stride, format, flags));
		subBuffer.m_pMemoryBase[i] = nullptr;
		subBuffer.m_offset[i] = 0;
	}
	subBuffer.m_availableMemory = elemsCount * stride;
}

void ParticleBufferSet::CreateSpriteBuffer(uint poolSize)
{
#if defined(PFX_16BIT_IB)
	typedef uint16 idxFormat;
	m_maxSpriteCount = 1 << 14; // max sprite count for 16 bit ibs is 2^16/4
	const auto format = DXGI_FORMAT_R16_UINT;
#elif defined(PFX_32BIT_IB)
	typedef uint32 idxFormat;
	m_maxSpriteCount = poolSize;
	const auto format = DXGI_FORMAT_R32_UINT;
#endif

	std::vector<idxFormat> spriteIndices;
	spriteIndices.resize(m_maxSpriteCount * 6);
	for (uint particleId = 0; particleId < m_maxSpriteCount; ++particleId)
	{
		const uint baseIndexIdx = particleId * 6;
		const uint baseVertIdx = particleId * 4;
		const uint quadTessIdx[] = { 0, 1, 2, 2, 1, 3 };
		for (uint i = 0; i < 6; ++i)
			spriteIndices[baseIndexIdx + i] = idxFormat(baseVertIdx + quadTessIdx[i]);
	}
	m_spriteIndexBuffer.Create(
	  spriteIndices.size(), sizeof(idxFormat), format,
	  DX11BUF_BIND_INDEX_BUFFER,
	  spriteIndices.data());
}

void ParticleBufferSet::Release()
{
	for (uint i = 0; i < EBT_Total; ++i)
	{
		SubBuffer& subBuffer = m_subBuffers[i];
		subBuffer.m_availableMemory = 0;
		for (uint j = 0; j < nBuffers; ++j)
		{
			subBuffer.m_buffers[j].release();
			subBuffer.m_offset[j] = 0;
			subBuffer.m_pMemoryBase[j] = nullptr;
		}
	}

	m_spriteIndexBuffer.Release();

	m_maxSpriteCount = 0;
	m_valid = false;
}

void ParticleBufferSet::Lock()
{
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	if (CRenderer::CV_r_multithreaded)
	{
		m_ids[fillId] = (m_ids[processId] + 1) % nBuffers;
		m_ids[processId] = (m_ids[processId] + (nBuffers - 1)) % nBuffers;
	}
	else
	{
		m_ids[processId] = (m_ids[processId] + 1) % nBuffers;
	}
	const uint lockId = m_ids[processId];

	for (auto& subBuffer : m_subBuffers)
		subBuffer.m_buffers[lockId]->Unlock();

	for (auto& subBuffer : m_subBuffers)
	{
		subBuffer.m_pMemoryBase[lockId] = subBuffer.m_buffers[lockId]->Lock();
		subBuffer.m_offset[lockId] = 0;
	}
}

void ParticleBufferSet::Unlock()
{
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	for (auto& subBuffer : m_subBuffers)
	{
		subBuffer.m_buffers[bindId]->Unlock();
		subBuffer.m_pMemoryBase[bindId] = nullptr;
		subBuffer.m_offset[bindId] = 0;
	}
}

void ParticleBufferSet::SetFence()
{
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	for (auto& subBuffer : m_subBuffers)
		subBuffer.m_buffers[bindId]->SetFence();
}

void ParticleBufferSet::WaitForFence()
{
	assert(m_valid);

	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	const uint cvId = m_ids[fillId];

	for (auto& subBuffer : m_subBuffers)
		subBuffer.m_buffers[cvId]->WaitForFence();
}

uint ParticleBufferSet::GetAllocId() const
{
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	return gRenDev->m_RP.m_particleBuffer.m_ids[fillId];
}

void ParticleBufferSet::Alloc(uint index, EBufferTypes type, uint numElems, SAlloc* pAllocOut)
{
	assert(pAllocOut != nullptr);
	assert(uint(type) < EBT_Total);

	SubBuffer& subBuffer = m_subBuffers[type];
	const uint stride = subBuffer.m_buffers[index]->GetStride();
	LONG offset;
	do
	{
		offset = *(volatile LONG*)&subBuffer.m_offset[index];
		if (numElems * stride > (subBuffer.m_availableMemory - offset))
			numElems = (subBuffer.m_availableMemory - offset) / stride;
	}
	while (CryInterlockedCompareExchange((volatile LONG*)&subBuffer.m_offset[index], offset + numElems * stride, offset) != offset);

	pAllocOut->m_pBase = subBuffer.m_pMemoryBase[index];
	pAllocOut->m_firstElem = offset / stride;
	pAllocOut->m_numElemns = numElems;
}

void ParticleBufferSet::Alloc(uint index, uint numElems, SAllocStreams* pAllocOut)
{
	assert(pAllocOut != nullptr);

	SubBuffer& subBuffer = m_subBuffers[EBT_PositionsSRV];
	const uint stride = subBuffer.m_buffers[index]->GetStride();
	LONG offset;
	do
	{
		offset = *(volatile LONG*)&subBuffer.m_offset[index];
		if (numElems * stride > (subBuffer.m_availableMemory - offset))
			numElems = (subBuffer.m_availableMemory - offset) / stride;
	}
	while (CryInterlockedCompareExchange((volatile LONG*)&subBuffer.m_offset[index], offset + numElems * stride, offset) != offset);
	const uint firstElement = offset / stride;

	pAllocOut->m_pPositions = alias_cast<Vec3*>(m_subBuffers[EBT_PositionsSRV].m_pMemoryBase[index]) + firstElement;
	pAllocOut->m_pAxes = alias_cast<SParticleAxes*>(m_subBuffers[EBT_AxesSRV].m_pMemoryBase[index]) + firstElement;
	pAllocOut->m_pColorSTs = alias_cast<SParticleColorST*>(m_subBuffers[EBT_ColorSTsSRV].m_pMemoryBase[index]) + firstElement;
	pAllocOut->m_firstElem = firstElement;
	pAllocOut->m_numElemns = numElems;
}

void ParticleBufferSet::BindVB()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	SubBuffer& subBuffer = m_subBuffers[EBT_Vertices];
	CRY_ASSERT_MESSAGE(subBuffer.m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	subBuffer.m_buffers[bindId]->BindVB(0, 0, sizeof(SVF_P3F_C4B_T4B_N3F2));
}

void ParticleBufferSet::BindIB()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	SubBuffer& subBuffer = m_subBuffers[EBT_Indices];
	CRY_ASSERT_MESSAGE(subBuffer.m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	subBuffer.m_buffers[bindId]->BindIB(0);
}

void ParticleBufferSet::BindSRVs()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_PositionsSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_AxesSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_ColorSTsSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");

	m_subBuffers[EBT_PositionsSRV].m_buffers[bindId]->BindSRV(CDeviceManager::TYPE_VS, 7);
	m_subBuffers[EBT_AxesSRV].m_buffers[bindId]->BindSRV(CDeviceManager::TYPE_VS, 8);
	m_subBuffers[EBT_ColorSTsSRV].m_buffers[bindId]->BindSRV(CDeviceManager::TYPE_VS, 9);
}

void ParticleBufferSet::BindSpriteIB()
{
#if defined(PFX_16BIT_IB)
	gcpRendD3D.FX_SetIStream(m_spriteIndexBuffer.GetBuffer(), 0, Index16);
#elif defined(PFX_32BIT_IB)
	gcpRendD3D.FX_SetIStream(m_spriteIndexBuffer.GetBuffer(), 0, Index32);
#endif
}

bool ParticleBufferSet::IsValid() const
{
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	const uint cvId = m_ids[fillId];
	if (!m_valid)
		return false;
	for (int i = 0; i < EBT_Total; ++i)
	{
		if (m_subBuffers[i].m_pMemoryBase[cvId] == nullptr)
			return false;
	}
	return true;
}
