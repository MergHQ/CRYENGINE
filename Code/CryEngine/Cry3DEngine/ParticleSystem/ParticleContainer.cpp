// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleContainer.h"

namespace pfx2
{

ElementTypeArray<uint32> UsedMem, AllocMem;

CParticleContainer::CParticleContainer()
{
	static PUseData s_useData = NewUseData();
	SetUsedData(s_useData, EDD_Particle);
}

CParticleContainer::CParticleContainer(const PUseData& pUseData, EDataDomain domain)
{
	SetUsedData(pUseData, domain);
}

CParticleContainer::~CParticleContainer()
{
	FreeData();
}

byte* CParticleContainer::AllocData(uint32 size, uint32 cap)
{
	if (!cap)
		return 0;
	void* ptr = CryModuleMemalign(cap, CRY_PFX2_PARTICLES_ALIGNMENT);
	memset(ptr, 0, cap);
	UsedMem[m_useData.domain] += size;
	AllocMem[m_useData.domain] += cap;
	return (byte*)ptr;
}

void CParticleContainer::FreeData()
{
	if (!m_pData)
		return;
	CryModuleMemalignFree(m_pData);
	UsedMem[m_useData.domain] -= NumElements() * m_useData.totalSize;
	AllocMem[m_useData.domain] -= Capacity() * m_useData.totalSize;
}

void CParticleContainer::MoveData(uint dst, uint src, uint count)
{
	CRY_PFX2_PROFILE_DETAIL;

	for (auto type : EParticleDataType::indices())
	{
		if (byte* pBytes = ByteData(type))
		{
			const size_t stride = type.info().typeSize;
			memcpy(pBytes + dst * stride, pBytes + src * stride, count * stride);
		}
	}
}

void CParticleContainer::Resize(uint32 newSize)
{
	newSize = CRY_PFX2_GROUP_ALIGN(newSize);
	if (newSize <= m_capacity)
		return;

	CRY_PFX2_PROFILE_DETAIL;

	const size_t newCapacity = m_lastId <= CRY_PFX2_GROUP_STRIDE * 2 ? newSize : CRY_PFX2_GROUP_ALIGN(newSize + (newSize >> 1));

	byte* pNew = AllocData(newSize * m_useData.totalSize, newCapacity * m_useData.totalSize);
	if (m_capacity)
	{
		for (auto type : EParticleDataType::indices())
		{
			if (HasData(type))
			{
				uint offset = m_useData.offsets[type];
				const size_t stride = type.info().typeSize;
				memcpy(pNew + offset * newCapacity, m_pData + m_capacity * offset, ExtendedSize() * stride);
			}
		}
	}
	FreeData();
	m_pData = pNew;
	m_capacity = newCapacity;
}

void CParticleContainer::SetUsedData(const PUseData& pUseData, EDataDomain domain)
{
	CRY_PFX2_PROFILE_DETAIL;

	if (m_capacity)
	{
		// Check if changed
		if (pUseData->totalSizes[domain] != m_useData.totalSize
			|| memcmp(pUseData->offsets.data(), m_useData.offsets.data(), m_useData.offsets.size_mem()))
		{
			// Transfer existing data
			byte* pNew = AllocData(ExtendedSize() * pUseData->totalSizes[domain], m_capacity * pUseData->totalSizes[domain]);
			for (auto type : EParticleDataType::indices())
			{
				if (pUseData->Used(type) && m_useData.Used(type))
				{
					memcpy(pNew + m_capacity * pUseData->offsets[type], m_pData + m_capacity * m_useData.offsets[type], ExtendedSize() * type.info().typeSize);
				}
			}
			FreeData();
			m_pData = pNew;
		}
	}
	m_useData = SUseDataRef(pUseData, domain);
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
	FreeData();
	m_pData = nullptr;
	m_capacity = m_lastId = m_firstSpawnId = m_lastSpawnId = m_firstDeadId = m_lastDeadId = m_totalSpawned = 0;
}


namespace detail
{
template<typename T> ILINE void copyElement(T* pData, size_t stride, size_t dst, size_t src)
{
	pData[dst] = pData[src];
}
template<> ILINE void copyElement(char* pData, size_t stride, size_t dst, size_t src)
{
	memcpy(pData + dst * stride, pData + src * stride, stride);
}

template<typename T>
void MoveElements(TConstArray<SMoveElem> toMove, T* pData, size_t stride = 0)
{
	for (auto move : toMove)
	{
		copyElement(pData, stride, move.second, move.first);
	}
}

void AddMove(TDynArray<SMoveElem>& toMove, TParticleId src, TParticleId dst)
{
	if (src != dst)
	{
		if (toMove.size() && toMove.back().second == src)
			toMove.back().second = dst;
		else
			toMove.emplace_back(src, dst);
	}
}

void RemoveElements(TConstArray<TParticleId> toRemove, TDynArray<SMoveElem>& toMove, TParticleId& endId)
{
	uint pos;
	for (pos = toRemove.size(); pos-- > 0; )
	{
		AddMove(toMove, --endId, toRemove[pos]);
	}
}

void InitSwapIds(TVarArray<TParticleId> swapIds)
{
	for (TParticleId id = 0; id < swapIds.size(); ++id)
		swapIds[id] = id;
}

}

void CParticleContainer::BeginSpawn()
{
	m_firstSpawnId = m_lastSpawnId = m_lastId;
}

bool CParticleContainer::AddNeedsMove(uint count) const
{
	if (!count || m_lastDeadId == m_firstDeadId)
		return false;

	TParticleId newfirstSpawn = max(m_firstSpawnId, CRY_PFX2_GROUP_ALIGN(m_lastId));
	TParticleId newLastSpawn = newfirstSpawn + count;

	return newLastSpawn > m_firstDeadId;
}

void CParticleContainer::AddElement()
{
	AddElements(1);
	EndSpawn();
}

void CParticleContainer::AddElements(uint count, TVarArray<TParticleId> swapIds)
{
	if (!count)
		return;

	CRY_PFX2_PROFILE_DETAIL;

	// Resize for added elements
	if (m_lastSpawnId <= m_lastId)
		m_firstSpawnId = m_lastSpawnId = CRY_PFX2_GROUP_ALIGN(m_lastId);
	m_lastSpawnId += count;

	uint deadCount = DeadRange().size();
	if (AddNeedsMove(count))
	{
		CRY_ASSERT(swapIds.size() >= ExtendedSize());
		uint newDead = max(m_firstDeadId, CRY_PFX2_GROUP_ALIGN(m_lastSpawnId));
		uint newSize = newDead + deadCount;

		Resize(newSize);

		// Move dead elements out of added range
		uint moveEnd = min(newDead, m_lastDeadId);
		uint moveDest = max(newDead, m_lastDeadId);
		uint moveCount = moveEnd - m_firstDeadId;

		MoveData(moveDest, m_firstDeadId, moveCount);

		detail::InitSwapIds(swapIds);
		while (m_firstDeadId < moveEnd)
			swapIds[m_firstDeadId++] = moveDest++;

		m_firstDeadId = newDead;
		m_lastDeadId = newSize;
	}
	else
	{
		Resize(m_lastSpawnId);
		if (!deadCount)
			m_firstDeadId = m_lastDeadId = m_lastSpawnId;
	}

	m_totalSpawned += count;
}

void CParticleContainer::RemoveElements(TConstArray<TParticleId> toRemove, TConstArray<bool> doPreserve, TVarArray<TParticleId> swapIds)
{
	CRY_PFX2_PROFILE_DETAIL;
	CRY_ASSERT(!HasNewBorns());

	// Create move schema
	TDynArray<detail::SMoveElem> toMove;

	uint archiveCount = 0;
	if (doPreserve.size())
	{
		CRY_ASSERT(doPreserve.size() >= ExtendedSize());

		// Remove unpreserved dead elements
		for (TParticleId id = m_lastDeadId; id-- > m_firstDeadId; )
		{
			if (!doPreserve[id])
				detail::AddMove(toMove, --m_lastDeadId, id);
		}

		// Count archived elements
		for (TParticleId id : toRemove)
			archiveCount += doPreserve[id];

		if (archiveCount)
		{
			// Expand for archived elements
			if (m_lastDeadId == m_firstDeadId)
				m_firstDeadId = m_lastDeadId = CRY_PFX2_GROUP_ALIGN(m_lastId);
			m_lastDeadId += archiveCount;
			Resize(m_lastDeadId);
		}
	}

	// Remove live elements, swapping from end
	for (TParticleId archiveDest = m_lastDeadId; toRemove.size(); toRemove.erase_back())
	{
		TParticleId id = toRemove.back();
		CRY_ASSERT(IdIsAlive(id));
		if (doPreserve.size() && doPreserve[id])
			detail::AddMove(toMove, id, --archiveDest);
		detail::AddMove(toMove, --m_lastId, id);
	}

	if (m_firstDeadId == m_lastDeadId)
		m_firstDeadId = m_lastDeadId = m_lastId;
	m_firstSpawnId = m_lastSpawnId = m_lastId;

	MoveData(toMove, swapIds);
}

void CParticleContainer::Reparent(TConstArray<TParticleId> swapIds, TDataType<TParticleId> parentType)
{
	CRY_PFX2_PROFILE_DETAIL;

	auto parentIds = IOStream(parentType);
	for (auto id : FullRange())
	{
		TParticleId& parentId = parentIds[id];
		if (parentId != gInvalidId)
			parentId = swapIds[parentId];
	}
	for (auto id : DeadRange())
	{
		TParticleId& parentId = parentIds[id];
		if (parentId != gInvalidId)
			parentId = swapIds[parentId];
	}
}

void CParticleContainer::EndSpawn()
{
	CRY_PFX2_PROFILE_DETAIL;

	if (m_firstSpawnId < m_lastId)
		return;

	const uint gapSize = m_firstSpawnId - m_lastId;
	if (gapSize != 0)
	{
		const uint numSpawn = NumSpawned();
		const uint movingId = m_lastSpawnId - min(gapSize, numSpawn);
		MoveData(m_lastId, movingId, gapSize);
		m_lastSpawnId -= gapSize;
		m_firstSpawnId -= gapSize;
	}

	m_lastId = m_lastSpawnId;
}

void CParticleContainer::MoveData(TConstArray<detail::SMoveElem> toMove, TVarArray<TParticleId> swapIds)
{
	if (!swapIds.empty())
	{
		// Update from element moves
		TDynArray<TParticleId> ids(max(swapIds.size(), ExtendedSize()));
		detail::InitSwapIds(ids);

		for (auto move : toMove)
			ids[move.second] = ids[move.first];

		// Create swapIds from current ids
		swapIds.fill(gInvalidId);
		for (auto i : FullRange())
		{
			TParticleId v0 = ids[i];
			swapIds[v0] = i;
		}
		for (auto i : DeadRange())
		{
			TParticleId v0 = ids[i];
			swapIds[v0] = i;
		}
	}

	// Now move
	if (!toMove.empty())
	{
		for (auto dataTypeId : EParticleDataType::indices())
		{
			byte* pData = ByteData(dataTypeId);
			if (!pData)
				continue;
			const uint stride = dataTypeId.info().typeSize;
			switch (stride)
			{
			case 1:
				detail::MoveElements(toMove, (uint8*)pData);
				break;
			case 2:
				detail::MoveElements(toMove, (uint16*)pData);
				break;
			case 4:
				detail::MoveElements(toMove, (uint32*)pData);
				break;
			case 8:
				detail::MoveElements(toMove, (uint64*)pData);
				break;
			default:
				detail::MoveElements(toMove, (char*)pData, stride);
			}
		}
	}
}

}
