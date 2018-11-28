// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleContainer.h"

namespace
{

uint memData;
uint memContainer;

byte* ParticleAlloc(uint32 sz)
{
	if (!sz)
		return 0;
	void* ptr = CryModuleMemalign(sz, CRY_PFX2_PARTICLES_ALIGNMENT);
	memset(ptr, 0, sz);
	memData += sz;
	return (byte*)ptr;
}

void ParticleFree(void* ptr, uint32 sz)
{
	if (!ptr)
		return;
	CryModuleMemalignFree(ptr);
	memData -= sz;
}

}

namespace pfx2
{

CParticleContainer::CParticleContainer()
{
	static PUseData s_useData = NewUseData();
	SetUsedData(s_useData);
	memContainer += sizeof(*this);
}

CParticleContainer::CParticleContainer(const PUseData& pUseData)
{
	SetUsedData(pUseData);
	memContainer += sizeof(*this);
}

CParticleContainer::~CParticleContainer()
{
	ParticleFree(m_pData, m_capacity * m_useData.totalSize);
	memContainer -= sizeof(*this);
}

void CParticleContainer::Resize(uint32 newSize)
{
	CRY_PFX2_PROFILE_DETAIL;

	newSize = CRY_PFX2_GROUP_ALIGN(newSize);
	if (newSize <= m_capacity)
		return;

	const size_t newCapacity = CRY_PFX2_GROUP_ALIGN(newSize + min(newSize >> 1, m_capacity));

	byte* pNew = ParticleAlloc(newCapacity * m_useData.totalSize);
	if (m_capacity)
	{
		for (auto type : EParticleDataType::indices())
		{
			if (HasData(type))
			{
				uint offset = m_useData.offsets[type];
				const size_t stride = type.info().typeSize;
				memcpy(pNew + offset * newCapacity, m_pData + m_capacity * offset, m_lastId * stride);
			}
		}
	}
	ParticleFree(m_pData, m_capacity * m_useData.totalSize);
	m_pData = pNew;
	m_capacity = newCapacity;
}

void CParticleContainer::SetUsedData(const PUseData& pUseData)
{
	CRY_PFX2_PROFILE_DETAIL;

	// Transfer existing data
	if (m_capacity)
	{
		byte* pNew = ParticleAlloc(m_capacity * pUseData->totalSize);
		for (auto type : EParticleDataType::indices())
		{
			if (pUseData->Used(type) && m_useData.Used(type))
			{
				memcpy(pNew + m_capacity * pUseData->offsets[type], m_pData + m_capacity * m_useData.offsets[type], m_lastId * type.info().typeSize);
			}
		}
		ParticleFree(m_pData, m_capacity * m_useData.totalSize);
		m_pData = pNew;
	}
	m_useData = pUseData;
}

void CParticleContainer::CopyData(EParticleDataType dstType, EParticleDataType srcType, SUpdateRange range)
{
	CRY_PFX2_ASSERT(dstType.info().type == srcType.info().type);
	if (HasData(dstType) && HasData(srcType))
	{
		size_t stride = dstType.info().typeSize;
		size_t count = range.size();
		uint dim = dstType.info().dimension;
		for (uint i = 0; i < dim; ++i)
		{
			memcpy(
				ByteData(dstType + i) + stride * range.m_begin,
				ByteData(srcType + i) + stride * range.m_begin,
				stride * count);
		}
	}
}

void CParticleContainer::Clear()
{
	ParticleFree(m_pData, m_capacity * m_useData.totalSize);
	m_pData = nullptr;
	m_capacity = m_lastId = m_firstSpawnId = m_lastSpawnId = m_nextSpawnId = 0;
}

template<typename TData, typename FnCopy>
ILINE void SwapToEndRemove(TParticleId lastParticleId, TConstArray<TParticleId> toRemove, TData* pData, size_t stride, FnCopy fnCopy)
{
	const uint finalSize = lastParticleId - toRemove.size();
	uint end = lastParticleId - 1;
	uint i = 0;
	uint j = toRemove.size() - 1;
	for (; i < toRemove.size() && toRemove[i] < finalSize; ++i, --end)
	{
		for (; end == toRemove[j]; --j, --end)
			;
		fnCopy(pData + stride * toRemove[i], pData + stride * end, stride * sizeof(TData));
	}
}

void SwapToEndRemove(TParticleId lastParticleId, TConstArray<TParticleId> toRemove, void* pData, size_t stride)
{
	auto copyFn = [](uint8* pDest, uint8* pSrc, uint stride)
	{
		memcpy(pDest, pSrc, stride);
	};
	SwapToEndRemove(lastParticleId, toRemove, reinterpret_cast<uint8*>(pData), stride, copyFn);
}

template<typename TData>
void SwapToEndRemove(TParticleId lastParticleId, TConstArray<TParticleId> toRemove, TData* pData)
{
	auto copyFn = [](TData* pDest, TData* pSrc, uint stride)
	{
		*pDest = *pSrc;
	};
	SwapToEndRemove(lastParticleId, toRemove, pData, 1, copyFn);
}

void CParticleContainer::BeginAddParticles()
{
	m_firstSpawnId = m_lastSpawnId = m_lastId;
}

void CParticleContainer::AddParticle()
{
	AddParticles(1);
	ResetSpawnedParticles();
}

void CParticleContainer::AddParticles(uint count)
{
	if (!count)
		return;

	if (m_firstSpawnId <= m_lastId)
		m_firstSpawnId = m_lastSpawnId = CRY_PFX2_GROUP_ALIGN(m_lastId);

	m_lastSpawnId += count;
	m_nextSpawnId += count;

	Resize(m_lastSpawnId);
}

void CParticleContainer::RemoveParticles(TVarArray<TParticleId> toRemove, TVarArray<TParticleId> swapIds)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (toRemove.empty())
		return;

