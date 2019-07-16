// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Renderer/SFMesh.h"
#include "Renderer/SFTechnique.h"
#include <Kernel/SF_Debug.h>
#include <Kernel/SF_Alg.h>
#include <Kernel/SF_HeapNew.h>

namespace Scaleform {
namespace Render {

void CSFMesh::Clear()
{
	CDeviceBufferManager& DevBufMan = ((CD3D9Renderer*)gEnv->pRenderer)->m_DevBufMan;
	if (~0u != m_pVertexBuffer)
	{
		DevBufMan.Destroy(m_pVertexBuffer);
	}

	if (~0u != m_pIndexBuffer)
	{
		DevBufMan.Destroy(m_pIndexBuffer);
	}

	if (m_pBufferData)
	{
		SF_FREE_ALIGN(m_pBufferData);
	}

	m_pVertexBuffer = ~0u;
	m_pIndexBuffer = ~0u;
	m_pBufferData = nullptr;
}

void CSFMesh::Map(UByte** pVertexDataStart, IndexType** pIndexDataStart)
{
	if (!m_pMappedData)
	{
		if (!m_pBufferData)
		{
			m_pBufferData = (UByte*)SF_MEMALIGN(AllocSize, CRY_PLATFORM_ALIGNMENT, StatRender_MeshStaging_Mem);
		}
		m_pMappedData = m_pBufferData;
	}
	*pVertexDataStart = m_pMappedData;
	*pIndexDataStart = (IndexType*)(m_pMappedData + GetVertexBufferSize());
}

void CSFMesh::Unmap()
{
	CRY_ASSERT(m_pMappedData);
	if (!m_pMappedData)
	{
		return;
	}

	const uint32 vbSize = GetVertexBufferSize();
	const uint32 ibSize = GetIndexBufferSize();
	CDeviceBufferManager& DevBufMan = ((CD3D9Renderer*)gEnv->pRenderer)->m_DevBufMan;

	if (vbSize)
	{
		if (~0u == m_pVertexBuffer)
		{
			m_pVertexBuffer = DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, vbSize);
		}

		if (~0u != m_pVertexBuffer)
		{
			DevBufMan.UpdateBuffer(m_pVertexBuffer, m_pMappedData, vbSize);
		}
	}

	if (ibSize)
	{
		if (~0u == m_pIndexBuffer)
		{
			m_pIndexBuffer = DevBufMan.Create(BBT_INDEX_BUFFER, BU_STATIC, ibSize);
		}

		if (~0u != m_pIndexBuffer)
		{
			DevBufMan.UpdateBuffer(m_pIndexBuffer, m_pMappedData + vbSize, ibSize);
		}
	}

	m_pMappedData = nullptr;
}

void CSFMeshCache::ClampParams(MeshCacheParams* p)
{
	p->MaxBatchInstances = Alg::Clamp<unsigned>(p->MaxBatchInstances, 1, SF_RENDER_MAX_BATCHES);
	p->VBLockEvictSizeLimit = Alg::Clamp<UPInt>(p->VBLockEvictSizeLimit, 0, p->VBLockEvictSizeLimit = 1024 * 256);

	UPInt maxStagingItemSize = p->MaxVerticesSizeInBatch + sizeof(UInt16) * p->MaxIndicesInBatch;
	if (maxStagingItemSize * 2 > p->StagingBufferSize)
	{
		p->StagingBufferSize = maxStagingItemSize * 2;
	}
}

bool CSFMeshCache::Initialize()
{
	ClampParams(&Params);

	if (!StagingBuffer.Initialize(pHeap, Params.StagingBufferSize))
	{
		return false;
	}

	const unsigned bufferSize = sizeof(VertexXY16fAlpha) * 6 * SF_RENDER_MAX_BATCHES;
	VertexXY16fAlpha pbuffer[6 * SF_RENDER_MAX_BATCHES];
	fillMaskEraseVertexBuffer<VertexXY16fAlpha>(pbuffer, SF_RENDER_MAX_BATCHES);

	CDeviceBufferManager& DevBufMan = ((CD3D9Renderer*)gEnv->pRenderer)->m_DevBufMan;
	m_quadVertexBuffer = DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, bufferSize);
	DevBufMan.UpdateBuffer(m_quadVertexBuffer, pbuffer, bufferSize);
	return ~0u != m_quadVertexBuffer;
}

