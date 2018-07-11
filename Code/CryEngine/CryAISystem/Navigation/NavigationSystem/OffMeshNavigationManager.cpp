// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "OffMeshNavigationManager.h"

#include "NavigationSystem.h"

OffMeshNavigationManager::OffMeshNavigationManager(const int offMeshMapSize)
	: m_offMeshMap(offMeshMapSize)
	, m_objectRegistrationEnabled(false)
	, m_operations(128)
	, m_listeners(32)
{
}

const MNM::OffMeshNavigation& OffMeshNavigationManager::GetOffMeshNavigationForMesh(const NavigationMeshID& meshID) const
{
	if (meshID && m_offMeshMap.validate(meshID))
	{
		return m_offMeshMap[meshID];
	}
	else
	{
		assert(0);
		AIWarning("No off-mesh navigation available, returning empty one! Navigation might not be able to use external links when path-finding");
		return m_emptyOffMeshNavigation;
	}
}

void OffMeshNavigationManager::QueueCustomLinkAddition(const MNM::LinkAdditionRequest& request)
{
#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
	AILogCommentID("<MNM:OffMeshLink>", "OffMeshNavigationManager::QueueCustomLinkAddition for entity %x", request.requestOwner);
#endif
	m_operations.push_back(request);
}

void OffMeshNavigationManager::QueueCustomLinkRemoval(const MNM::LinkRemovalRequest& request)
{
#if DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS
	AILogCommentID("<MNM:OffMeshLink>", "OffMeshNavigationManager::QueueCustomLinkRemoval linkId %u for entity %x", request.linkId, request.requestOwner);
#endif
	m_operations.push_back(request);
}

bool OffMeshNavigationManager::IsLinkRemovalRequested(const MNM::OffMeshLinkID& linkID) const
{
	for (const MNM::OffMeshOperationRequestBase& requestBase : m_operations)
	{
		if ((requestBase.operationType == MNM::eOffMeshOperationType_Remove) && (requestBase.linkId == linkID))
		{
			return true;
		}
	}

	return false;
}

bool OffMeshNavigationManager::AddCustomLink(const NavigationMeshID& meshID, MNM::OffMeshLinkPtr& pLinkData, MNM::OffMeshLinkID& linkID, MNM::TriangleID* pOutStartTriangleID, MNM::TriangleID* pOutEndTriangleID, const bool bCloneLinkData)
{
	// Grab the navigation mesh
	NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(meshID);

	CRY_ASSERT(pLinkData.get() != nullptr);

	MNM::OffMeshLink& linkData = *pLinkData;

	// Query the entry/exit positions
	const Vec3 startPoint = linkData.GetStartPosition();
	const Vec3 endPoint = linkData.GetEndPosition();

	MNM::vector3_t fixedStartPoint(startPoint);
	MNM::vector3_t fixedEndPoint(endPoint);

	MNM::TriangleID startTriangleID, endTriangleID;

	const MNM::real_t range = MNM::real_t(1.0f);

	// Get entry triangle
	startTriangleID = mesh.navMesh.GetTriangleAt(fixedStartPoint, range, range, nullptr);

	if (!startTriangleID)
	{
		//Bail out; Entry position not connected to a triangle
		return false;
	}

	// Get entry triangle
	endTriangleID = mesh.navMesh.GetTriangleAt(fixedEndPoint, range, range, nullptr);

	if (!endTriangleID)
	{
		//Bail out; Exit position not connected to a triangle
		return false;
	}

	if (startTriangleID == endTriangleID)
	{
		// points are on the same triangle so don't bother
		return false;
	}

	// Select the corresponding off-mesh navigation object
	CRY_ASSERT_TRACE(m_offMeshMap.validate(meshID), ("Trying to add offmesh link to invalid mesh! (meshID = %u)", (uint32)meshID));
	MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshID];

	// Important: if we already added this particular link before, we need to remove it prior to re-adding it, as the startTriangleID might be
	//            a different one now and then the NavMesh would suddenly have 2 different triangle-links referring to the same offmesh-link.
	//            This can happen if there are multiple add-link requests in m_operations dealing with the same linkID but with *different* start/end positions (!)
	TLinkInfoMap::const_iterator it = m_links.find(linkID);
	if (it != m_links.end())
	{
		// we added this link before already, now see if the starting triangle changed
		if (it->second.startTriangleID != startTriangleID)
		{
			// gotcha! (notice that we end up here very rarely in practice)
			offMeshNavigation.RemoveLink(mesh, it->second.startTriangleID, linkID);
		}
	}

	// Register the new link with the off-mesh navigation system
	offMeshNavigation.AddLink(mesh, startTriangleID, endTriangleID, *&linkID);
	CRY_ASSERT_TRACE(linkID != MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID, ("Adding new offmesh link failed"));

	MNM::OffMeshLinkPtr pOffMeshLink;
	if (bCloneLinkData)
	{
		pOffMeshLink.reset(linkData.Clone());
	}
	else
	{
		pOffMeshLink = pLinkData;
	}
	pOffMeshLink->SetLinkID(linkID);

	m_links[linkID] = SLinkInfo(meshID, startTriangleID, endTriangleID, std::move(pOffMeshLink));

	gAIEnv.pNavigationSystem->AddOffMeshLinkIslandConnectionsBetweenTriangles(meshID, startTriangleID, endTriangleID, linkID);
	gAIEnv.pNavigationSystem->RequestUpdateMeshAccessibility(meshID);

	if (pOutStartTriangleID)
	{
		*pOutStartTriangleID = startTriangleID;
	}

	if (pOutEndTriangleID)
	{
		*pOutEndTriangleID = endTriangleID;
	}

	return true;
}

