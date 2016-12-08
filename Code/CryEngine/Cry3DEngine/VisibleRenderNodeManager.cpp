// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "3dEngine.h"

//////////////////////////////////////////////////////////////////////////
void SRenderNodeTempData::Free()
{
	FreeRenderObjects();

	if (userData.m_pFoliage)
	{
		userData.m_pFoliage->Release();
		userData.m_pFoliage = NULL;
	}

	userData.pOwnerNode = nullptr;
	userData.bToDelete = true;
}

//////////////////////////////////////////////////////////////////////////
void SRenderNodeTempData::FreeRenderObjects()
{
	// Release permanent CRenderObject(s)
	for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
	{
		if (userData.arrPermanentRenderObjects[lod])
		{
			gEnv->pRenderer->EF_FreeObject(userData.arrPermanentRenderObjects[lod]);
			userData.arrPermanentRenderObjects[lod] = 0;
		}
	}
}

void SRenderNodeTempData::InvalidateRenderObjectsInstanceData()
{
	// Release permanent CRenderObject(s)
	for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
	{
		if (userData.arrPermanentRenderObjects[lod])
		{
			userData.arrPermanentRenderObjects[lod]->m_bInstanceDataDirty = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CVisibleRenderNodesManager::CVisibleRenderNodesManager()
	: m_lastStartUpdateNode(0)
	, m_currentNodesToDelete(0)
	, m_lastUpdateFrame(0)
{

}

CVisibleRenderNodesManager::~CVisibleRenderNodesManager()
{
	ClearAll();
}

SRenderNodeTempData* CVisibleRenderNodesManager::AllocateTempData(int lastSeenFrame)
{
	SRenderNodeTempData* pData = m_pool.New();

	{
		CryAutoCriticalSectionNoRecursive lock(m_accessLock);
		m_visibleNodes.push_back(pData);
	}
	return pData;
}

//////////////////////////////////////////////////////////////////////////
bool CVisibleRenderNodesManager::SetLastSeenFrame(SRenderNodeTempData* pTempData, const SRenderingPassInfo& passInfo)
{
	int frame = passInfo.GetFrameID();
	bool bCanRenderThisFrame = true;

	if (passInfo.IsShadowPass())
	{
		pTempData->userData.lastSeenShadowFrame = frame;
	}
	else
	{
		int recursion = passInfo.IsRecursivePass() ? 1 : 0;
		// Only return true if last seen frame is different form the current
		bCanRenderThisFrame = pTempData->userData.lastSeenFrame[recursion] != frame;
		pTempData->userData.lastSeenFrame[recursion] = frame;
	}
	return bCanRenderThisFrame;
}

//////////////////////////////////////////////////////////////////////////
void CVisibleRenderNodesManager::UpdateVisibleNodes(int currentFrame, int maxNodesToCheck)
{
	if (m_lastUpdateFrame == currentFrame) // Fast exit if happens on the same frame
		return;
	m_lastUpdateFrame = currentFrame;

	FUNCTION_PROFILER_3DENGINE;

	assert(gEnv->mMainThreadId == CryGetCurrentThreadId());

	m_currentNodesToDelete++;
	m_currentNodesToDelete = (m_currentNodesToDelete) % MAX_DELETE_BUFFERS; // Always cycle delete buffers.
	for (auto* node : m_toDeleteNodes[m_currentNodesToDelete])
	{
		m_pool.Delete(node);
	}
	m_toDeleteNodes[m_currentNodesToDelete].clear();

	{
		// LOCK START
		CryAutoCriticalSectionNoRecursive lock(m_accessLock);

		int numNodes = m_visibleNodes.size();
		int start = m_lastStartUpdateNode;
		if (start >= numNodes)
		{
			start = 0;
			m_lastStartUpdateNode = 0;
		}
		int end = start + maxNodesToCheck;
		if (end > numNodes)
			end = numNodes;

		int lastNode = numNodes - 1;

		int maxFrames = (uint32)C3DEngine::GetCVars()->e_RNTmpDataPoolMaxFrames;
		int numItemsToDelete = 0;
		for (int i = start; i < end && i <= lastNode; ++i)
		{
			SRenderNodeTempData* RESTRICT_POINTER pTempData = m_visibleNodes[i];

			int lastSeenFrame = std::max(pTempData->userData.lastSeenFrame[0], pTempData->userData.lastSeenFrame[1]);
			lastSeenFrame = std::max(lastSeenFrame, pTempData->userData.lastSeenShadowFrame);
			int diff = std::abs(currentFrame - lastSeenFrame);
			if (diff > maxFrames || pTempData->userData.bToDelete)
			{
				if (pTempData->userData.pOwnerNode)
				{
					pTempData->userData.pOwnerNode->OnRenderNodeBecomeInvisible();
					pTempData->userData.pOwnerNode->m_pTempData = nullptr; // clear reference to use from owning render node.
				}
				m_visibleNodes[i]->Free();
				m_toDeleteNodes[m_currentNodesToDelete].push_back(m_visibleNodes[i]);
				if (i < lastNode)
				{
					// move item from the end of the array to this spot, and repeat check on same index.
					m_visibleNodes[i] = m_visibleNodes[lastNode];
					lastNode--;
					i--;
				}
				numItemsToDelete++;
			}
			m_lastStartUpdateNode = i + 1;
		}
		// delete not relevant items at the end.
		if (numItemsToDelete > 0)
		{
			m_visibleNodes.resize(m_visibleNodes.size() - numItemsToDelete);
		}
		// LOCK END
	}
	return;
}

//////////////////////////////////////////////////////////////////////////
void CVisibleRenderNodesManager::InvalidateAll()
{
	{
		// LOCK START
		CryAutoCriticalSectionNoRecursive lock(m_accessLock);
		for (auto* node : m_visibleNodes)
		{
			node->userData.bToDelete = true;
		}
		// LOCK END
	}
}

//////////////////////////////////////////////////////////////////////////
void CVisibleRenderNodesManager::ClearAll()
{
	CryAutoCriticalSectionNoRecursive lock(m_accessLock);

	for (auto* node : m_visibleNodes)
	{
		if (node->userData.pOwnerNode)
		{
			node->userData.pOwnerNode->OnRenderNodeBecomeInvisible();
			node->userData.pOwnerNode->m_pTempData = nullptr; // clear reference to use from owning render node.
		}
		m_pool.Delete(node);
	}
	m_visibleNodes.clear();

	for (auto& nodes : m_toDeleteNodes)
	{
		for (auto* node : nodes)
		{
			m_pool.Delete(node);
		}
		nodes.clear();
	}
}

CVisibleRenderNodesManager::Statistics CVisibleRenderNodesManager::GetStatistics() const
{
	Statistics stats;
	stats.numFree = 0;
	stats.numUsed = m_visibleNodes.size();
	return stats;
}
