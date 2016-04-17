// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     24/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleContainer.h"

CRY_PFX2_DBG

namespace
{

static std::set<void*> g_allocMems;

void* ParticleAlloc(size_t sz)
{
	void* ptr = CryModuleMemalign(sz, CRY_PFX2_PARTICLES_ALIGNMENT);
	memset(ptr, 0, sz);
	return ptr;
}

void ParticleFree(void* ptr)
{
	CryModuleMemalignFree(ptr);
}

}

namespace pfx2
{

CParticleContainer::CParticleContainer()
{
	memset(m_pData, 0, sizeof(m_pData));
	Clear();
}

CParticleContainer::CParticleContainer(const CParticleContainer& copy)
{
	memset(m_pData, 0, sizeof(m_pData));
	Clear();
}

CParticleContainer::~CParticleContainer()
{
	Clear();
}

void CParticleContainer::Resize(size_t newSize)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (newSize < m_maxParticles)
		return;

	const size_t newMaxParticles = CRY_PFX2_PARTICLESGROUP_UPPER(newSize + (newSize >> 1)) + 1;

	void* prevBuffers[EPDT_Count];
	for (size_t type = 0; type < EPDT_Count; ++type)
	{
		const size_t stride = gParticleDataStrides[type];
		prevBuffers[type] = m_pData[type];
		m_pData[type] = 0;
		if (m_useData[type] && newMaxParticles > 0)
			m_pData[type] = ParticleAlloc(newMaxParticles * stride);
	}

	for (size_t type = 0; type < EPDT_Count; ++type)
	{
		if (!m_useData[type] && newMaxParticles > 0)
			continue;
		const size_t stride = gParticleDataStrides[type];
		const byte* pSource = reinterpret_cast<byte*>(prevBuffers[type]);
		byte* pDest = reinterpret_cast<byte*>(m_pData[type]);
		memcpy(pDest, pSource, m_lastId * stride);
	}

	for (size_t type = 0; type < EPDT_Count; ++type)
	{
		if (prevBuffers[type])
			ParticleFree(prevBuffers[type]);
	}

	m_maxParticles = newMaxParticles;
}

void CParticleContainer::ResetUsedData()
{
	for (size_t type = 0; type < EPDT_Count; ++type)
		m_useData[type] = false;
}

void CParticleContainer::AddParticleData(EParticleDataType type)
{
	CRY_PFX2_PROFILE_DETAIL;

	m_useData[type] = true;
	if (m_pData[type] == 0)
		m_pData[type] = ParticleAlloc(m_maxParticles * gParticleDataStrides[type]);
}

void CParticleContainer::AddParticleData(EParticleVec3Field type)
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	AddParticleData(xType);
	AddParticleData(yType);
	AddParticleData(zType);
}

void CParticleContainer::AddParticleData(EParticleQuatField type)
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	EParticleDataType wType = EParticleDataType(type + 3);
	AddParticleData(xType);
	AddParticleData(yType);
	AddParticleData(zType);
	AddParticleData(wType);
}

void CParticleContainer::Trim()
{
	for (size_t type = 0; type < EPDT_Count; ++type)
	{
		if (!m_useData[type] && m_pData[type] != 0)
		{
			ParticleFree(m_pData[type]);
			m_pData[type] = 0;
		}
	}
}

void CParticleContainer::Clear()
{
	CRY_PFX2_PROFILE_DETAIL;

	for (size_t i = 0; i < EPDT_Count; ++i)
	{
		if (m_pData[i] != 0)
			ParticleFree(m_pData[i]);
		m_pData[i] = 0;
		m_useData[i] = false;
	}
	m_maxParticles = CRY_PFX2_PARTICLESGROUP_STRIDE;
	m_lastId = 0;
	m_firstSpawnId = 0;
	m_lastSpawnId = 0;
	m_nextSpawnId = 0;
}

