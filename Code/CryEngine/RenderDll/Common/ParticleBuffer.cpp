// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleBuffer.h"
#include <CryRenderer/RenderElements/CREParticle.h>
#include "DriverD3D.h"

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_LINUX || CRY_PLATFORM_APPLE
	#define PFX_32BIT_IB
#else
	#define PFX_16BIT_IB
#endif

CParticleSubBuffer::CParticleSubBuffer()
	: m_pLockedData(nullptr)
	, m_elemCount(0)
	, m_stride(0)
	, m_flags(0)
	, m_buffer(1)
{
}

CParticleSubBuffer::~CParticleSubBuffer()
{
	Release();
}

void CParticleSubBuffer::Create(uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags)
{
	m_elemCount = elemsCount;
	m_stride = stride;
	m_flags = flags;
	m_buffer.Create(elemsCount, stride, format, flags, nullptr);
}

void CParticleSubBuffer::Release()
{
	Unlock(0);
	m_buffer.Release();
	m_elemCount = 0;
	m_stride = 0;
	m_flags = 0;
}

byte* CParticleSubBuffer::Lock()
{
	if (m_pLockedData)
		return m_pLockedData;

	if (m_buffer.GetBuffer())
		m_pLockedData = reinterpret_cast<byte*>(m_buffer.Lock());

	return m_pLockedData;
}

void CParticleSubBuffer::Unlock(uint32 size)
{
	if (m_pLockedData && m_buffer.GetBuffer())
	{
		m_buffer.Unlock(size);
		// (1074511) !RB durango: removed usage of D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_NONCOHERENT from all created buffers as advised by microsoft. All dynamic buffers now use onion (D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT) and all static buffers use garlic now (D3D11_GRAPHICS_MEMORY_ACCESS_CPU_WRITECOMBINE_NONCOHERENT)
#if !CRY_PLATFORM_DURANGO
		CDeviceManager::InvalidateCpuCache(m_pLockedData, 0, m_elemCount * m_stride);
		CDeviceManager::InvalidateGpuCache(m_buffer.GetBuffer(), m_pLockedData, 0, m_elemCount * m_stride);
#endif
		m_pLockedData = nullptr;
	}
}

HRESULT CParticleSubBuffer::BindVB(uint streamNumber, int bytesOffset, int stride)
{
	assert((m_flags & DX11BUF_BIND_VERTEX_BUFFER) != 0);
	HRESULT h = gcpRendD3D->FX_SetVStream(
		streamNumber, m_buffer.GetBuffer(), bytesOffset,
		(stride == 0 ? m_stride : stride));
	CHECK_HRESULT(h);
	return h;
}

HRESULT CParticleSubBuffer::BindIB(uint offset)
{
	assert((m_flags & DX11BUF_BIND_INDEX_BUFFER) != 0);
	return gcpRendD3D->FX_SetIStream(
		m_buffer.GetBuffer(), offset, (m_stride == 2 ? Index16 : Index32));
}

void CParticleSubBuffer::BindSRV(CDeviceManager::SHADER_TYPE shaderType, uint slot)
{
	assert(m_buffer.GetSRV() != 0);
	auto pSRV = m_buffer.GetSRV();
	gcpRendD3D->m_DevMan.BindSRV(shaderType, &pSRV, slot, 1);
}

CParticleBufferSet::CParticleBufferSet()
	: m_valid(false)
	, m_maxSpriteCount(0)
{
	for (auto& fence : m_fences)
	{
		fence = 0;
	}
	Release();
}

CParticleBufferSet::~CParticleBufferSet()
{
	Release();
}

