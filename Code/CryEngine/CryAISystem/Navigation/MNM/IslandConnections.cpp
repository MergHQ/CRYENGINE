// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "IslandConnections.h"
#include <CryAISystem/IPathfinder.h>
#include "../NavigationSystem/NavigationSystem.h"
#include "OffGridLinks.h"
#ifdef CRYAISYSTEM_DEBUG
	#include "DebugDrawContext.h"
#endif

namespace MNM
{

void IslandConnections::SetOneWayOffmeshConnectionBetweenIslands(const MNM::GlobalIslandID fromIslandId, const MNM::AreaAnnotation fromIslandAnnotation, const MNM::GlobalIslandID toIslandId, const MNM::AreaAnnotation toIslandAnnotation, const OffMeshLinkID offMeshLinkId, const TriangleID toTriangleId, const uint32 connectionObjectOwnerId)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);
	
	CRY_ASSERT(fromIslandId.GetNavigationMeshIDAsUint32() == toIslandId.GetNavigationMeshIDAsUint32());
	CRY_ASSERT(m_offMeshLinksMap.find(offMeshLinkId) == m_offMeshLinksMap.end(), "Trying to register the same offMeshLink (id = %u) twice", offMeshLinkId);

	m_offMeshLinksMap.emplace(offMeshLinkId, SOffMeshLinkData(fromIslandId, toTriangleId, connectionObjectOwnerId));

	// Connection between two islands consists always of two links for having possibility of reversed search.
	const int singleConnection = 1;
	const int noConnection = 0;

	SIslandNode& fromIslandNode = m_islandConnections[fromIslandId];
	fromIslandNode.annotation = fromIslandAnnotation;
	ModifyConnectionToIsland(fromIslandNode, toIslandId.GetStaticIslandID(), toIslandAnnotation, offMeshLinkId, singleConnection, noConnection);

	SIslandNode& toIslandNode = m_islandConnections[toIslandId];
	toIslandNode.annotation = toIslandAnnotation;
	ModifyConnectionToIsland(toIslandNode, fromIslandId.GetStaticIslandID(), fromIslandAnnotation, offMeshLinkId, noConnection, singleConnection);
}

void IslandConnections::SetTwoWayConnectionBetweenIslands(const MNM::GlobalIslandID islandId1, const MNM::AreaAnnotation islandAnnotation1, const MNM::GlobalIslandID islandId2, const MNM::AreaAnnotation islandAnnotation2, const int connectionsChange)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);
	
	CRY_ASSERT(islandId1.GetNavigationMeshIDAsUint32() == islandId2.GetNavigationMeshIDAsUint32());

	// Connection between two islands consists always of two links for having possibility of reversed search.

	SIslandNode& islandNode1 = m_islandConnections[islandId1];
	SIslandNode& islandNode2 = m_islandConnections[islandId2];
	islandNode1.annotation = islandAnnotation1;
	islandNode2.annotation = islandAnnotation2;

	if (connectionsChange != 0)
	{
		ModifyConnectionToIsland(islandNode1, islandId2.GetStaticIslandID(), islandAnnotation2, OffMeshLinkID(), connectionsChange, connectionsChange);
		if (islandNode1.links.empty())
		{
			// if connectionsChange was negative, link could be removed in SetConnectionToIsland
			m_islandConnections.erase(islandId1);
		}

		ModifyConnectionToIsland(islandNode2, islandId1.GetStaticIslandID(), islandAnnotation1, OffMeshLinkID(), connectionsChange, connectionsChange);
		if (islandNode2.links.empty())
		{
			// if connectionsChange was negative, link could be removed in SetConnectionToIsland
			m_islandConnections.erase(islandId2);
		}
	}
}

