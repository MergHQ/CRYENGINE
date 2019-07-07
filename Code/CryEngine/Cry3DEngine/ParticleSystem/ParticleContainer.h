// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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

namespace detail
{
using SMoveElem = std::pair<TParticleId, TParticleId>;
}

class CParticleContainer
{
public:
	CParticleContainer();
	CParticleContainer(const PUseData& pUseData, EDataDomain domain);
	CParticleContainer(const CParticleContainer& copy) = delete;
	~CParticleContainer();

	void                              Resize(uint32 newSize);
	void                              SetUsedData(const PUseData& pUseData, EDataDomain domain);
	bool                              HasData(EParticleDataType type) const { return m_useData.Used(type); }
	void                              AddElement();
	void                              Clear();
	template<typename T> void         FillData(TDataType<T> type, const T& data, SUpdateRange range);
	void                              CopyData(EParticleDataType dstType, EParticleDataType srcType, SUpdateRange range);

	template<typename T> TIStream<T>  IStream(TDataType<T> type, const T& defaultVal = T()) const { return TIStream<T>(Data(type), AlignedSize(), defaultVal);	}
	template<typename T> TIOStream<T> IOStream(TDataType<T> type) 	                              { return TIOStream<T>(Data(type), AlignedSize());	}

	TIStream<Vec3>                    IStream(TDataType<Vec3> type, Vec3 defaultVal = Vec3(0)) const;
	TIOStream<Vec3>                   IOStream(TDataType<Vec3> type);
	TIStream<Quat>                    IStream(TDataType<Quat> type, Quat defaultVal = Quat(IDENTITY)) const;
	TIOStream<Quat>                   IOStream(TDataType<Quat> type);

	// Legacy stream functions
	IFStream                          GetIFStream(TDataType<float> type, float defaultVal = 0.0f) const                      { return IStream(type, defaultVal); }
	IOFStream                         GetIOFStream(TDataType<float> type)                                                    { return IOStream(type); }
	IVec3Stream                       GetIVec3Stream(TDataType<Vec3> type, Vec3 defaultVal = Vec3(ZERO)) const               { return IStream(type, defaultVal); }
	IOVec3Stream                      GetIOVec3Stream(TDataType<Vec3> type)                                                  { return IOStream(type); }
	IQuatStream                       GetIQuatStream(TDataType<Quat> type, Quat defaultVal = Quat(IDENTITY)) const           { return IStream(type, defaultVal); }
	IOQuatStream                      GetIOQuatStream(TDataType<Quat> type)                                                  { return IOStream(type); }
	IColorStream                      GetIColorStream(TDataType<UCol> type, UCol defaultVal = UCol()) const                  { return IStream(type, defaultVal); }
	IOColorStream                     GetIOColorStream(TDataType<UCol> type)                                                 { return IOStream(type); }
	IPidStream                        GetIPidStream(TDataType<TParticleId> type, TParticleId defaultVal = gInvalidId) const  { return IStream(type, defaultVal); }
	IOPidStream                       GetIOPidStream(TDataType<TParticleId> type)                                            { return IOStream(type); }

	uint32                            Size() const                    { CRY_PFX2_ASSERT(!HasNewBorns()); return m_lastId; }
	uint32                            RealSize() const                { return m_lastId + HasNewBorns() * NumSpawned(); }
	uint32                            ExtendedSize() const            { return m_lastDeadId;}
	uint32                            AlignedSize() const             { return CRY_PFX2_GROUP_ALIGN(m_lastDeadId); }
	uint32                            NumElements() const             { return RealSize() + DeadRange().size(); }
	uint32                            Capacity() const                { return m_capacity; }
	uint32                            LastSpawned() const             { return m_lastSpawnId; }
	uint32                            NumSpawned() const              { return m_lastSpawnId - m_firstSpawnId; }
	bool                              HasNewBorns() const             { return m_lastSpawnId > m_lastId; }
	bool                              IsNewBorn(TParticleId id) const { return id >= m_firstSpawnId && id < m_lastSpawnId; }
	TParticleId                       RealId(TParticleId pId) const;
	SUpdateRange                      FullRange() const               { CRY_PFX2_ASSERT(!HasNewBorns()); return SUpdateRange(0, m_lastId); }
	SUpdateRange                      SpawnedRange() const            { return SUpdateRange(m_firstSpawnId, m_lastSpawnId); }
	SUpdateRange                      NonSpawnedRange() const         { return SUpdateRange(0, min(m_lastId, m_firstSpawnId)); }
	SUpdateRange                      DeadRange() const               { return SUpdateRange(m_firstDeadId, m_lastDeadId); }
	uint32                            TotalSpawned() const            { return m_totalSpawned; }
	uint32                            GetSpawnIdOffset() const        { return m_totalSpawned - m_lastSpawnId; }
	bool                              IdIsAlive(TParticleId id) const { return id < m_lastId; }
	bool                              IdIsDead(TParticleId id) const  { return id >= m_firstDeadId && id < m_lastDeadId; }
	bool                              IdIsValid(TParticleId id) const { return IdIsAlive(id) || IdIsDead(id); }

	void                              BeginSpawn();
	bool                              AddNeedsMove(uint count) const;
	void                              AddElements(uint count, TVarArray<TParticleId> swapIds = {});
	void                              EndSpawn();
	void                              RemoveElements(TConstArray<TParticleId> toRemove, TConstArray<bool> doPreserve = {}, TVarArray<TParticleId> swapIds = {});
	void                              Reparent(TConstArray<TParticleId> swapIds, TDataType<TParticleId> parentType);

	// Old names	
	uint32                            GetNumParticles() const { return Size(); }
	uint32                            GetMaxParticles() const { return Capacity(); }

private:
	byte*                             ByteData(EParticleDataType type) const { return HasData(type) ? m_pData + m_useData.offsets[type] * m_capacity : nullptr; }
	template<typename T> const T*     Data(TDataType<T> type) const          { return reinterpret_cast<const T*>(ByteData(type)); }
	template<typename T> T*           Data(TDataType<T> type)                { return reinterpret_cast<T*>(ByteData(type)); }

	byte*                             AllocData(uint cap, uint size);
	void                              FreeData();
	void                              MoveData(uint dst, uint src, uint count);
	void                              MoveData(TConstArray<detail::SMoveElem> toMove, TVarArray<TParticleId> swapIds);

	TParticleId m_lastId       = 0;
	TParticleId m_firstSpawnId = 0;
	TParticleId m_lastSpawnId  = 0;
	TParticleId m_firstDeadId  = 0;
	TParticleId m_lastDeadId   = 0;
	TParticleId m_capacity     = 0;
	TParticleId m_totalSpawned = 0;

	SUseDataRef m_useData;
	byte*       m_pData        = 0;
};

}

#include "ParticleContainerImpl.h"