template<typename TData, typename FnCopy>
ILINE void SwapToEndRemoveTpl(TParticleId lastParticleId, const TParticleId* toRemove, size_t toRemoveCount, TData* pData, size_t stride, FnCopy fnCopy)
{
	const uint finalSize = lastParticleId - toRemoveCount;
	uint end = lastParticleId - 1;
	uint i = 0;
	uint j = toRemoveCount - 1;
	for (; i < toRemoveCount && toRemove[i] < finalSize; ++i, --end)
	{
		for (; end == toRemove[j]; --j, --end)
			;
		fnCopy(pData + stride * toRemove[i], pData + stride * end, stride * sizeof(TData));
	}
}

void SwapToEndRemove(TParticleId lastParticleId, const TParticleId* toRemove, size_t toRemoveCount, void* pData, size_t stride)
{
	auto copyFn = [](uint8* pDest, uint8* pSrc, uint stride)
	{
		memcpy(pDest, pSrc, stride);
	};
	SwapToEndRemoveTpl(
	  lastParticleId, toRemove, toRemoveCount,
	  reinterpret_cast<uint8*>(pData), stride, copyFn);
}

void SwapToEndRemoveStride1(TParticleId lastParticleId, const TParticleId* toRemove, size_t toRemoveCount, void* pData)
{
	auto copyFn = [](uint8* pDest, uint8* pSrc, uint stride)
	{
		*pDest = *pSrc;
	};
	SwapToEndRemoveTpl(
	  lastParticleId, toRemove, toRemoveCount,
	  reinterpret_cast<uint8*>(pData), 1, copyFn);
}

void SwapToEndRemoveStride4(TParticleId lastParticleId, const TParticleId* toRemove, size_t toRemoveCount, void* pData)
{
	auto copyFn = [](uint32* pDest, uint32* pSrc, uint stride)
	{
		*pDest = *pSrc;
	};
	SwapToEndRemoveTpl(
	  lastParticleId, toRemove, toRemoveCount,
	  reinterpret_cast<uint32*>(pData), 1, copyFn);
}

void SwapToEndRemoveStride8(TParticleId lastParticleId, const TParticleId* toRemove, size_t toRemoveCount, void* pData)
{
	auto copyFn = [](uint64* pDest, uint64* pSrc, uint stride)
	{
		*pDest = *pSrc;
	};
	SwapToEndRemoveTpl(
	  lastParticleId, toRemove, toRemoveCount,
	  reinterpret_cast<uint64*>(pData), 1, copyFn);
}

void CParticleContainer::AddParticle()
{
	SSpawnEntry entry = { 1 };
	AddRemoveParticles(&entry, 1, 0, 0);
}

