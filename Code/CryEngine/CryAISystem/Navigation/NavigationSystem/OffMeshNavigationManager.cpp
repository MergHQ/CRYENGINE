// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "OffMeshNavigationManager.h"
#include "Navigation/MNM/NavMeshQueryManager.h"

#include "NavigationSystem.h"

OffMeshNavigationManager::OffMeshNavigationManager(const int offMeshMapSize)
	: m_offMeshMap(offMeshMapSize)
	, m_objectRegistrationEnabled(false)
	, m_additionProcessingIdx(0)
	, m_removalProcessingIdx(0)
{
	m_additionRequests.reserve(128);
	m_removalRequests.reserve(128);
	m_annotationChangeRequests.reserve(32);
}

const MNM::OffMeshNavigation& OffMeshNavigationManager::GetOffMeshNavigationForMesh(const NavigationMeshID& meshID) const
{
	if (meshID && m_offMeshMap.validate(meshID))
	{
		return m_offMeshMap[meshID];
	}
	else
	{
		CRY_ASSERT(0);
		AIWarning("No off-mesh navigation available, returning empty one! Navigation might not be able to use external links when path-finding");
		return m_emptyOffMeshNavigation;
	}
}

MNM::OffMeshLinkID OffMeshNavigationManager::RequestLinkAddition(const MNM::OffMeshLinkID linkId, const NavigationAgentTypeID agentTypeId, _smart_ptr<MNM::IOffMeshLink> pLinkData, const MNM::SOffMeshLinkCallbacks& callbacks)
{
	CRY_ASSERT(!IsLinkAdditionRequested(linkId), "Link with id %u is already requested!", linkId);
	CRY_ASSERT(m_debugRemovingLinkId == linkId || m_links.find(linkId) == m_links.end(), "Link with id %u is aready added!", linkId);

	const MNM::OffMeshLinkID requestLinkId = !linkId.IsValid() ? MNM::OffMeshNavigation::GenerateLinkId() : linkId;
	SLinkAdditionRequest request(agentTypeId, pLinkData, requestLinkId, callbacks);
	m_additionRequests.push_back(request);

#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
	AILogAlways("<OffMeshNavigationManager> Requested link addition. LinkId = %u. Requested id: %u", requestLinkId, linkId);
#endif

	return requestLinkId;
}

bool OffMeshNavigationManager::CancelLinkAddition(const MNM::OffMeshLinkID linkId)
{
	for (size_t i = m_additionProcessingIdx; i < m_additionRequests.size(); ++i)
	{
		if (m_additionRequests[i].linkId == linkId)
		{
#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
			AILogAlways("<OffMeshNavigationManager> Link addition canceled. LinkId = %u.", linkId);
#endif			
			m_additionRequests.erase(m_additionRequests.begin() + i);
			return true;
		}
	}
	return false;
}

void OffMeshNavigationManager::RequestLinkRemoval(const MNM::OffMeshLinkID linkId)
{
#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
	AILogAlways("<OffMeshNavigationManager> Requested link removal. LinkId = %u.", linkId);
#endif
	
	CancelLinkAddition(linkId);

	TLinkInfoMap::iterator linkIt = m_links.find(linkId);
	if (linkIt == m_links.end())
		return;

	linkIt->second.removedCallback = nullptr;

	m_removalRequests.push_back(linkId);
}

bool OffMeshNavigationManager::IsLinkAdditionRequested(const MNM::OffMeshLinkID linkId) const
{
	for (const SLinkAdditionRequest& additionRequest : m_additionRequests)
	{
		if (additionRequest.linkId == linkId)
			return true;
	}
	return false;
}

bool OffMeshNavigationManager::IsLinkRemovalRequested(const MNM::OffMeshLinkID linkId) const
{
	for (const MNM::OffMeshLinkID removalLinkId : m_removalRequests)
	{
		if (removalLinkId == linkId)
			return true;
	}
	return false;
}

