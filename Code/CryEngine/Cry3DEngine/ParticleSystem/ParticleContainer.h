// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     24/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLECONTAINER_H
#define PARTICLECONTAINER_H

#pragma once

#include <CrySerialization/IArchive.h>
#include "ParticleCommon.h"
#include "ParticleDataTypes.h"
#include "ParticleMath.h"
#include "ParticleUpdate.h"

namespace pfx2
{

class CParticleContainer;

typedef TIStream<UCol, UColv>               IColorStream;
typedef TIStream<uint32, uint32v>           IUintStream;
typedef TIOStream<uint32>                   IOUintStream;
typedef TIStream<TParticleId, TParticleIdv> IPidStream;
typedef TIOStream<TParticleId>              IOPidStream;

class CParticleContainer
{
public:
	struct SParticleRangeIterator;

	struct SSpawnEntry
	{
		uint32 m_count;
		uint32 m_parentId;
		float  m_ageBegin;
		float  m_ageIncrement;
		float  m_fractionBegin;
		float  m_fractionCounter;
	};

public:
	CParticleContainer();
	CParticleContainer(const CParticleContainer& copy);
	~CParticleContainer();

	void                              Resize(size_t newSize);
	void                              ResetUsedData();
	void                              AddParticleData(EParticleDataType type);
	void                              AddParticleData(EParticleVec3Field type);
	void                              AddParticleData(EParticleQuatField type);
	bool                              HasData(EParticleDataType type) const { return m_useData[type]; }
	void                              AddParticle();
	void                              AddRemoveParticles(const SSpawnEntry* pSpawnEntries, size_t numSpawnEntries, const TParticleIdArray* pToRemove, TParticleIdArray* pSwapIds);
	void                              Trim();
	void                              Clear();

	void*                             GetData(EParticleDataType type)       { return m_pData[type]; }
	const void*                       GetData(EParticleDataType type) const { return m_pData[type]; }
	template<EParticleDataType type, typename T>
	void                              FillData(const SUpdateContext& context, const T& data);
	template<EParticleDataType type, typename T>
	void                              FillSpawnedData(const SUpdateContext& context, const T& data);
	template<EParticleDataType dstType, EParticleDataType srcType>
	void                              CopyData(const SUpdateContext& context);

	IFStream                          GetIFStream(EParticleDataType type, float defaultVal = 0.0f) const;
	IOFStream                         GetIOFStream(EParticleDataType type);
	IVec3Stream                       GetIVec3Stream(EParticleVec3Field type, Vec3 defaultVal = Vec3(ZERO)) const;
	IOVec3Stream                      GetIOVec3Stream(EParticleVec3Field type);
	IQuatStream                       GetIQuatStream(EParticleQuatField type, Quat defaultVal = Quat(IDENTITY)) const;
	IOQuatStream                      GetIOQuatStream(EParticleQuatField type);
	IColorStream                      GetIColorStream(EParticleDataType type) const;
	IOColorStream                     GetIOColorStream(EParticleDataType type);
	IUintStream                       GetIUintStream(EParticleDataType type, uint32 defaultVal = 0) const;
	IOUintStream                      GetIOUintStream(EParticleDataType type);
	IPidStream                        GetIPidStream(EParticleDataType type, TParticleId defaultVal = gInvalidId) const;
	IOPidStream                       GetIOPidStream(EParticleDataType type);
	template<typename T> TIStream<T>  GetTIStream(EParticleDataType type, const T& defaultVal = T()) const;
	template<typename T> TIOStream<T> GetTIOStream(EParticleDataType type);

	bool                              Empty() const                   { return GetNumParticles() == 0; }
	uint32                            GetNumParticles() const         { return m_lastId + GetNumSpawnedParticles(); }
	bool                              HasSpawnedParticles() const     { return GetNumSpawnedParticles() != 0; }
	TParticleId                       GetFirstSpawnParticleId() const { return TParticleId(m_firstSpawnId); }
	TParticleId                       GetLastParticleId() const       { return TParticleId(m_lastSpawnId); }
	size_t                            GetMaxParticles() const         { return m_maxParticles; }
	uint32                            GetNumSpawnedParticles() const  { return m_lastSpawnId - m_firstSpawnId; }
	void                              ResetSpawnedParticles();
	void                              RemoveNewBornFlags();
	TParticleId                       GetRealId(TParticleId pId) const;

	SUpdateRange                      GetFullRange() const;
	SUpdateRange                      GetSpawnedRange() const;

private:
	void DebugRemoveParticlesStability(const TParticleIdArray& toRemove, const TParticleIdArray& swapIds) const;
	void DebugParticleCounters() const;

	void*  m_pData[EPDT_Count];
	bool   m_useData[EPDT_Count];
	uint32 m_nextSpawnId;
	uint32 m_maxParticles;

	uint32 m_lastId;
	uint32 m_firstSpawnId;
	uint32 m_lastSpawnId;
};

}

#include "ParticleContainerImpl.h"

#endif // PARTICLECONTAINER_H
