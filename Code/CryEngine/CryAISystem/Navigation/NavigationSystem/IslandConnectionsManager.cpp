// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "IslandConnectionsManager.h"

IslandConnectionsManager::IslandConnectionsManager()
{
}

void IslandConnectionsManager::Reset()
{
	m_globalIslandConnections.Reset();
}

MNM::IslandConnections& IslandConnectionsManager::GetIslandConnections()
{
	return m_globalIslandConnections;
}

bool IslandConnectionsManager::AreIslandsConnected(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID startIsland, const MNM::GlobalIslandID endIsland, const INavMeshQueryFilter* pFilter) const
{
	return m_globalIslandConnections.CanNavigateBetweenIslands(pEntityToTestOffGridLinks, startIsland, endIsland, pFilter);
}

#ifdef CRYAISYSTEM_DEBUG

void IslandConnectionsManager::DebugDraw() const
{
	m_globalIslandConnections.DebugDraw();
}

#endif