void OffMeshNavigationManager::ProcessQueuedRequests()
{
	m_removalProcessingIdx = 0;
	m_additionProcessingIdx = 0;

	while (m_additionProcessingIdx < m_additionRequests.size() || m_removalProcessingIdx < m_removalRequests.size())
	{
		while (m_removalProcessingIdx < m_removalRequests.size())
		{
			ProcessLinkRemovalRequest(m_removalRequests[m_removalProcessingIdx++]);
		}
		while (m_additionProcessingIdx < m_additionRequests.size())
		{
			ProcessLinkAdditionRequest(m_additionRequests[m_additionProcessingIdx++]);
		}
	}

	m_additionRequests.clear();
	m_removalRequests.clear();

	m_removalProcessingIdx = 0;
	m_additionProcessingIdx = 0;

	for (const SAnnotationChangeRequest& annotationChangeRequest : m_annotationChangeRequests)
	{
		ProcessAnnotationChangeRequest(annotationChangeRequest);
	}
	m_annotationChangeRequests.clear();
}

MNM::EOffMeshLinkAdditionResult OffMeshNavigationManager::GetNavigationLinkDataForAdditionRequest(const MNM::SOffMeshLinkCallbacks& callbacks, const NavigationAgentTypeID agentTypeId, MNM::IOffMeshLink* pLinkData, MNM::IOffMeshLink::SNavigationData& navigationLinkData) const
{
	if (!callbacks.acquireDataCallback)
		return MNM::EOffMeshLinkAdditionResult::AcquireCallbackNotSet;

	const MNM::EOffMeshLinkAdditionResult acquireDataResult = callbacks.acquireDataCallback(agentTypeId, pLinkData, navigationLinkData);
	if (acquireDataResult != MNM::EOffMeshLinkAdditionResult::Success)
		return acquireDataResult;

	if (!navigationLinkData.startTriangleId.IsValid())
		return MNM::EOffMeshLinkAdditionResult::InvalidStartTriangle; // Entry position not connected to a triangle

	if (!navigationLinkData.endTriangleId.IsValid())
		return MNM::EOffMeshLinkAdditionResult::InvalidEndTriangle; // Exit position not connected to a triangle

	if (!navigationLinkData.meshId.IsValid())
		return MNM::EOffMeshLinkAdditionResult::InvalidMesh;

	return MNM::EOffMeshLinkAdditionResult::Success;
}

bool OffMeshNavigationManager::ProcessLinkAdditionRequest(const SLinkAdditionRequest& request)
{
	MNM::IOffMeshLink::SNavigationData navigationLinkData;
	const MNM::EOffMeshLinkAdditionResult result = TryCreateLink(request, navigationLinkData);
	if (request.callbacks.additionCompletedCallback)
	{
		request.callbacks.additionCompletedCallback(request.linkId, MNM::SOffMeshAdditionCallbackData(navigationLinkData.startTriangleId, navigationLinkData.endTriangleId, result));
	}
	return result == MNM::EOffMeshLinkAdditionResult::Success;
}