void OffMeshNavigationManager::RemoveCustomLink(const MNM::OffMeshLinkID& linkID)
{
	TLinkInfoMap::iterator linkIt = m_links.find(linkID);
	if (linkIt != m_links.end())
	{
		SLinkInfo& linkInfo = linkIt->second;

		// Select the corresponding off-mesh navigation object
		assert(m_offMeshMap.validate(linkInfo.meshID));
		MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[linkInfo.meshID];
		NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(linkInfo.meshID);

		// Remove link
		// NOTE: There should only ever be one element in the link array
		offMeshNavigation.RemoveLink(mesh, linkInfo.startTriangleID, linkID);

		gAIEnv.pNavigationSystem->RemoveOffMeshLinkIslandConnection(linkID);
		gAIEnv.pNavigationSystem->RequestUpdateMeshAccessibility(linkInfo.meshID);

		// Remove cached data
		m_links.erase(linkIt);
	}
}

void OffMeshNavigationManager::ProcessQueuedRequests()
{
	if (!m_operations.empty())
	{
		for (Operations::iterator it = m_operations.begin(); it != m_operations.end(); ++it)  // notice: the callbacks might add new operations, so we cannot simply cache end()
		{
			switch (it->operationType)
			{
			case MNM::eOffMeshOperationType_Add:
				{
					MNM::TriangleID startTriangleID = MNM::TriangleID(0);
					MNM::TriangleID endTriangleID = MNM::TriangleID(0);
					const bool linkGotSuccessfullyAdded = AddCustomLink(it->meshId, it->pLinkData, it->linkId, &startTriangleID, &endTriangleID, it->bCloneLinkData);
					if (it->callback)
					{
						it->callback(MNM::SOffMeshOperationCallbackData(it->linkId, startTriangleID, endTriangleID, linkGotSuccessfullyAdded));
					}
				}
				break;
			case MNM::eOffMeshOperationType_Remove:
				NotifyAllListenerAboutLinkDeletion(it->linkId);
				RemoveCustomLink(it->linkId);
				break;
			default:
				assert(0);
			}

		}

		stl::free_container(m_operations);
	}
}

void OffMeshNavigationManager::NotifyAllListenerAboutLinkDeletion(const MNM::OffMeshLinkID& linkID)
{
	for (Listeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnOffMeshLinkGoingToBeRemoved(linkID);
	}
}

MNM::OffMeshLink* OffMeshNavigationManager::GetOffMeshLink(const MNM::OffMeshLinkID& linkID)
{
	TLinkInfoMap::iterator linkIt = m_links.find(linkID);
	if (linkIt != m_links.end())
	{
		SLinkInfo& linkInfo = linkIt->second;
		return linkInfo.offMeshLink.get();
	}

	return NULL;
}

const MNM::OffMeshLink* OffMeshNavigationManager::GetOffMeshLink(const MNM::OffMeshLinkID& linkID) const
{
	TLinkInfoMap::const_iterator linkIt = m_links.find(linkID);
	if (linkIt != m_links.end())
	{
		const SLinkInfo& linkInfo = linkIt->second;
		return linkInfo.offMeshLink.get();
	}

	return NULL;
}

void OffMeshNavigationManager::RegisterListener(IOffMeshNavigationListener* pListener, const char* listenerName)
{
	m_listeners.Add(pListener, listenerName);
}

void OffMeshNavigationManager::UnregisterListener(IOffMeshNavigationListener* pListener)
{
	m_listeners.Remove(pListener);
}

void OffMeshNavigationManager::RemoveAllQueuedAdditionRequestForEntity(const EntityId requestOwner)
{
	m_operations.erase(std::remove_if(m_operations.begin(), m_operations.end(), MNM::IsOffMeshAdditionOperationRequestRelatedToEntity(requestOwner)), m_operations.end());
}

