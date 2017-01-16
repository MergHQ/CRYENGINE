// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "NavigationUpdatesManager.h"

#include "Navigation/NavigationSystem/NavigationSystem.h"

//------------------------------------------------------------------------
CMNMUpdatesManager::CMNMUpdatesManager(NavigationSystem* pNavSystem)
	: m_pNavigationSystem(pNavSystem)
	, m_isMNMRegenerationRequestExecutionEnabled(true)
	, m_wasMNMRegenerationRequestedThisUpdateCycle(false)
	, m_postponeUpdatesForStabilization(false)
	, m_lastUpdateTime(0.0f)
{

}

//------------------------------------------------------------------------
void CMNMUpdatesManager::ClearBufferedRegenerationRequests()
{
	m_ignoredUpdateRequestsQueue.clear();
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::Clear()
{
	m_activeUpdateRequestsQueue.clear();
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::Update()
{
	UpdateEntityChanges();
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::UpdateEntityChanges()
{
	if (m_postponeUpdatesForStabilization)
	{
		if (gEnv->pTimer->GetFrameStartTime().GetDifferenceInSeconds(m_lastUpdateTime) < gAIEnv.CVars.NavmeshStabilizationTimeToUpdate)
		{
			return;
		}
	}
	
	for (const auto& it : m_bufferedEntityUpdatesMap)
	{
		const EntityUpdate& entityUpdate = it.second;

		if (!entityUpdate.aabbNew.IsReset())
		{
			if (((entityUpdate.aabbOld.min - entityUpdate.aabbNew.min).len2() + (entityUpdate.aabbOld.max - entityUpdate.aabbNew.max).len2()) > 0.0f)
			{
				WorldChanged(entityUpdate.aabbOld);
				WorldChanged(entityUpdate.aabbNew);
			}
		}
		else
		{
			WorldChanged(entityUpdate.aabbOld);
		}
	}
	m_bufferedEntityUpdatesMap.clear();
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::OnMeshDestroyed(NavigationMeshID meshID)
{
	for (TileUpdateRequest& task : m_activeUpdateRequestsQueue)
	{
		if (task.meshID == meshID)
			task.aborted = true;
	}
	
	TileTaskQueue::iterator it = m_ignoredUpdateRequestsQueue.begin();
	TileTaskQueue::iterator end = m_ignoredUpdateRequestsQueue.end();
	while(it != end)
	{
		TileUpdateRequest& task = *it;
		if (task.meshID == meshID)
		{
			end = end - 1;
			std::swap(task, *end);
			continue;
		}
		++it;
	}
	if (end != m_ignoredUpdateRequestsQueue.end())
	{
		m_ignoredUpdateRequestsQueue.erase(end, m_ignoredUpdateRequestsQueue.end());
	}
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::EntityChanged(int id, const AABB& aabb)
{
#if NAVIGATION_SYSTEM_PC_ONLY
	if (id == -1)
	{
		WorldChanged(aabb);
		return;
	}

	m_lastUpdateTime = gEnv->pTimer->GetFrameStartTime();

	auto it = m_bufferedEntityUpdatesMap.find(id);
	if (it != m_bufferedEntityUpdatesMap.end())
	{
		it->second.aabbNew = aabb;
	}
	else if(aabb.IsNonZero())
	{
		EntityUpdate entityUpdate;

		entityUpdate.aabbOld = aabb;
		entityUpdate.aabbNew = AABB(AABB::RESET);
		m_bufferedEntityUpdatesMap[id] = entityUpdate;
	}
#endif //NAVIGATION_SYSTEM_PC_ONLY
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::WorldChanged(const AABB& aabb)
{
#if NAVIGATION_SYSTEM_PC_ONLY
	for (const AgentType& agentType : m_pNavigationSystem->m_agentTypes)
	{
		for (const AgentType::MeshInfo& meshInfo : agentType.meshes)
		{
			const NavigationMeshID meshID = meshInfo.id;
			const NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
			
			if (mesh.boundary && Overlap::AABB_AABB(aabb, m_pNavigationSystem->m_volumes[mesh.boundary].aabb))
			{
				RequestQueueMeshUpdate(meshID, aabb);
			}
		}
	}
#endif //NAVIGATION_SYSTEM_PC_ONLY
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::RequestGlobalUpdateForAgentType(NavigationAgentTypeID agentTypeID)
{
	if (!agentTypeID || agentTypeID > m_pNavigationSystem->m_agentTypes.size())
		return;
	
	const AgentType& agentType = m_pNavigationSystem->m_agentTypes[agentTypeID - 1];

	for (const AgentType::MeshInfo& meshInfo : agentType.meshes)
	{
		const NavigationMeshID meshID = meshInfo.id;
		const NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
		const NavigationVolumeID boundaryID = mesh.boundary;

		if (boundaryID)
		{
			RequestQueueMeshUpdate(meshID, m_pNavigationSystem->m_worldAABB);
		}
	}
}

//------------------------------------------------------------------------
CMNMUpdatesManager::EUpdateRequestStatus CMNMUpdatesManager::GetCurrentRequestsQueue(CMNMUpdatesManager::TileTaskQueue*& queue)
{
	if (!m_isMNMRegenerationRequestExecutionEnabled)
	{
		queue = &m_ignoredUpdateRequestsQueue;
		return EUpdateRequestStatus::RequestDelayedAndBuffered;
	}
	else
	{
		queue = &m_activeUpdateRequestsQueue;
		return EUpdateRequestStatus::RequestInQueue;
	}
}

//------------------------------------------------------------------------
CMNMUpdatesManager::EUpdateRequestStatus CMNMUpdatesManager::RequestQueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb)
{
	assert(meshID != 0);

#if NAVIGATION_SYSTEM_PC_ONLY
	if (!meshID || !m_pNavigationSystem->m_meshes.validate(meshID))
	{
		return EUpdateRequestStatus::RequestInvalid;
	}

	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
	MNM::CNavMesh& navMesh = mesh.navMesh;

	if (aabb.IsEmpty() || !mesh.boundary)
		return EUpdateRequestStatus::RequestInvalid;

	m_wasMNMRegenerationRequestedThisUpdateCycle = true;

	const MNM::CNavMesh::SGridParams& paramsGrid = navMesh.GetGridParams();

	const AABB& boundary = m_pNavigationSystem->m_volumes[mesh.boundary].aabb;
	const AgentType& agentType = m_pNavigationSystem->m_agentTypes[mesh.agentTypeID - 1];

	const float extraH = std::max(paramsGrid.voxelSize.x, paramsGrid.voxelSize.y) * (agentType.settings.radiusVoxelCount + 1);
	const float extraV = paramsGrid.voxelSize.z * (agentType.settings.heightVoxelCount + 1);
	const float extraVM = paramsGrid.voxelSize.z; // tiles above are not directly influenced

	Vec3 bmin(std::max(0.0f, std::max(boundary.min.x, aabb.min.x - extraH) - paramsGrid.origin.x),
		std::max(0.0f, std::max(boundary.min.y, aabb.min.y - extraH) - paramsGrid.origin.y),
		std::max(0.0f, std::max(boundary.min.z, aabb.min.z - extraV) - paramsGrid.origin.z));

	Vec3 bmax(std::max(0.0f, std::min(boundary.max.x, aabb.max.x + extraH) - paramsGrid.origin.x),
		std::max(0.0f, std::min(boundary.max.y, aabb.max.y + extraH) - paramsGrid.origin.y),
		std::max(0.0f, std::min(boundary.max.z, aabb.max.z + extraVM) - paramsGrid.origin.z));

	uint16 xmin = (uint16)(floor_tpl(bmin.x / (float)paramsGrid.tileSize.x));
	uint16 xmax = (uint16)(floor_tpl(bmax.x / (float)paramsGrid.tileSize.x));

	uint16 ymin = (uint16)(floor_tpl(bmin.y / (float)paramsGrid.tileSize.y));
	uint16 ymax = (uint16)(floor_tpl(bmax.y / (float)paramsGrid.tileSize.y));

	uint16 zmin = (uint16)(floor_tpl(bmin.z / (float)paramsGrid.tileSize.z));
	uint16 zmax = (uint16)(floor_tpl(bmax.z / (float)paramsGrid.tileSize.z));

	TileTaskQueue* requestsQueue;
	EUpdateRequestStatus result = GetCurrentRequestsQueue(requestsQueue);

	TileTaskQueue::iterator it = requestsQueue->begin();
	TileTaskQueue::iterator rear = requestsQueue->end();

	for (; it != rear; )
	{
		TileUpdateRequest& task = *it;

		if ((task.meshID == meshID) && (task.x >= xmin) && (task.x <= xmax) &&
			(task.y >= ymin) && (task.y <= ymax) &&
			(task.z >= zmin) && (task.z <= zmax))
		{
			rear = rear - 1;
			std::swap(task, *rear);

			continue;
		}
		++it;
	}

	if (rear != requestsQueue->end())
	{
		requestsQueue->erase(rear, requestsQueue->end());
	}

	for (size_t y = ymin; y <= ymax; ++y)
	{
		for (size_t x = xmin; x <= xmax; ++x)
		{
			for (size_t z = zmin; z <= zmax; ++z)
			{
				TileUpdateRequest task;
				task.meshID = meshID;
				task.x = (uint16)x;
				task.y = (uint16)y;
				task.z = (uint16)z;
				requestsQueue->push_back(task);
			}
		}
	}
	return result;
#else //!NAVIGATION_SYSTEM_PC_ONLY
	return EUpdateRequestStatus::RequestInvalid;
#endif //!NAVIGATION_SYSTEM_PC_ONLY
}

//------------------------------------------------------------------------
CMNMUpdatesManager::EUpdateRequestStatus CMNMUpdatesManager::RequestQueueDifferenceUpdate(
	NavigationMeshID meshID,
	const NavigationBoundingVolume& oldVolume,
	const NavigationBoundingVolume& newVolume)
{
#if NAVIGATION_SYSTEM_PC_ONLY
	// TODO: implement properly by verifying what didn't change
	// since there will be loads of volume-aabb intersection tests,
	// this should be a new job running in a different thread
	// producing an array of all the tiles that need updating
	// which then gets concatenated into m_tileQueue in the main update

	if (!meshID || !m_pNavigationSystem->m_meshes.validate(meshID))
	{
		return EUpdateRequestStatus::RequestInvalid;
	}

	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
	MNM::CNavMesh& navMesh = mesh.navMesh;

	m_wasMNMRegenerationRequestedThisUpdateCycle = true;

	const MNM::CNavMesh::SGridParams& paramsGrid = navMesh.GetGridParams();

	AABB aabb = oldVolume.aabb;
	aabb.Add(newVolume.aabb);

	const AgentType& agentType = m_pNavigationSystem->m_agentTypes[mesh.agentTypeID - 1];

	const float extraH = std::max(paramsGrid.voxelSize.x, paramsGrid.voxelSize.y) * (agentType.settings.radiusVoxelCount + 1);
	const float extraV = paramsGrid.voxelSize.z * (agentType.settings.heightVoxelCount + 1);
	const float extraVM = paramsGrid.voxelSize.z; // tiles above are not directly influenced

	Vec3 bmin(std::max(0.0f, (aabb.min.x - extraH) - paramsGrid.origin.x),
		std::max(0.0f, (aabb.min.y - extraH) - paramsGrid.origin.y),
		std::max(0.0f, (aabb.min.z - extraV) - paramsGrid.origin.z));

	Vec3 bmax(std::max(0.0f, (aabb.max.x + extraH) - paramsGrid.origin.x),
		std::max(0.0f, (aabb.max.y + extraH) - paramsGrid.origin.y),
		std::max(0.0f, (aabb.max.z + extraVM) - paramsGrid.origin.z));

	uint16 xmin = (uint16)(floor_tpl(bmin.x / (float)paramsGrid.tileSize.x));
	uint16 xmax = (uint16)(floor_tpl(bmax.x / (float)paramsGrid.tileSize.x));

	uint16 ymin = (uint16)(floor_tpl(bmin.y / (float)paramsGrid.tileSize.y));
	uint16 ymax = (uint16)(floor_tpl(bmax.y / (float)paramsGrid.tileSize.y));

	uint16 zmin = (uint16)(floor_tpl(bmin.z / (float)paramsGrid.tileSize.z));
	uint16 zmax = (uint16)(floor_tpl(bmax.z / (float)paramsGrid.tileSize.z));

	TileTaskQueue* requestsQueue;
	EUpdateRequestStatus result = GetCurrentRequestsQueue(requestsQueue);
	
	TileTaskQueue::iterator it = requestsQueue->begin();
	TileTaskQueue::iterator rear = requestsQueue->end();

	for (; it != rear; )
	{
		TileUpdateRequest& task = *it;

		if ((task.meshID == meshID) && (task.x >= xmin) && (task.x <= xmax) &&
			(task.y >= ymin) && (task.y <= ymax) &&
			(task.z >= zmin) && (task.z <= zmax))
		{
			rear = rear - 1;
			std::swap(task, *rear);

			continue;
		}
		++it;
	}

	if (rear != requestsQueue->end())
	{
		requestsQueue->erase(rear, requestsQueue->end());
	}

	for (size_t y = ymin; y <= ymax; ++y)
	{
		for (size_t x = xmin; x <= xmax; ++x)
		{
			for (size_t z = zmin; z <= zmax; ++z)
			{
				TileUpdateRequest task;
				task.meshID = meshID;
				task.x = (uint16)x;
				task.y = (uint16)y;
				task.z = (uint16)z;

				requestsQueue->push_back(task);
			}
		}
	}
	return result;
#else //!NAVIGATION_SYSTEM_PC_ONLY
	return EUpdateRequestStatus::RequestInvalid;
#endif //!NAVIGATION_SYSTEM_PC_ONLY
}

//------------------------------------------------------------------------
size_t CMNMUpdatesManager::QueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb)
{
	assert(meshID != 0);

	size_t affectedCount = 0;
#if NAVIGATION_SYSTEM_PC_ONLY
	if (meshID && m_pNavigationSystem->m_meshes.validate(meshID))
	{
		FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

		NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
		MNM::CNavMesh& navMesh = mesh.navMesh;

		const MNM::CNavMesh::SGridParams& paramsGrid = navMesh.GetGridParams();

		if (aabb.IsEmpty() || !mesh.boundary)
			return 0;

		const AABB& boundary = m_pNavigationSystem->m_volumes[mesh.boundary].aabb;
		const AgentType& agentType = m_pNavigationSystem->m_agentTypes[mesh.agentTypeID - 1];

		const float extraH = std::max(paramsGrid.voxelSize.x, paramsGrid.voxelSize.y) * (agentType.settings.radiusVoxelCount + 1);
		const float extraV = paramsGrid.voxelSize.z * (agentType.settings.heightVoxelCount + 1);
		const float extraVM = paramsGrid.voxelSize.z; // tiles above are not directly influenced

		Vec3 bmin(std::max(0.0f, std::max(boundary.min.x, aabb.min.x - extraH) - paramsGrid.origin.x),
			std::max(0.0f, std::max(boundary.min.y, aabb.min.y - extraH) - paramsGrid.origin.y),
			std::max(0.0f, std::max(boundary.min.z, aabb.min.z - extraV) - paramsGrid.origin.z));

		Vec3 bmax(std::max(0.0f, std::min(boundary.max.x, aabb.max.x + extraH) - paramsGrid.origin.x),
			std::max(0.0f, std::min(boundary.max.y, aabb.max.y + extraH) - paramsGrid.origin.y),
			std::max(0.0f, std::min(boundary.max.z, aabb.max.z + extraVM) - paramsGrid.origin.z));

		uint16 xmin = (uint16)(floor_tpl(bmin.x / (float)paramsGrid.tileSize.x));
		uint16 xmax = (uint16)(floor_tpl(bmax.x / (float)paramsGrid.tileSize.x));

		uint16 ymin = (uint16)(floor_tpl(bmin.y / (float)paramsGrid.tileSize.y));
		uint16 ymax = (uint16)(floor_tpl(bmax.y / (float)paramsGrid.tileSize.y));

		uint16 zmin = (uint16)(floor_tpl(bmin.z / (float)paramsGrid.tileSize.z));
		uint16 zmax = (uint16)(floor_tpl(bmax.z / (float)paramsGrid.tileSize.z));

		TileTaskQueue::iterator it = m_activeUpdateRequestsQueue.begin();
		TileTaskQueue::iterator rear = m_activeUpdateRequestsQueue.end();

		for (; it != rear; )
		{
			TileUpdateRequest& task = *it;

			if ((task.meshID == meshID) && (task.x >= xmin) && (task.x <= xmax) &&
				(task.y >= ymin) && (task.y <= ymax) &&
				(task.z >= zmin) && (task.z <= zmax))
			{
				rear = rear - 1;
				std::swap(task, *rear);

				continue;
			}
			++it;
		}

		if (rear != m_activeUpdateRequestsQueue.end())
		{
			m_activeUpdateRequestsQueue.erase(rear, m_activeUpdateRequestsQueue.end());
		}

		for (size_t y = ymin; y <= ymax; ++y)
		{
			for (size_t x = xmin; x <= xmax; ++x)
			{
				for (size_t z = zmin; z <= zmax; ++z)
				{
					TileUpdateRequest task;
					task.meshID = meshID;
					task.x = (uint16)x;
					task.y = (uint16)y;
					task.z = (uint16)z;

					m_activeUpdateRequestsQueue.push_back(task);

					++affectedCount;
				}
			}
		}
	}

#endif
	return affectedCount;
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::QueueDifferenceUpdate(NavigationMeshID meshID, const NavigationBoundingVolume& oldVolume, const NavigationBoundingVolume& newVolume)
{
#if NAVIGATION_SYSTEM_PC_ONLY

	// TODO: implement properly by verifying what didn't change
	// since there will be loads of volume-aabb intersection tests,
	// this should be a new job running in a different thread
	// producing an array of all the tiles that need updating
	// which then gets concatenated into m_tileQueue in the main update
	if (meshID && m_pNavigationSystem->m_meshes.validate(meshID))
	{
		FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

		NavigationMesh& mesh = m_pNavigationSystem->m_meshes[meshID];
		MNM::CNavMesh& navMesh = mesh.navMesh;

		const MNM::CNavMesh::SGridParams& paramsGrid = navMesh.GetGridParams();

		AABB aabb = oldVolume.aabb;
		aabb.Add(newVolume.aabb);

		const AgentType& agentType = m_pNavigationSystem->m_agentTypes[mesh.agentTypeID - 1];

		const float extraH = std::max(paramsGrid.voxelSize.x, paramsGrid.voxelSize.y) * (agentType.settings.radiusVoxelCount + 1);
		const float extraV = paramsGrid.voxelSize.z * (agentType.settings.heightVoxelCount + 1);
		const float extraVM = paramsGrid.voxelSize.z; // tiles above are not directly influenced

		Vec3 bmin(std::max(0.0f, (aabb.min.x - extraH) - paramsGrid.origin.x),
			std::max(0.0f, (aabb.min.y - extraH) - paramsGrid.origin.y),
			std::max(0.0f, (aabb.min.z - extraV) - paramsGrid.origin.z));

		Vec3 bmax(std::max(0.0f, (aabb.max.x + extraH) - paramsGrid.origin.x),
			std::max(0.0f, (aabb.max.y + extraH) - paramsGrid.origin.y),
			std::max(0.0f, (aabb.max.z + extraVM) - paramsGrid.origin.z));

		uint16 xmin = (uint16)(floor_tpl(bmin.x / (float)paramsGrid.tileSize.x));
		uint16 xmax = (uint16)(floor_tpl(bmax.x / (float)paramsGrid.tileSize.x));

		uint16 ymin = (uint16)(floor_tpl(bmin.y / (float)paramsGrid.tileSize.y));
		uint16 ymax = (uint16)(floor_tpl(bmax.y / (float)paramsGrid.tileSize.y));

		uint16 zmin = (uint16)(floor_tpl(bmin.z / (float)paramsGrid.tileSize.z));
		uint16 zmax = (uint16)(floor_tpl(bmax.z / (float)paramsGrid.tileSize.z));

		TileTaskQueue::iterator it = m_activeUpdateRequestsQueue.begin();
		TileTaskQueue::iterator rear = m_activeUpdateRequestsQueue.end();

		for (; it != rear; )
		{
			TileUpdateRequest& task = *it;

			if ((task.meshID == meshID) && (task.x >= xmin) && (task.x <= xmax) &&
				(task.y >= ymin) && (task.y <= ymax) &&
				(task.z >= zmin) && (task.z <= zmax))
			{
				rear = rear - 1;
				std::swap(task, *rear);

				continue;
			}
			++it;
		}

		if (rear != m_activeUpdateRequestsQueue.end())
		{
			m_activeUpdateRequestsQueue.erase(rear, m_activeUpdateRequestsQueue.end());
		}

		for (size_t y = ymin; y <= ymax; ++y)
		{
			for (size_t x = xmin; x <= xmax; ++x)
			{
				for (size_t z = zmin; z <= zmax; ++z)
				{
					TileUpdateRequest task;
					task.meshID = meshID;
					task.x = (uint16)x;
					task.y = (uint16)y;
					task.z = (uint16)z;

					m_activeUpdateRequestsQueue.push_back(task);
				}
			}
		}
	}
#endif
}

//------------------------------------------------------------------------
void CMNMUpdatesManager::DebugDraw()
{
	if (!gEnv->pRenderer)
		return;

	IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
	if (!pRenderAuxGeom)
		return;
	
	for (const auto& it : m_bufferedEntityUpdatesMap)
	{
		const EntityUpdate& entityUpdate = it.second;

		pRenderAuxGeom->DrawAABB(entityUpdate.aabbOld, IDENTITY, false, Col_Red, eBBD_Faceted);

		if (!entityUpdate.aabbNew.IsReset())
		{
			pRenderAuxGeom->DrawAABB(entityUpdate.aabbNew, IDENTITY, false, Col_Red, eBBD_Faceted);
		}
	}
}