MNM::EOffMeshLinkAdditionResult OffMeshNavigationManager::TryCreateLink(const SLinkAdditionRequest& request, MNM::IOffMeshLink::SNavigationData& navigationLinkData)
{
	CRY_ASSERT(request.pLinkData.get() != nullptr);

	const MNM::OffMeshLinkID linkId = request.linkId;
	TLinkInfoMap::const_iterator it = m_links.find(linkId);
	if (it != m_links.end())
	{
		CRY_ASSERT(it == m_links.end(), "OffMesh link with id %u already exists!", linkId);
		return MNM::EOffMeshLinkAdditionResult::AlreadyExists;
	}

	SetIdToOffMeshLink(*request.pLinkData, request.linkId);

	const MNM::EOffMeshLinkAdditionResult result = GetNavigationLinkDataForAdditionRequest(request.callbacks, request.agentTypeId, request.pLinkData.get(), navigationLinkData);
	if (result != MNM::EOffMeshLinkAdditionResult::Success)
		return result;

	if (navigationLinkData.startTriangleId == navigationLinkData.endTriangleId)
		return MNM::EOffMeshLinkAdditionResult::Success; // Points are on the same triangle, so don't bother
	
	const NavigationMeshID meshId = navigationLinkData.meshId;
	if (!m_offMeshMap.validate(meshId))
	{
		CRY_ASSERT(m_offMeshMap.validate(meshId), "Trying to add offmesh link to invalid mesh! (meshID = %u)", (uint32)meshId);
		return MNM::EOffMeshLinkAdditionResult::InvalidMesh;
	}

	if (navigationLinkData.annotation.GetRawValue() == 0)
	{
		const MNM::SAreaType& defaultArea = gAIEnv.pNavigationSystem->GetAnnotationLibrary().GetDefaultAreaType();
		navigationLinkData.annotation.SetFlags(defaultArea.defaultFlags);
		navigationLinkData.annotation.SetType(defaultArea.id);
	}

	// Select the corresponding off-mesh navigation object
	NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(navigationLinkData.meshId);
	MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshId];

	// Register the new link with the off-mesh navigation system
	offMeshNavigation.AddLink(mesh, navigationLinkData.startTriangleId, navigationLinkData.endTriangleId, linkId);
	CRY_ASSERT(linkId.IsValid(), "Adding new offmesh link failed");

	m_links.emplace(linkId, SLinkControlInfo(meshId, navigationLinkData.startTriangleId, navigationLinkData.endTriangleId, navigationLinkData.annotation, request.pLinkData, request.callbacks));

	gAIEnv.pNavigationSystem->AddOffMeshLinkIslandConnectionsBetweenTriangles(meshId, navigationLinkData.startTriangleId, navigationLinkData.endTriangleId, linkId);
	gAIEnv.pNavigationSystem->RequestUpdateMeshAccessibility(meshId);

	return MNM::EOffMeshLinkAdditionResult::Success;
}


void OffMeshNavigationManager::ProcessLinkRemovalRequest(const MNM::OffMeshLinkID linkId)
{
	TLinkInfoMap::iterator linkIt = m_links.find(linkId);
	if (linkIt == m_links.end())
		return;
	
	SLinkControlInfo& linkInfo = linkIt->second;

	// Select the corresponding off-mesh navigation object
	CRY_ASSERT(m_offMeshMap.validate(linkInfo.meshID));
	MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[linkInfo.meshID];
	NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(linkInfo.meshID);

	// Remove link
	// NOTE: There should only ever be one element in the link array
	offMeshNavigation.RemoveLink(mesh, linkInfo.startTriangleID, linkId);

	gAIEnv.pNavigationSystem->RemoveOffMeshLinkIslandConnection(linkId);
	gAIEnv.pNavigationSystem->RequestUpdateMeshAccessibility(linkInfo.meshID);

	// Remove cached data
	m_links.erase(linkIt);
}

void OffMeshNavigationManager::ProcessAnnotationChangeRequest(const SAnnotationChangeRequest& request)
{
	TLinkInfoMap::iterator linkIt = m_links.find(request.linkId);
	if (linkIt == m_links.end())
		return;

	SLinkControlInfo& linkInfo = linkIt->second;
	linkInfo.annotation = request.annotation;
}

MNM::IOffMeshLink* OffMeshNavigationManager::GetOffMeshLink(const MNM::OffMeshLinkID linkId)
{
	TLinkInfoMap::iterator linkIt = m_links.find(linkId);
	if (linkIt != m_links.end())
	{
		SLinkControlInfo& linkInfo = linkIt->second;
		return linkInfo.offMeshLink.get();
	}
	return nullptr;
}

