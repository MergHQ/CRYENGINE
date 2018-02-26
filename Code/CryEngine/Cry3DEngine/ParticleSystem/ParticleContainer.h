// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     24/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CrySerialization/IArchive.h>
#include "ParticleCommon.h"
#include "ParticleDataTypes.h"
#include "ParticleMath.h"
#include "ParticleDataStreams.h"

namespace pfx2
{

class CParticleContainer;

typedef TIStream<UCol>         IColorStream;
typedef TIStream<uint32>       IUintStream;
typedef TIOStream<uint32>      IOUintStream;
typedef TIStream<TParticleId>  IPidStream;
typedef TIOStream<TParticleId> IOPidStream;

class CParticleContainer
{
public:
	CParticleContainer();
	CParticleContainer(const CParticleContainer& copy);
	~CParticleContainer();

	void                              Resize(uint32 newSize);
	void                              ResetUsedData();
	void                              AddParticleData(EParticleDataType type);
	bool                              HasData(EParticleDataType type) const { return m_useData[type]; }
	void                              AddParticle();
	void                              Trim();
	void                              Clear();

	template<typename T> T*           GetData(EParticleDataType type);
	template<typename T> const T*     GetData(EParticleDataType type) const;
	template<typename T> void         FillData(EParticleDataType type, const T& data, SUpdateRange range, bool allDims = true);
	void                              CopyData(EParticleDataType dstType, EParticleDataType srcType, SUpdateRange range);

	IFStream                          GetIFStream(EParticleDataType type, float defaultVal = 0.0f) const;
	IOFStream                         GetIOFStream(EParticleDataType type);
	IVec3Stream                       GetIVec3Stream(EParticleDataType type, Vec3 defaultVal = Vec3(ZERO)) const;
	IOVec3Stream                      GetIOVec3Stream(EParticleDataType type);
	IQuatStream                       GetIQuatStream(EParticleDataType type, Quat defaultVal = Quat(IDENTITY)) const;
	IOQuatStream                      GetIOQuatStream(EParticleDataType type);
	IColorStream                      GetIColorStream(EParticleDataType type, UCol defaultVal = UCol()) const;
	IOColorStream                     GetIOColorStream(EParticleDataType type);
	IUintStream                       GetIUintStream(EParticleDataType type, uint32 defaultVal = 0) const;
	IOUintStream                      GetIOUintStream(EParticleDataType type);
	IPidStream                        GetIPidStream(EParticleDataType type, TParticleId defaultVal = gInvalidId) const;
	IOPidStream                       GetIOPidStream(EParticleDataType type);
	template<typename T> TIStream<T>  GetTIStream(EParticleDataType type, const T& defaultVal = T()) const;
	template<typename T> TIOStream<T> GetTIOStream(EParticleDataType type);

	bool                              Empty() const                   { return GetNumParticles() == 0; }
	uint32                            GetNumParticles() const         { CRY_ASSERT(!HasNewBorns()); return m_lastId; }
	uint32                            GetRealNumParticles() const     { return m_lastId + HasNewBorns() * GetNumSpawnedParticles(); }
	uint32                            GetMaxParticles() const         { return m_maxParticles; }
	uint32                            GetNumSpawnedParticles() const  { return m_lastSpawnId - m_firstSpawnId; }
	bool                              HasNewBorns() const             { return m_lastSpawnId > m_lastId; }
	bool                              IsNewBorn(TParticleId id) const { return id >= m_firstSpawnId && id < m_lastSpawnId; }
	TParticleId                       GetRealId(TParticleId pId) const;
	SUpdateRange                      GetFullRange() const            { CRY_ASSERT(!HasNewBorns()); return SUpdateRange(0, m_lastId); }
	SUpdateRange                      GetSpawnedRange() const         { return SUpdateRange(m_firstSpawnId, m_lastSpawnId); }
	SUpdateRange                      GetNonSpawnedRange() const      { CRY_ASSERT(!HasNewBorns()); return SUpdateRange(0, m_firstSpawnId); }

	void                              AddParticles(TConstArray<SSpawnEntry> spawnEntries);
	void                              ResetSpawnedParticles();
	void                              RemoveParticles(TVarArray<TParticleId> toRemove, TVarArray<TParticleId> swapIds);

private:
	void                              MakeSwapIds(TVarArray<TParticleId> toRemove, TVarArray<TParticleId> swapIds);
	
	StaticEnumArray<void*, EParticleDataType> m_pData;
	StaticEnumArray<bool, EParticleDataType>  m_useData;
	TParticleId m_nextSpawnId;
	TParticleId m_maxParticles;

	TParticleId m_lastId;
	TParticleId m_firstSpawnId;
	TParticleId m_lastSpawnId;
};

}

#include "ParticleUpdate.h"
#include "ParticleContainerImpl.h"