void CParticleBufferSet::Create(uint poolSize)
{
	for (auto& fence : m_fences)
	{
		gRenDev->m_DevMan.CreateFence(fence);
	}

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

void CParticleBufferSet::CreateSubBuffer(CParticleBufferSet::EBufferTypes type, uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags)
{
	SubBuffer& subBuffer = m_subBuffers[type];
	for (uint i = 0; i < CREParticle::numBuffers; ++i)
	{
		subBuffer.m_buffers[i].Create(elemsCount, stride, format, flags);
		subBuffer.m_pMemoryBase[i] = nullptr;
		subBuffer.m_offset[i] = 0;
	}
	subBuffer.m_availableMemory = elemsCount * stride;
}

void CParticleBufferSet::CreateSpriteBuffer(uint poolSize)
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

void CParticleBufferSet::Release()
{
	for (auto& fence : m_fences)
	{
		if (fence != 0)
		{
			gRenDev->m_DevMan.ReleaseFence(fence);
			fence = 0;
		}
	}

	for (uint i = 0; i < EBT_Total; ++i)
	{
		SubBuffer& subBuffer = m_subBuffers[i];
		subBuffer.m_availableMemory = 0;
		for (uint j = 0; j < CREParticle::numBuffers; ++j)
		{
			subBuffer.m_buffers[j].Release();
			subBuffer.m_offset[j] = 0;
			subBuffer.m_pMemoryBase[j] = nullptr;
		}
	}

	m_spriteIndexBuffer.Release();

	m_maxSpriteCount = 0;
	m_valid = false;
}

void CParticleBufferSet::Lock()
{
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	if (CRenderer::CV_r_multithreaded)
	{
		m_ids[fillId] = (m_ids[processId] + 1) % CREParticle::numBuffers;
		m_ids[processId] = (m_ids[processId] + (CREParticle::numBuffers - 1)) % CREParticle::numBuffers;
	}
	else
	{
		m_ids[processId] = (m_ids[processId] + 1) % CREParticle::numBuffers;
	}
	const uint lockId = m_ids[processId];

	for (auto& subBuffer : m_subBuffers)
		subBuffer.m_buffers[lockId].Unlock(0);

	for (auto& subBuffer : m_subBuffers)
	{
		subBuffer.m_pMemoryBase[lockId] = subBuffer.m_buffers[lockId].Lock();
		subBuffer.m_offset[lockId] = 0;
	}
}

void CParticleBufferSet::Unlock()
{
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	const uint numPulledElements = m_subBuffers[EBT_PositionsSRV].m_offset[bindId] / m_subBuffers[EBT_PositionsSRV].m_buffers[bindId].GetStride();
	m_subBuffers[EBT_AxesSRV].m_offset[bindId] = numPulledElements * m_subBuffers[EBT_AxesSRV].m_buffers[bindId].GetStride();
	m_subBuffers[EBT_ColorSTsSRV].m_offset[bindId] = numPulledElements * m_subBuffers[EBT_ColorSTsSRV].m_buffers[bindId].GetStride();

	for (auto& subBuffer : m_subBuffers)
	{
		subBuffer.m_buffers[bindId].Unlock(subBuffer.m_offset[bindId]);
		subBuffer.m_pMemoryBase[bindId] = nullptr;
		subBuffer.m_offset[bindId] = 0;
	}
}

void CParticleBufferSet::SetFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
	assert(m_valid);
	assert(gRenDev->m_pRT->IsRenderThread());

	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	gRenDev->m_DevMan.IssueFence(m_fences[bindId]);
#endif
}

void CParticleBufferSet::WaitForFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
	assert(m_valid);

	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	const uint cvId = m_ids[fillId];

	gRenDev->m_DevMan.SyncFence(m_fences[cvId], true, false);
#endif
}

uint CParticleBufferSet::GetAllocId() const
{
	const uint fillId = gRenDev->m_RP.m_nFillThreadID;
	return gRenDev->m_RP.m_particleBuffer.m_ids[fillId];
}

uint CParticleBufferSet::GetBindId() const
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];
	return bindId;
}

void CParticleBufferSet::Alloc(uint index, EBufferTypes type, uint numElems, SAlloc* pAllocOut)
{
	assert(pAllocOut != nullptr);
	assert(uint(type) < EBT_Total);

	SubBuffer& subBuffer = m_subBuffers[type];
	const uint stride = subBuffer.m_buffers[index].GetStride();
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

void CParticleBufferSet::Alloc(uint index, uint numElems, SAllocStreams* pAllocOut)
{
	assert(pAllocOut != nullptr);

	SubBuffer& subBuffer = m_subBuffers[EBT_PositionsSRV];
	const uint stride = subBuffer.m_buffers[index].GetStride();
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

void CParticleBufferSet::BindVB()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	SubBuffer& subBuffer = m_subBuffers[EBT_Vertices];
	CRY_ASSERT_MESSAGE(subBuffer.m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	subBuffer.m_buffers[bindId].BindVB(0, 0, sizeof(SVF_P3F_C4B_T4B_N3F2));
}

void CParticleBufferSet::BindIB()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	SubBuffer& subBuffer = m_subBuffers[EBT_Indices];
	CRY_ASSERT_MESSAGE(subBuffer.m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	subBuffer.m_buffers[bindId].BindIB(0);
}

void CParticleBufferSet::BindSRVs()
{
	const uint processId = gRenDev->m_RP.m_nProcessThreadID;
	const uint bindId = m_ids[processId];

	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_PositionsSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_AxesSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");
	CRY_ASSERT_MESSAGE(m_subBuffers[EBT_ColorSTsSRV].m_pMemoryBase[bindId] == nullptr, "Cannot bind buffer that wasn't unlocked.");

	m_subBuffers[EBT_PositionsSRV].m_buffers[bindId].BindSRV(CDeviceManager::TYPE_VS, 7);
	m_subBuffers[EBT_AxesSRV].m_buffers[bindId].BindSRV(CDeviceManager::TYPE_VS, 8);
	m_subBuffers[EBT_ColorSTsSRV].m_buffers[bindId].BindSRV(CDeviceManager::TYPE_VS, 9);
}

void CParticleBufferSet::BindSpriteIB()
{
#if defined(PFX_16BIT_IB)
	gcpRendD3D.FX_SetIStream(m_spriteIndexBuffer.GetBuffer(), 0, Index16);
#elif defined(PFX_32BIT_IB)
	gcpRendD3D.FX_SetIStream(m_spriteIndexBuffer.GetBuffer(), 0, Index32);
#endif
}

bool CParticleBufferSet::IsValid() const
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