void IslandConnections::ModifyConnectionToIsland(
	SIslandNode& fromIslandNode,
	const MNM::StaticIslandID toIslandId,
	const MNM::AreaAnnotation toIslandAnnotation,
	const OffMeshLinkID offMeshLinkId,
	const int outConnectionsCountChange,
	const int inConnectionsCountChange)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);
	
	auto findIt = std::find(fromIslandNode.links.begin(), fromIslandNode.links.end(), SIslandLink(toIslandId, offMeshLinkId));
	if (findIt != fromIslandNode.links.end())
	{
		SIslandLink& link = *findIt;

		const bool hasOutConnections = link.outConnectionsCount > 0;
		const bool hasInConnections = link.inConnectionsCount > 0;

		CRY_ASSERT(link.outConnectionsCount >= -outConnectionsCountChange && link.inConnectionsCount >= -inConnectionsCountChange);

		link.outConnectionsCount += outConnectionsCountChange;
		link.inConnectionsCount += inConnectionsCountChange;

		if (link.outConnectionsCount == 0 && link.inConnectionsCount == 0)
		{
			fromIslandNode.links.erase(findIt);
			fromIslandNode.outLinksCount--;
			fromIslandNode.inLinksCount--;
		}
		else
		{
			if (hasOutConnections && link.outConnectionsCount == 0)
			{
				fromIslandNode.outLinksCount--;
			}
			else if (hasInConnections && link.inConnectionsCount == 0)
			{
				fromIslandNode.inLinksCount--;
			}
		}
	}
	else
	{
		CRY_ASSERT(outConnectionsCountChange > 0 || inConnectionsCountChange);

		fromIslandNode.links.emplace_back(SIslandLink(toIslandId, offMeshLinkId, toIslandAnnotation, outConnectionsCountChange, inConnectionsCountChange));
		if (outConnectionsCountChange > 0)
		{
			fromIslandNode.outLinksCount++;
		}
		if (inConnectionsCountChange > 0)
		{
			fromIslandNode.inLinksCount++;
		}
	}
}

void IslandConnections::RemoveOffMeshLinkConnection(const OffMeshLinkID offMeshLinkId)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);
	
	auto findOffmeshIt = m_offMeshLinksMap.find(offMeshLinkId);
	if (findOffmeshIt == m_offMeshLinksMap.end())
		return;

	MNM::GlobalIslandID toIslandId = MNM::Constants::eGlobalIsland_InvalidIslandID;

	const SOffMeshLinkData& offMeshLinkData = findOffmeshIt->second;
	auto findIt = m_islandConnections.find(offMeshLinkData.fromIslandId);
	if (findIt != m_islandConnections.end())
	{
		SIslandNode& islandNode = findIt->second;
		for (size_t i = 0, count = islandNode.links.size(); i < count; ++i)
		{
			if (islandNode.links[i].offMeshLinkID != offMeshLinkId)
				continue;

			toIslandId = MNM::GlobalIslandID(offMeshLinkData.fromIslandId.GetNavigationMeshIDAsUint32(), islandNode.links[i].toIsland);
			
			if (i < count - 1)
			{
				islandNode.links[i] = islandNode.links[count - 1];
			}
			islandNode.links.pop_back();
			islandNode.outLinksCount--;

			if (count == 1)
			{
				m_islandConnections.erase(findIt);
			}
			break;
		}
	}

	CRY_ASSERT(!(toIslandId == MNM::Constants::eGlobalIsland_InvalidIslandID));
	findIt = m_islandConnections.find(toIslandId);
	if (findIt != m_islandConnections.end())
	{
		SIslandNode& islandNode = findIt->second;
		for (size_t i = 0, count = islandNode.links.size(); i < count; ++i)
		{
			if (islandNode.links[i].offMeshLinkID != offMeshLinkId)
				continue;

			if (i < count - 1)
			{
				islandNode.links[i] = islandNode.links[count - 1];
			}
			islandNode.links.pop_back();
			islandNode.inLinksCount--;

			if (count == 1)
			{
				m_islandConnections.erase(findIt);
			}
			break;
		}
	}

	m_offMeshLinksMap.erase(findOffmeshIt);
}

void IslandConnections::GetConnectedIslands(const MNM::GlobalIslandID seedIslandId, ConnectedIslandsArray& connectedIslandsArray) const
{
	const NavigationMeshID meshId(seedIslandId.GetNavigationMeshIDAsUint32());
	TIslandNodeStatesArray& islandNodeStates = PrepareIslandNodeStatesArray(seedIslandId);

	ConnectedIslandsArray& openList = connectedIslandsArray;
	openList.clear();
	openList.push_back(seedIslandId.GetStaticIslandID());

	size_t currentIdx = 0;
	while (currentIdx < openList.size())
	{
		const MNM::StaticIslandID currentStaticID = openList[currentIdx++];
		const MNM::GlobalIslandID currentGlobalID(meshId, currentStaticID);
		islandNodeStates[currentStaticID] = EIslandNodeState::Closed;

		TIslandConnectionsMap::const_iterator currentIslandIt = m_islandConnections.find(currentGlobalID);
		if (currentIslandIt == m_islandConnections.end())
		{
			continue;
		}

		const SIslandNode& islandNode = currentIslandIt->second;

		for (const SIslandLink& link : islandNode.links)
		{
			if (!link.IsOutgoing())
				continue;

			if (islandNodeStates[link.toIsland] != EIslandNodeState::None)
				continue;

			islandNodeStates[link.toIsland] = EIslandNodeState::OpenedForward;

			openList.push_back(link.toIsland);
		}
	}
}