void OffMeshNavigationManager::RefreshConnections(const NavigationMeshID meshID, const MNM::TileID tileID)
{
	if (!m_offMeshMap.validate(meshID))
		return;

	MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshID];
	NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(meshID);

	//////////////////////////////////////////////////////////////////////////
	// Invalidate links for the tile.
	// Because the tile has been regenerated, all outgoing off-mesh links are invalid now
	offMeshNavigation.InvalidateLinks(tileID);

	//////////////////////////////////////////////////////////////////////////
	// Refresh all links tied to this mesh and tile
	// NOTE: This only re-adds them to the Tile with the cached data.
	TLinkInfoMap::iterator iter;
	for (iter = m_links.begin(); iter != m_links.end(); )
	{
		// Find out if object is bound to the modified mesh and tile
		SLinkInfo& linkInfo = iter->second;
		if (linkInfo.meshID != meshID)
		{
			++iter;
			continue;  // Mismatched mesh id
		}

		const bool isLinkOutgoingFromThisTile = (tileID == MNM::ComputeTileID(linkInfo.startTriangleID));
		const bool isLinkIncomingToThisTile = (tileID == MNM::ComputeTileID(linkInfo.endTriangleID));

		// is the link completely unrelated to given tile?
		if (!isLinkOutgoingFromThisTile && !isLinkIncomingToThisTile)
		{
			++iter;
			continue;
		}

		const MNM::OffMeshLinkID linkID = iter->first;

		// is the link incoming from a different tile?
		// -> need to remove it before it can potentially become a dangling link (i. e. the triangleID where it ends would no longer exist or would have morphed into a different one)
		//    (in fact, it *is* already a dangling link because given tile has just been regenerated and all its triangleIDs most likely have changed!)
		//    Notice that InvalidateLinks(tileID) only cares about outgoing links and also doesn't remove the link-information from the NavMesh, so we need
		//    to call RemoveLink() to get the NavMesh patched as well.
		if (isLinkIncomingToThisTile && !isLinkOutgoingFromThisTile)
		{
			offMeshNavigation.RemoveLink(mesh, linkInfo.startTriangleID, linkID);
		}

		// disconnect whatever islands were connected by the offmesh-link
		gAIEnv.pNavigationSystem->RemoveOffMeshLinkIslandConnection(linkID);

		// Try to re-connect everything later on, but only if it was not explicitly requested to be deleted by an outside source.
		// For example: if, at the exact same update the tile was refreshed but an outside source also requested the explicit
		// removal of a link then we should basically not re-apply the link.
		if (!IsLinkRemovalRequested(linkID))
		{
			MNM::LinkAdditionRequest request(linkInfo.offMeshLink->GetEntityIdForOffMeshLink(), meshID, linkInfo.offMeshLink, linkID);

			// Link data was already cloned when it was added the first time. If we clone it again, the game-side of the link have no way
			// to know, that link data was cloned (there is no event about refreshed connections). That means, if the game-side holds a
			// pointer to the link, it will become dangling and will lead to crash.
			request.bCloneLinkData = false;

			for (Listeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
			{
				notifier->OnOffMeshLinkGoingToBeRefreshed(request);
			}
			QueueCustomLinkAddition(request);
		}

		// But for the meantime, also remove the entry from m_links because the .triangleID member was referencing a triangle in the now regenerated tile (!)
		//
		// Carefully read this:
		//
		//    If we didn't do this, then remove-link requests and add-link requests in the m_operations queue could use that "dead" triangle reference
		//    to try to remove offmesh-links in the NavMesh where there was none, OR try to add offmesh-links in the NavMesh where there was one already, OR
		//    (even worse, as no assert will catch this scenario) it would manipulate *foreign* offmesh-links in the NavMesh.
		//
		m_links.erase(iter++);
	}

	for (Listeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnRefreshConnections(meshID, tileID);
	}
}

void OffMeshNavigationManager::Clear()
{
	m_offMeshMap.clear();
	m_links.clear();
	m_objectRegistrationEnabled = false;

	stl::free_container(m_operations);
}

void OffMeshNavigationManager::Enable()
{
	m_objectRegistrationEnabled = true;
}

void OffMeshNavigationManager::OnNavigationMeshCreated(const NavigationMeshID& meshID)
{
	assert(!m_offMeshMap.validate(meshID));

	m_offMeshMap.insert(meshID, MNM::OffMeshNavigation());
}

void OffMeshNavigationManager::OnNavigationMeshDestroyed(const NavigationMeshID& meshID)
{
	assert(m_offMeshMap.validate(meshID));

	m_offMeshMap.erase(meshID);

	// Remove all existing links for this mesh
	TLinkInfoMap::iterator iter;
	for (iter = m_links.begin(); iter != m_links.end(); )
	{
		// Find out if object is bound to the modified mesh and tile
		SLinkInfo& linkInfo = iter->second;
		if (linkInfo.meshID != meshID)
		{
			++iter;
			continue;  // Mismatched mesh id
		}

		const MNM::OffMeshLinkID& linkID = iter->first;
		NotifyAllListenerAboutLinkDeletion(linkID);

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

OffMeshNavigationManager::ProfileMemoryStats OffMeshNavigationManager::GetMemoryStats(ICrySizer* pSizer, const NavigationMeshID meshID) const
{
	const size_t initialSize = pSizer->GetTotalSize();
	ProfileMemoryStats memStats;

	// for all links: track memory of the ones with given mesh ID
	for (TLinkInfoMap::const_iterator it = m_links.begin(); it != m_links.end(); ++it)
	{
		const SLinkInfo& linkInfo = it->second;

		if (linkInfo.meshID == meshID)
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