const MNM::IOffMeshLink* OffMeshNavigationManager::GetOffMeshLink(const MNM::OffMeshLinkID linkId) const
{
	TLinkInfoMap::const_iterator linkIt = m_links.find(linkId);
	if (linkIt != m_links.end())
	{
		const SLinkControlInfo& linkInfo = linkIt->second;
		return linkInfo.offMeshLink.get();
	}
	return nullptr;
}

const MNM::IOffMeshLink* OffMeshNavigationManager::GetLinkAndAnnotation(const MNM::OffMeshLinkID linkId, MNM::AreaAnnotation& annotation) const
{
	TLinkInfoMap::const_iterator linkIt = m_links.find(linkId);
	if (linkIt == m_links.end())
		return nullptr;

	const SLinkControlInfo& linkInfo = linkIt->second;
	annotation = linkInfo.annotation;
	return linkInfo.offMeshLink.get();
}

void OffMeshNavigationManager::RequestLinkAnnotationChange(const MNM::OffMeshLinkID linkId, const MNM::AreaAnnotation annotation)
{
	m_annotationChangeRequests.emplace_back(linkId, annotation);
}

void OffMeshNavigationManager::RefreshConnections(const NavigationMeshID meshId, const MNM::TileID tileId)
{
	if (!m_offMeshMap.validate(meshId))
		return;

	MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshId];
	NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(meshId);

	//////////////////////////////////////////////////////////////////////////
	// Invalidate links for the tile.
	// Because the tile has been regenerated, all outgoing off-mesh links are invalid now
	offMeshNavigation.InvalidateAllLinksForTile(tileId);

	//////////////////////////////////////////////////////////////////////////
	// Refresh all links tied to this mesh and tile
	// NOTE: This only re-adds them to the Tile with the cached data.
	for (TLinkInfoMap::iterator iter = m_links.begin(); iter != m_links.end(); )
	{
		// Find out if object is bound to the modified mesh and tile
		SLinkControlInfo& linkInfo = iter->second;
		if (linkInfo.meshID != meshId)
		{
			++iter;
			continue;
		}

		const bool isLinkOutgoingFromThisTile = (tileId == MNM::ComputeTileID(linkInfo.startTriangleID));
		const bool isLinkIncomingToThisTile = (tileId == MNM::ComputeTileID(linkInfo.endTriangleID));

		// is the link completely unrelated to given tile?
		if (!isLinkOutgoingFromThisTile && !isLinkIncomingToThisTile)
		{
			++iter;
			continue;
		}

		const MNM::OffMeshLinkID linkId = iter->first;

		// is the link incoming from a different tile?
		// -> need to remove it before it can potentially become a dangling link (i. e. the triangleID where it ends would no longer exist or would have morphed into a different one)
		//    (in fact, it *is* already a dangling link because given tile has just been regenerated and all its triangleIDs most likely have changed!)
		//    Notice that InvalidateLinks(tileID) only cares about outgoing links and also doesn't remove the link-information from the NavMesh, so we need
		//    to call RemoveLink() to get the NavMesh patched as well.
		if (isLinkIncomingToThisTile && !isLinkOutgoingFromThisTile)
		{
			offMeshNavigation.RemoveLink(mesh, linkInfo.startTriangleID, linkId);
		}

		// disconnect whatever islands were connected by the off-mesh link
		gAIEnv.pNavigationSystem->RemoveOffMeshLinkIslandConnection(linkId);

#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
		AILogAlways("<OffMeshNavigationManager> Link Removed. LinkId = %u.", linkId);
#endif

		// Notify link owner about it is going to be removed so it can clean up cached data and request re-addition if needed
		if (linkInfo.removedCallback)
		{
#ifdef USE_CRY_ASSERT
			m_debugRemovingLinkId = linkId;
#endif // USE_CRY_ASSERT
			linkInfo.removedCallback(linkId, meshId, MNM::EOffMeshLinkRemovalReason::MeshUpdated);
#ifdef USE_CRY_ASSERT
			m_debugRemovingLinkId.Invalidate();
#endif // USE_CRY_ASSERT
		}

		// But for the meantime, also remove the entry from m_links because the .triangleID member was referencing a triangle in the now regenerated tile (!)
		//
		// Carefully read this:
		//
		//    If we didn't do this, then remove-link requests and add-link requests could use that "dead" triangle reference
		//    to try to remove off-mesh links in the NavMesh where there was none, OR try to add off-mesh links in the NavMesh where there was one already, OR
		//    (even worse, as no assert will catch this scenario) it would manipulate *foreign* off-mesh links in the NavMesh.
		//

		iter = m_links.erase(iter);
	}
}