struct SAcceptAllIslandsFilter
{
	inline bool PassFilter(const MNM::AreaAnnotation&) const { return true; }
};

bool IslandConnections::CanNavigateBetweenIslands(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, const INavMeshQueryFilter* pFilter) const
{
	if (pFilter)
	{
		return CanNavigateBetweenIslandsInternal(pEntityToTestOffGridLinks, fromIsland, toIsland, *pFilter);
	}
	else
	{
		SAcceptAllIslandsFilter filter;
		return CanNavigateBetweenIslandsInternal(pEntityToTestOffGridLinks, fromIsland, toIsland, filter);
	}
}

template <typename TFilter>
bool IslandConnections::CanNavigateBetweenIslandsInternal(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, const TFilter& filter) const
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);

	const EntityId entityIdToTestOffMeshLinks = pEntityToTestOffGridLinks ? pEntityToTestOffGridLinks->GetId() : INVALID_ENTITYID;

	IF_UNLIKELY(fromIsland == MNM::Constants::eGlobalIsland_InvalidIslandID || toIsland == MNM::Constants::eGlobalIsland_InvalidIslandID)
		return false;

	if (fromIsland == toIsland)
		return true;

	const OffMeshNavigationManager* offMeshNavigationManager = gAIEnv.pNavigationSystem->GetOffMeshNavigationManager();
	CRY_ASSERT(offMeshNavigationManager);

	if (fromIsland.GetNavigationMeshIDAsUint32() != toIsland.GetNavigationMeshIDAsUint32())
		return false; // Island connections between two different meshes isn't supported

	TIslandNodeStatesArray& islandNodeStates = PrepareIslandNodeStatesArray(fromIsland);

	TIslandConnectionsMap::const_iterator fromIslandIt = m_islandConnections.find(fromIsland);
	TIslandConnectionsMap::const_iterator toIslandIt = m_islandConnections.find(toIsland);
	if (fromIslandIt == m_islandConnections.end() || toIslandIt == m_islandConnections.end())
		return false;

	const NavigationMeshID meshId(fromIsland.GetNavigationMeshIDAsUint32());

	const SIslandNode& fromIslandNode = fromIslandIt->second;
	const SIslandNode& toIslandNode = toIslandIt->second;
	size_t uberIslandSize = 64;

	/*
	Search algorithm keeps two open lists of open nodes - one for forward search and one for reversed search. 
	Connection between islands is found when any of the expanded nodes expanded from the current node is already opened in different open list.
	If a node with very big number of outgoing links is found, search is reversed to prevent expanding such node. 
	*/

	/* Algorithm explanation in more human words:
	Typically, when the level is represented by the island graph we will have some island nodes that have a lot of connections. 
	These are usually central places on the map that provide connectivity in all directions. We will name these 'uber islands'. 
	The idea behind this algorithm is to try and postpone opening 'uber island nodes' (or not opening at all) as long as possible because it would mean 
	we would need to put all outgoing links of such an island in the open list. But, as this algorithm is only interested in answering 
	the question IF a connection exists, we will probably find answers sooner by trying out nodes that do not have so many outgoing links. 
	We do this by using 2 open lists: one for the start node and one for the end node. If we detect an 'uber island' 
	(based on some heuristic on the amount of connections of an island), we will continue the search 'in the other direction' by switching 
	to the other open list and continue the flooding from there. The initial heuristic for an 'uber island' is some 'large' value. 
	If we find island that have even more connections then this number will be used instead.
	*/

	TIslandOpenList openLists[2];

	openLists[EIslandNodeState::OpenedForward].push_back(fromIsland);
	islandNodeStates[fromIsland.GetStaticIslandID()] = EIslandNodeState::OpenedForward;

	openLists[EIslandNodeState::OpenedReversed].push_back(toIsland);
	islandNodeStates[toIsland.GetStaticIslandID()] = EIslandNodeState::OpenedReversed;

	EIslandNodeState currentSearchState = fromIslandNode.outLinksCount <= toIslandNode.inLinksCount ? EIslandNodeState::OpenedForward : EIslandNodeState::OpenedReversed;
	TIslandOpenList& openList = openLists[currentSearchState];

	while (!openList.empty())
	{
		const MNM::GlobalIslandID currentGlobalID = openList.front();
		openList.pop_front();

		const MNM::StaticIslandID currentStaticID = currentGlobalID.GetStaticIslandID();
		islandNodeStates[currentStaticID] = EIslandNodeState::Closed;
		
		TIslandConnectionsMap::const_iterator currentIslandConnectionsIt = m_islandConnections.find(currentGlobalID);
		if (currentIslandConnectionsIt == m_islandConnections.end())
		{
			CRY_ASSERT(0);
			continue;
		}

		const SIslandNode& islandNode = currentIslandConnectionsIt->second;

		if (currentSearchState == EIslandNodeState::OpenedReversed)
		{
			if (islandNode.inLinksCount > uberIslandSize)
			{
				// Current node has too many in-going links, push it back to open list and reverse the search
				uberIslandSize = islandNode.inLinksCount;
				
				openList.push_back(MNM::GlobalIslandID(meshId, currentStaticID));
				islandNodeStates[currentStaticID] = currentSearchState;

				currentSearchState = EIslandNodeState::OpenedForward;
				openList = openLists[EIslandNodeState::OpenedForward];
				continue;
			}
			
			// Is current node accessible with provided filter?
			if (!filter.PassFilter(islandNode.annotation))
				continue;

			for (const SIslandLink& link : islandNode.links)
			{
				// Expand only to in-going links in reversed search
				if (!link.IsIngoing())
					continue;

				const MNM::StaticIslandID toIslandStaticID = link.toIsland;
				const EIslandNodeState toNodeState = islandNodeStates[toIslandStaticID];
				
				// Expand only to nodes that weren't visited yet
				if (toNodeState == EIslandNodeState::Closed || toNodeState == EIslandNodeState::OpenedReversed)
					continue;

				if (link.offMeshLinkID != 0)
				{
					MNM::AreaAnnotation linkAnnotation;
					const IOffMeshLink* pOffMeshLink = offMeshNavigationManager->GetLinkAndAnnotation(link.offMeshLinkID, linkAnnotation);
					if(!pOffMeshLink || !filter.PassFilter(linkAnnotation))
						continue;

					if(entityIdToTestOffMeshLinks && !pOffMeshLink->CanUse(entityIdToTestOffMeshLinks, nullptr))
						continue;
				}

				// Is the node already opened in opposite search? 
				if (toNodeState == EIslandNodeState::OpenedForward)
					return true;

				openList.push_back(MNM::GlobalIslandID(meshId, link.toIsland));
				islandNodeStates[toIslandStaticID] = EIslandNodeState::OpenedReversed;
			}
		}
		else
		{
			if (islandNode.outLinksCount > uberIslandSize)
			{
				// Current node has too many out-going links, push it back to open list and reverse the search
				uberIslandSize = islandNode.outLinksCount;
				
				openList.push_back(MNM::GlobalIslandID(meshId, currentStaticID));
				islandNodeStates[currentStaticID] = currentSearchState;

				currentSearchState = EIslandNodeState::OpenedReversed;
				openList = openLists[EIslandNodeState::OpenedReversed];
				continue;
			}
			
			for (const SIslandLink& link : islandNode.links)
			{
				// Expand only to out-going links in forward search
				if (!link.IsOutgoing())
					continue;

				const MNM::StaticIslandID toIslandStaticID = link.toIsland;
				const EIslandNodeState toNodeState = islandNodeStates[toIslandStaticID];
				
				// Expand only to nodes that weren't visited yet
				if (toNodeState == EIslandNodeState::Closed || toNodeState == EIslandNodeState::OpenedForward)
					continue;

				// Is the next node accessible with provided filter?
				if (!filter.PassFilter(link.toIslandAnnotation))
					continue;

				if (link.offMeshLinkID != 0)
				{
					MNM::AreaAnnotation linkAnnotation;
					const IOffMeshLink* pOffMeshLink = offMeshNavigationManager->GetLinkAndAnnotation(link.offMeshLinkID, linkAnnotation);
					if (!pOffMeshLink || !filter.PassFilter(linkAnnotation))
						continue;

					if(entityIdToTestOffMeshLinks && !pOffMeshLink->CanUse(entityIdToTestOffMeshLinks, nullptr))
						continue;
				}

				// Is the node already opened in opposite search? 
				if (toNodeState == EIslandNodeState::OpenedReversed)
					return true;

				openList.push_back(MNM::GlobalIslandID(meshId, link.toIsland));
				islandNodeStates[toIslandStaticID] = EIslandNodeState::OpenedForward;
			}
		}
	}

	// One of the open list was emptied without finding a connection
	return false;
}