	if (!swapIds.empty())
		MakeSwapIds(toRemove, swapIds);

	for (auto dataTypeId : EParticleDataType::indices())
	{
		byte* pData = ByteData(dataTypeId);
		if (!pData)
			continue;
		const uint stride = dataTypeId.info().typeSize;
		switch (stride)
		{
		case 1:
			SwapToEndRemove(m_lastId, toRemove, reinterpret_cast<uint8*>(pData));
			break;
		case 4:
			SwapToEndRemove(m_lastId, toRemove, reinterpret_cast<uint32*>(pData));
			break;
		case 8:
			SwapToEndRemove(m_lastId, toRemove, reinterpret_cast<uint64*>(pData));
			break;
		default:
			SwapToEndRemove(m_lastId, toRemove, pData, stride);
		}
	}

	m_lastId -= toRemove.size();
	m_firstSpawnId = m_lastSpawnId = m_lastId;
}

void CParticleContainer::MakeSwapIds(TVarArray<TParticleId> toRemove, TVarArray<TParticleId> swapIds)
{
	CRY_PFX2_PROFILE_DETAIL;

	const uint finalSize = m_lastId - toRemove.size();
	CRY_PFX2_ASSERT(uint(swapIds.size()) >= m_lastId);    // swapIds not big enough

	for (TParticleId j = 0; j < m_lastId; ++j)
		swapIds[j] = j;

	SwapToEndRemove(m_lastId, toRemove, swapIds.data());
	for (uint i = finalSize; i < m_lastId; ++i)
		swapIds[i] = gInvalidId;

	for (uint i = 0; i < finalSize; ++i)
	{
		TParticleId v0 = swapIds[i];
		TParticleId v1 = swapIds[v0];
		swapIds[i] = v1;
		swapIds[v0] = i;
	}
}

void CParticleContainer::ResetSpawnedParticles()
{
	CRY_PFX2_PROFILE_DETAIL;

	if (m_firstSpawnId < m_lastId)
		return;

	const uint gapSize = m_firstSpawnId - m_lastId;
	if (gapSize != 0)
	{
		const uint numSpawn = GetNumSpawnedParticles();
		const uint movingId = m_lastSpawnId - min(gapSize, numSpawn);
		for (auto type : EParticleDataType::indices())
		{
			if (byte* pBytes = ByteData(type))
			{
				const size_t stride = type.info().typeSize;
				memcpy(pBytes + m_lastId * stride, pBytes + movingId * stride, gapSize * stride);
			}
		}
		m_lastSpawnId -= gapSize;
		m_firstSpawnId -= gapSize;
	}

	m_lastId = m_lastSpawnId;
}

}
