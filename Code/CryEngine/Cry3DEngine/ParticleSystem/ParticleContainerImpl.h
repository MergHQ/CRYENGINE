// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     21/10/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLECONTAINERIMPL_H
#define PARTICLECONTAINERIMPL_H

#pragma once

namespace pfx2
{

//////////////////////////////////////////////////////////////////////////
// CParticleContainer

ILINE TParticleId CParticleContainer::GetRealId(TParticleId pId) const
{
	if (pId < m_lastId)
		return pId;
	const uint numSpawn = GetNumSpawnedParticles();
	const uint gapSize = m_firstSpawnId - m_lastId;
	const uint movingId = m_lastSpawnId - min(gapSize, numSpawn);
	const uint offset = movingId - m_lastId;
	if (pId >= movingId)
		return pId - offset;
	return pId;
}

template<EParticleDataType type, typename T>
ILINE void CParticleContainer::FillData(const SUpdateContext& context, const T& data)
{
	CRY_PFX2_ASSERT(sizeof(T) == gParticleDataStrides[type]);
	if (HasData(type))
	{
		T* pBuffer = reinterpret_cast<T*>(m_pData[type]);
		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		pBuffer[particleId] = data;
		CRY_PFX2_FOR_END;
	}
}

template<EParticleDataType type, typename T>
ILINE void CParticleContainer::FillSpawnedData(const SUpdateContext& context, const T& data)
{
	// PFX2_TODO : the way it is, this function cannot be vectorized
	CRY_PFX2_ASSERT(sizeof(T) == gParticleDataStrides[type]);
	if (HasData(type))
	{
		T* pBuffer = reinterpret_cast<T*>(m_pData[type]);
		CRY_PFX2_FOR_SPAWNED_PARTICLES(context)
		pBuffer[particleId] = data;
		CRY_PFX2_FOR_END;
	}
}

template<EParticleDataType dstType, EParticleDataType srcType>
ILINE void CParticleContainer::CopyData(const SUpdateContext& context)
{
	CRY_PFX2_ASSERT(gParticleDataStrides[dstType] == gParticleDataStrides[srcType]);
	if (HasData(dstType) && HasData(srcType))
	{
		size_t stride = gParticleDataStrides[dstType];
		size_t count = context.m_updateRange.m_lastParticleId - context.m_updateRange.m_firstParticleId;
		memcpy(
		  (byte*)m_pData[dstType] + stride * context.m_updateRange.m_firstParticleId,
		  (byte*)m_pData[srcType] + stride * context.m_updateRange.m_firstParticleId,
		  stride * count);
	}
}

ILINE IFStream CParticleContainer::GetIFStream(EParticleDataType type, float defaultVal) const
{
	return IFStream(reinterpret_cast<const float*>(GetData(type)), defaultVal);
}

ILINE IOFStream CParticleContainer::GetIOFStream(EParticleDataType type)
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(float));
	return IOFStream(reinterpret_cast<float*>(GetData(type)));
}

ILINE IVec3Stream CParticleContainer::GetIVec3Stream(EParticleVec3Field type, Vec3 defaultVal) const
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	CRY_PFX2_ASSERT(gParticleDataStrides[xType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[yType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[zType] == sizeof(float));
	return IVec3Stream(
	  reinterpret_cast<const float*>(GetData(xType)),
	  reinterpret_cast<const float*>(GetData(yType)),
	  reinterpret_cast<const float*>(GetData(zType)),
	  defaultVal);
}

ILINE IOVec3Stream CParticleContainer::GetIOVec3Stream(EParticleVec3Field type)
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	CRY_PFX2_ASSERT(gParticleDataStrides[xType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[yType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[zType] == sizeof(float));
	return IOVec3Stream(
	  reinterpret_cast<float*>(GetData(xType)),
	  reinterpret_cast<float*>(GetData(yType)),
	  reinterpret_cast<float*>(GetData(zType)));
}

ILINE IQuatStream CParticleContainer::GetIQuatStream(EParticleQuatField type, Quat defaultVal) const
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	EParticleDataType wType = EParticleDataType(type + 3);
	CRY_PFX2_ASSERT(gParticleDataStrides[xType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[yType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[zType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[wType] == sizeof(float));
	return IQuatStream(
	  reinterpret_cast<const float*>(GetData(xType)),
	  reinterpret_cast<const float*>(GetData(yType)),
	  reinterpret_cast<const float*>(GetData(zType)),
	  reinterpret_cast<const float*>(GetData(wType)),
	  defaultVal);
}

ILINE IOQuatStream CParticleContainer::GetIOQuatStream(EParticleQuatField type)
{
	EParticleDataType xType = EParticleDataType(type);
	EParticleDataType yType = EParticleDataType(type + 1);
	EParticleDataType zType = EParticleDataType(type + 2);
	EParticleDataType wType = EParticleDataType(type + 3);
	CRY_PFX2_ASSERT(gParticleDataStrides[xType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[yType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[zType] == sizeof(float));
	CRY_PFX2_ASSERT(gParticleDataStrides[wType] == sizeof(float));
	return IOQuatStream(
	  reinterpret_cast<float*>(GetData(xType)),
	  reinterpret_cast<float*>(GetData(yType)),
	  reinterpret_cast<float*>(GetData(zType)),
	  reinterpret_cast<float*>(GetData(wType)));
}

ILINE IColorStream CParticleContainer::GetIColorStream(EParticleDataType type) const
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(UCol));
	return IColorStream(reinterpret_cast<const UCol*>(GetData(type)));
}

ILINE IOColorStream CParticleContainer::GetIOColorStream(EParticleDataType type)
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(UCol));
	return IOColorStream(reinterpret_cast<UCol*>(GetData(type)));
}

ILINE IUintStream CParticleContainer::GetIUintStream(EParticleDataType type, uint32 defaultVal) const
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(uint));
	return IUintStream(reinterpret_cast<const uint*>(GetData(type)), defaultVal);
}

ILINE IOUintStream CParticleContainer::GetIOUintStream(EParticleDataType type)
{
	return GetTIOStream<uint32>(type);
}

ILINE IPidStream CParticleContainer::GetIPidStream(EParticleDataType type, TParticleId defaultVal) const
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(TParticleId));
	return IPidStream(reinterpret_cast<const TParticleId*>(GetData(type)), defaultVal);
}

ILINE IOPidStream CParticleContainer::GetIOPidStream(EParticleDataType type)
{
	return GetTIOStream<TParticleId>(type);
}

template<typename T>
ILINE TIStream<T> CParticleContainer::GetTIStream(EParticleDataType type, const T& defaultVal) const
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(T));
	return TIStream<T>(reinterpret_cast<const T*>(GetData(type)), defaultVal);
}

template<typename T>
ILINE TIOStream<T> CParticleContainer::GetTIOStream(EParticleDataType type)
{
	CRY_PFX2_ASSERT(gParticleDataStrides[type] == sizeof(T));
	return TIOStream<T>(reinterpret_cast<T*>(GetData(type)));
}

ILINE SUpdateRange CParticleContainer::GetFullRange() const
{
	SUpdateRange range;
	range.m_firstParticleId = 0;
	range.m_lastParticleId = GetLastParticleId();
	return range;
}

ILINE SUpdateRange CParticleContainer::GetSpawnedRange() const
{
	SUpdateRange range;
	range.m_firstParticleId = GetFirstSpawnParticleId();
	range.m_lastParticleId = GetLastParticleId();
	return range;
}

}

#endif