void CParticleContainer::AddRemoveParticles(const SSpawnEntry* pSpawnEntries, size_t numSpawnEntries, const TParticleIdArray* pToRemove, TParticleIdArray* pSwapIds)
{
	CRY_PFX2_PROFILE_DETAIL;

	//////////////////////////////////////////////////////////////////////////

	size_t newSize = m_lastId;
	if (pToRemove)
		newSize -= pToRemove->size();
	if (pSpawnEntries)
	{
		for (size_t entryIdx = 0; entryIdx < numSpawnEntries; ++entryIdx)
		{
			const SSpawnEntry& spawnEntry = pSpawnEntries[entryIdx];
			newSize += spawnEntry.m_count;
		}
	}
	newSize = CRY_PFX2_PARTICLESGROUP_UPPER(newSize) + CRY_PFX2_PARTICLESGROUP_STRIDE + 1;
	Resize(newSize);

	//////////////////////////////////////////////////////////////////////////

	if (pToRemove && !pToRemove->empty())
	{
		const auto& toRemove = *pToRemove;
		const TParticleId lastParticleId = GetLastParticleId();
		const size_t toRemoveCount = toRemove.size();
		const uint finalSize = lastParticleId - toRemoveCount;

		for (size_t dataTypeId = 0; dataTypeId < EPDT_Count; ++dataTypeId)
		{
			const size_t stride = gParticleDataStrides[dataTypeId];
			void* pData = m_pData[dataTypeId];
			if (!m_useData[dataTypeId])
				continue;
			switch (stride)
			{
			case 1:
				SwapToEndRemoveStride1(lastParticleId, &toRemove[0], toRemoveCount, m_pData[dataTypeId]);
				break;
			case 4:
				SwapToEndRemoveStride4(lastParticleId, &toRemove[0], toRemoveCount, m_pData[dataTypeId]);
				break;
			case 8:
				SwapToEndRemoveStride8(lastParticleId, &toRemove[0], toRemoveCount, m_pData[dataTypeId]);
				break;
			default:
				SwapToEndRemove(lastParticleId, &toRemove[0], toRemoveCount, m_pData[dataTypeId], stride);
			}
		}

		if (pSwapIds && !pSwapIds->empty())
		{
			auto& swapIds = *pSwapIds;
			CRY_PFX2_ASSERT(uint(swapIds.size()) >= lastParticleId);    // swapIds not big enough

			for (size_t j = 0; j < lastParticleId; ++j)
				swapIds[j] = j;

			SwapToEndRemoveStride4(lastParticleId, &toRemove[0], toRemoveCount, swapIds.data());
			for (uint i = finalSize; i < lastParticleId; ++i)
				swapIds[i] = gInvalidId;

			for (uint i = 0; i < finalSize; ++i)
			{
				TParticleId v0 = swapIds[i];
				TParticleId v1 = swapIds[v0];
				swapIds[i] = v1;
				swapIds[v0] = i;
			}
		}
		m_lastId -= toRemoveCount;
	}

	//////////////////////////////////////////////////////////////////////////

	if (pSpawnEntries && numSpawnEntries)
	{
		m_firstSpawnId = m_lastSpawnId = CRY_PFX2_PARTICLESGROUP_UPPER(m_lastId) + 1;
		uint32 currentId = m_firstSpawnId;
		for (size_t entryIdx = 0; entryIdx < numSpawnEntries; ++entryIdx)
		{
			const SSpawnEntry& spawnEntry = pSpawnEntries[entryIdx];
			const size_t toAddCount = spawnEntry.m_count;

			if (HasData(EPDT_ParentId))
			{
				TParticleId* pParentIds = reinterpret_cast<TParticleId*>(m_pData[EPDT_ParentId]);
				for (uint32 i = currentId; i < currentId + toAddCount; ++i)
					pParentIds[i] = spawnEntry.m_parentId;
			}

			if (HasData(EPDT_SpawnId))
			{
				uint32* pSpawnIds = reinterpret_cast<TParticleId*>(m_pData[EPDT_SpawnId]);
				for (uint32 i = currentId; i < currentId + toAddCount; ++i)
					pSpawnIds[i] = m_nextSpawnId++;
			}

			if (HasData(EPDT_NormalAge))
			{
				// Store newborn ages
				float age = spawnEntry.m_ageBegin;
				float* pNormalAges = reinterpret_cast<float*>(m_pData[EPDT_NormalAge]);
				for (uint32 i = currentId; i < currentId + toAddCount; ++i, age += spawnEntry.m_ageIncrement)
					pNormalAges[i] = age;
			}

			if (HasData(EPDT_SpawnFraction))
			{
				float fraction = spawnEntry.m_fractionBegin;
				float* pSpawnFractions = reinterpret_cast<float*>(m_pData[EPDT_SpawnFraction]);
				for (uint32 i = currentId; i < currentId + toAddCount; ++i, fraction += spawnEntry.m_fractionCounter)
					pSpawnFractions[i] = fraction - uint(fraction);
			}

			currentId += toAddCount;
			m_lastSpawnId += toAddCount;
		}
	}
	else
	{
		m_firstSpawnId = m_lastSpawnId = m_lastId;
	}

	DebugParticleCounters();
}