IslandConnections::TIslandNodeStatesArray& IslandConnections::PrepareIslandNodeStatesArray(const MNM::GlobalIslandID fromIsland) const
{
	const NavigationMeshID fromMeshId(fromIsland.GetNavigationMeshIDAsUint32());
	const uint32 maxIslandIdx = gAIEnv.pNavigationSystem->GetMesh(fromMeshId).navMesh.GetIslands().GetTotalIslands() + 1;

	if (m_islandNodeStatesArrayCache.size() < maxIslandIdx)
	{
		m_islandNodeStatesArrayCache.resize(maxIslandIdx);
	}
	std::fill(m_islandNodeStatesArrayCache.begin(), m_islandNodeStatesArrayCache.begin() + maxIslandIdx, EIslandNodeState::None);

	return m_islandNodeStatesArrayCache;
}

void IslandConnections::Reset()
{
	m_islandConnections.clear();
	m_offMeshLinksMap.clear();
	m_islandNodeStatesArrayCache.clear();
}

void IslandConnections::ResetForMesh(const NavigationMeshID meshId)
{
	auto it = m_islandConnections.begin();
	while (it != m_islandConnections.end())
	{
		if (it->first.GetNavigationMeshIDAsUint32() != meshId)
		{
			it++;
			continue;
		}
		const SIslandNode& node = it->second;
		for (const SIslandLink& link : node.links)
		{
			if (link.offMeshLinkID.IsValid())
			{
				m_offMeshLinksMap.erase(link.offMeshLinkID);
			}
		}
		it = m_islandConnections.erase(it);
	}
}

