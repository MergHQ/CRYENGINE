// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Renderer/SFConfig.h"
#include <Render/Render_MeshCache.h>
#include <Kernel/SF_Array.h>
#include <Kernel/SF_Debug.h>

namespace Scaleform {
namespace Render {

class CSFRenderer;
class CSFTechniqueManager;

class CSFMesh : public MeshCacheItem
{
public:
	static CSFMesh* Create(MeshType type, MeshCacheListSet* pCacheList, MeshBaseContent& mc,
	                       UPInt vertexAllocSize, unsigned vertexCount, UPInt indexAllocSize, unsigned indexCount)
	{
		UByte* memory = MeshCacheItem::alloc(Scaleform::Memory::GetHeapByAddress(pCacheList->GetCache()), sizeof(CSFMesh), mc);
		return new(memory) CSFMesh(type, pCacheList, mc, vertexAllocSize, vertexCount, indexAllocSize, indexCount);
	}

	CSFMesh(MeshType type, MeshCacheListSet* pcacheList, MeshBaseContent& mc, UPInt vertexAllocSize, unsigned vertexCount, UPInt indexAllocSize, unsigned indexCount) :
		MeshCacheItem(type, pcacheList, mc, sizeof(CSFMesh), vertexAllocSize + indexAllocSize, vertexCount, indexCount),
		m_pVertexBuffer(~0u), m_pIndexBuffer(~0u), m_pMappedData(nullptr), m_pBufferData(nullptr)
	{
	}

	virtual ~CSFMesh() override { Clear(); }

	inline uint32 GetVertexBufferSize() const { return AllocSize - IndexCount * sizeof(IndexType); }
	inline uint32 GetIndexBufferSize() const  { return IndexCount * sizeof(IndexType); }
	inline bool   IsMapped()                  { return nullptr != m_pMappedData; }

	void          Clear();
	void          Map(UByte** pVertexDataStart, IndexType** pIndexDataStart);
	void          Unmap();

private:
	buffer_handle_t m_pVertexBuffer;
	buffer_handle_t m_pIndexBuffer;
	UByte*          m_pMappedData;
	UByte*          m_pBufferData;

	friend class CSFRenderer;
	friend class CSFMeshCache;
};

class CSFMeshCache : public MeshCache
{
public:
	CSFMeshCache(MemoryHeap* pheap, const MeshCacheParams& params)
		: MeshCache(pheap, params)
		, m_cacheList(this)
		, m_quadVertexBuffer(~0u)
		, m_isLocked(false)
		, m_clearCache(false)
	{
	}

	~CSFMeshCache() { Reset(); }

	bool Initialize();

	void Reset()
	{
		m_cacheList.EvictAll();
		StagingBuffer.Reset();
	}

	virtual QueueMode   GetQueueMode() const     { return QM_ExtendLocks; }
	virtual bool        AreBuffersLocked() const { return m_isLocked; }
	virtual bool        SetParams(const MeshCacheParams& argParams);
	virtual UPInt       Evict(MeshCacheItem* pItem, AllocAddr* pAllocator, MeshBase* pSkipMesh);
	virtual void        ClearCache();
	virtual void        EndFrame();
	virtual bool        LockBuffers();
	virtual void        UnlockBuffers();
	virtual void        LockMeshCacheItem(MeshCacheItem* pdata, UByte** pvertexDataStart, IndexType** pindexDataStart);
	virtual void        GetStats(Stats* stats);

	virtual AllocResult AllocCacheItem(
		MeshCacheItem** pdata,
		MeshCacheItem::MeshType meshType,
		MeshCacheItem::MeshBaseContent& mc,
		UPInt vertexBufferSize,
		unsigned vertexCount, unsigned indexCount,
		bool waitForCache, const VertexFormat* pDestFormat);

private:
	void ClampParams(MeshCacheParams* params);

	buffer_handle_t       m_quadVertexBuffer;
	MeshCacheListSet      m_cacheList;
	std::vector<CSFMesh*> m_lockedBuffers;
	bool                  m_isLocked;
	volatile bool         m_clearCache;

	friend class CSFRenderer;
};

} // ~Render namespace
} // ~Scaleform namespace