void OffMeshNavigationManager::Clear()
{
	m_offMeshMap.clear();
	m_links.clear();
	m_objectRegistrationEnabled = false;

	stl::free_container(m_additionRequests);
	stl::free_container(m_removalRequests);
}

void OffMeshNavigationManager::Enable()
{
	m_objectRegistrationEnabled = true;
}

void OffMeshNavigationManager::OnNavigationMeshCreated(const NavigationMeshID& meshId)
{
	CRY_ASSERT(!m_offMeshMap.validate(meshId));

	m_offMeshMap.insert(meshId, MNM::OffMeshNavigation());
}

void OffMeshNavigationManager::OnNavigationMeshDestroyed(const NavigationMeshID& meshId)
{
	CRY_ASSERT(m_offMeshMap.validate(meshId));

	m_offMeshMap.erase(meshId);

	// Remove all existing links for this mesh
	TLinkInfoMap::iterator iter;
	for (iter = m_links.begin(); iter != m_links.end(); )
	{
		// Find out if object is bound to the modified mesh and tile
		SLinkControlInfo& linkInfo = iter->second;
		if (linkInfo.meshID != meshId)
		{
			++iter;
			continue; 
		}

		const MNM::OffMeshLinkID linkId = iter->first;

#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
		AILogAlways("<OffMeshNavigationManager> Link Removed. LinkId = %u.", linkId);
#endif

		if (linkInfo.removedCallback)
		{
#ifdef USE_CRY_ASSERT
			m_debugRemovingLinkId = linkId;
#endif // USE_CRY_ASSERT
			linkInfo.removedCallback(linkId, meshId, MNM::EOffMeshLinkRemovalReason::MeshDestroyed);
#ifdef USE_CRY_ASSERT
			m_debugRemovingLinkId.Invalidate();
#endif // USE_CRY_ASSERT
		}

		m_links.erase(iter++);
	}
}

void OffMeshNavigationManager::OnNavigationLoadedComplete()
{
	//////////////////////////////////////////////////////////////////////////
	//Only after the navigation loaded process is completed off-mesh links can be created
	//Off-mesh links are created after the mesh is loaded, they are not stored within the mesh when exported
	Enable();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if DEBUG_MNM_ENABLED

OffMeshNavigationManager::ProfileMemoryStats OffMeshNavigationManager::GetMemoryStats(ICrySizer* pSizer, const NavigationMeshID meshId) const
{
	const size_t initialSize = pSizer->GetTotalSize();
	ProfileMemoryStats memStats;

	// for all links: track memory of the ones with given mesh ID
	for (TLinkInfoMap::const_iterator it = m_links.begin(); it != m_links.end(); ++it)
	{
		const SLinkControlInfo& linkInfo = it->second;

		if (linkInfo.meshID == meshId)
		{
			const size_t sizeBeforeLinkInfo = pSizer->GetTotalSize();
			pSizer->AddObjectSize(&linkInfo);
			memStats.linkInfoSize += pSizer->GetTotalSize() - sizeBeforeLinkInfo;
		}
	}

	memStats.totalSize = pSizer->GetTotalSize() - initialSize;

	return memStats;
}

#endif