#ifdef CRYAISYSTEM_DEBUG

void IslandConnections::DebugDraw() const
{
	const size_t totalGlobalIslandsRegisterd = m_islandConnections.size();
	const size_t memoryUsedByALink = sizeof(SIslandLink);
	size_t linksCount = 0;
	size_t linksCapacity = 0;
	size_t totalUsedMemory = 0;
	size_t totalEffectiveMemory = 0;
	TIslandConnectionsMap::const_iterator linksIt = m_islandConnections.begin();
	TIslandConnectionsMap::const_iterator linksEnd = m_islandConnections.end();
	for (; linksIt != linksEnd; ++linksIt)
	{
		const SIslandNode& islandNode = linksIt->second;
		linksCount += islandNode.links.size();
		linksCapacity += islandNode.links.capacity();
	}

	totalUsedMemory += linksCount * memoryUsedByALink;
	totalEffectiveMemory += linksCapacity * memoryUsedByALink;

	totalUsedMemory += totalGlobalIslandsRegisterd * sizeof(SIslandNode);
	totalEffectiveMemory += totalGlobalIslandsRegisterd * sizeof(SIslandNode);

	totalUsedMemory += sizeof(TIslandNodeStatesArray::value_type) * m_islandNodeStatesArrayCache.size();
	totalEffectiveMemory += sizeof(TIslandNodeStatesArray::value_type) * m_islandNodeStatesArrayCache.capacity();

	totalUsedMemory += sizeof(TIslandOffMeshLinkDataMap::value_type) * m_offMeshLinksMap.size();
	totalEffectiveMemory += sizeof(TIslandOffMeshLinkDataMap::value_type) * m_offMeshLinksMap.size();

	const float conversionValue = 1024.0f;
	CDebugDrawContext dc;
	dc->Draw2dLabel(10.0f, 5.0f, 1.6f, Col_White, false, "Amount of global islands registered into the system %d", (int)totalGlobalIslandsRegisterd);
	dc->Draw2dLabel(10.0f, 25.0f, 1.6f, Col_White, false, "Ammount of links registered into the system %d", (int)linksCount);
	dc->Draw2dLabel(10.0f, 45.0f, 1.6f, Col_White, false, "MNM::IslandConnections total used memory %.2fKB", totalUsedMemory / conversionValue);
	dc->Draw2dLabel(10.0f, 65.0f, 1.6f, Col_White, false, "MNM::IslandConnections total effective memory %.2fKB", totalEffectiveMemory / conversionValue);
}

#endif

} //namespace MNM