bool CSFMeshCache::SetParams(const MeshCacheParams& argParams)
{
	MeshCacheParams params(argParams);
	ClampParams(&params);
	m_cacheList.EvictAll();

	if (Params.StagingBufferSize != params.StagingBufferSize)
	{
		if (!StagingBuffer.Initialize(pHeap, params.StagingBufferSize))
		{
			StagingBuffer.Initialize(pHeap, Params.StagingBufferSize);
			return false;
		}
	}
	Params = params;
	return true;
}

void CSFMeshCache::ClearCache()
{
	if (((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread())
	{
		Reset();
		StagingBuffer.Initialize(pHeap, Params.StagingBufferSize);
	}
	else
	{
		m_clearCache = true;
	}
}

void CSFMeshCache::EndFrame()
{
	m_cacheList.EndFrame();
	if (m_clearCache)
	{
		m_clearCache = false;
		ClearCache();
	}
}

UPInt CSFMeshCache::Evict(MeshCacheItem* pItem, AllocAddr* pAllocator, MeshBase* pSkipMesh)
{
	CSFMesh* pBatch = (CSFMesh*)pItem;
	if (!pBatch->IsPending(FenceType_Vertex))
	{
		pBatch->Clear();
		pBatch->destroy(pSkipMesh, true);
		return pBatch->AllocSize;
	}
	else
	{
		pBatch->destroy(pSkipMesh, false);
		m_cacheList.PushFront(MCL_PendingFree, pBatch);
		return 0;
	}
}

CSFMeshCache::AllocResult CSFMeshCache::AllocCacheItem(
	MeshCacheItem** pdata,
	MeshCacheItem::MeshType meshType,
	MeshCacheItem::MeshBaseContent& mc,
	UPInt vertexBufferSize,
	unsigned vertexCount, unsigned indexCount,
	bool waitForCache, const VertexFormat* pDestFormat)
{
	if (!AreBuffersLocked() && !LockBuffers())
	{
		return Alloc_StateError;
	}

	*pdata = CSFMesh::Create(meshType, &m_cacheList, mc, vertexBufferSize, vertexCount, indexCount * sizeof(IndexType), indexCount);

	if (!*pdata)
	{
		CRY_ASSERT(0);
		return Alloc_Fail;
	}
	return Alloc_Success;
}

bool CSFMeshCache::LockBuffers()
{
	CRY_ASSERT(!m_isLocked);
	m_isLocked = true;

	if (pRQCaches)
	{
		pRQCaches->SetCacheLocked(Cache_Mesh);
	}
	return true;
}

void CSFMeshCache::UnlockBuffers()
{
	CRY_ASSERT(m_isLocked != 0);

	for (CSFMesh* pMeshCacheItem : m_lockedBuffers)
	{
		pMeshCacheItem->Unmap();
	}
	m_lockedBuffers.clear();
	m_isLocked = false;

	if (pRQCaches)
	{
		pRQCaches->ClearCacheLocked(Cache_Mesh);
	}
}

void CSFMeshCache::LockMeshCacheItem(MeshCacheItem* pdata, UByte** pvertexDataStart, IndexType** pindexDataStart)
{
	CSFMesh* pMeshCacheItem = static_cast<CSFMesh*>(pdata);
	CRY_ASSERT(pMeshCacheItem);
	if (!pMeshCacheItem)
	{
		return;
	}

	if (!pMeshCacheItem->IsMapped())
	{
		m_lockedBuffers.push_back(pMeshCacheItem);
	}
	pMeshCacheItem->Map(pvertexDataStart, pindexDataStart);
}

void CSFMeshCache::GetStats(Stats* stats)
{
	*stats = Stats();
	for (int32 i = 0; i < MCL_ItemCount; ++i)
	{
		MeshCacheListSet::ListSlot& slot = m_cacheList.GetSlot((MeshCacheListType)i);
		stats->TotalSize[MeshBuffer_Common] += slot.Size;
		stats->TotalSize[MeshBuffer_GpuMem + MeshBuffer_Common] += slot.Size;
	}
}

} // ~Render namespace
} // ~Scaleform namespace