void CParticleContainer::DebugRemoveParticlesStability(const TParticleIdArray& toRemove, const TParticleIdArray& swapIds) const
{
#ifdef CRY_DEBUG_PARTICLE_SYSTEM
	for (size_t i = 0; i < swapIds.size(); ++i)
	{
		TParticleId id = swapIds[i];
		CRY_PFX2_ASSERT(id < m_maxParticles || id == gInvalidId);   // algorithm robustnesses failure
	}
#endif
}

void CParticleContainer::DebugParticleCounters() const
{
#ifdef CRY_DEBUG_PARTICLE_SYSTEM
	#if 0
	size_t totalCount = 0;
	size_t spawncount = 0;
	for (auto& range : m_ranges)
	{
		CRY_PFX2_ASSERT(range.m_firstSpawnedParticle >= range.m_firstParticle);
		CRY_PFX2_ASSERT(range.m_firstSpawnedParticle <= range.m_lastParticle);  // first spawned particles is not within the correct range somehow
		totalCount += range.m_lastParticle - range.m_firstParticle;
		spawncount += range.m_lastParticle - range.m_firstSpawnedParticle;
	}
	CRY_PFX2_ASSERT(totalCount == m_numParticles);          // m_numParticles and ranges particle counts got inconsistent somehow
	CRY_PFX2_ASSERT(spawncount == m_numSpawnedParticles);   // m_numSpawnedParticles and ranges particle counts got inconsistent somehow
	#endif
#endif
}

void CParticleContainer::ResetSpawnedParticles()
{
	CRY_PFX2_PROFILE_DETAIL;

	CRY_PFX2_ASSERT(m_firstSpawnId >= m_lastId);

	const uint numSpawn = GetNumSpawnedParticles();
	const uint gapSize = m_firstSpawnId - m_lastId;
	const uint movingId = m_lastSpawnId - min(gapSize, numSpawn);
	if (gapSize != 0)
	{
		for (size_t dataTypeId = 0; dataTypeId < EPDT_Count; ++dataTypeId)
		{
			const size_t stride = gParticleDataStrides[dataTypeId];
			byte* pBytes = reinterpret_cast<byte*>(m_pData[dataTypeId]);
			if (!pBytes)
				continue;
			memcpy(pBytes + m_lastId * stride, pBytes + movingId * stride, gapSize * stride);
		}
	}

	m_lastId = m_lastSpawnId - gapSize;
	m_firstSpawnId = m_lastSpawnId = m_lastId;
}

void CParticleContainer::RemoveNewBornFlags()
{
	void* pBegin = m_pData[EPDT_State];
	void* pCursor = pBegin;

#ifdef CRY_PFX2_USE_SSE
	const __m128i flag128 = _mm_set1_epi8(~ESB_NewBorn);
	__m128i* pBegin128 = static_cast<__m128i*>(pCursor);
	__m128i* pEnd128 = static_cast<__m128i*>(pBegin) + m_lastId / sizeof(__m128i);
	for (; pBegin128 != pEnd128; ++pBegin128)
		*pBegin128 = _mm_and_si128(*pBegin128, flag128);
	pCursor = pEnd128;
#endif

	const uint8 mask = ~ESB_NewBorn;
	const uint32 flag32 = (mask << 24) | (mask << 16) | (mask << 8) | mask;
	uint32* pBegin32 = static_cast<uint32*>(pCursor);
	uint32* pEnd32 = static_cast<uint32*>(pBegin) + m_lastId / sizeof(uint32);
	for (; pBegin32 != pEnd32; ++pBegin32)
		*pBegin32 &= flag32;
	pCursor = pEnd32;

	uint8* pBegin8 = static_cast<uint8*>(pCursor);
	uint8* pEnd8 = static_cast<uint8*>(pBegin) + m_lastId / sizeof(uint8);
	for (; pBegin8 != pEnd8; ++pBegin8)
		*pBegin8 &= mask;
}

}
