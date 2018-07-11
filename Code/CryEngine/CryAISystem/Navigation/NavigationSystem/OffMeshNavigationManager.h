// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __OFFMESH_NAVIGATION_MANAGER_H__
#define __OFFMESH_NAVIGATION_MANAGER_H__

#pragma once

#include "Navigation/MNM/OffGridLinks.h"

#include <CryAISystem/INavigationSystem.h>
#include <CryAISystem/IOffMeshNavigationManager.h>

//////////////////////////////////////////////////////////////////////////
/// OffMesh Navigation Manager
/// Keeps track of off-mesh navigation objects associated to each navigation mesh
///
/// Interfaces also with the Navigation System, to update the right objects
/// when meshes are created, updated or deleted
//////////////////////////////////////////////////////////////////////////
class OffMeshNavigationManager : public IOffMeshNavigationManager
{
private:
	struct SLinkInfo
	{
		SLinkInfo() {}
		SLinkInfo(NavigationMeshID _meshId, MNM::TriangleID _startTriangleID, MNM::TriangleID _endTriangleID, MNM::OffMeshLinkPtr _offMeshLink)
			: meshID(_meshId)
			, startTriangleID(_startTriangleID)
			, endTriangleID(_endTriangleID)
			, offMeshLink(_offMeshLink)
		{}

		bool operator==(const NavigationMeshID _meshID) const
		{
			return (meshID == _meshID);
		}

		NavigationMeshID    meshID;
		MNM::TriangleID     startTriangleID;
		MNM::TriangleID     endTriangleID;
		MNM::OffMeshLinkPtr offMeshLink;
	};

public:
#if DEBUG_MNM_ENABLED
	struct ProfileMemoryStats
	{
		ProfileMemoryStats()
			: linkInfoSize(0)
			, totalSize(0)
		{
		}

		size_t linkInfoSize;      // SLinkInfo
		size_t totalSize;         // currently redundant, but good for easier extension in the future
	};
#endif

public:
	OffMeshNavigationManager(const int offMeshMapSize);

	const MNM::OffMeshNavigation& GetOffMeshNavigationForMesh(const NavigationMeshID& meshID) const;

	void                    RefreshConnections(const NavigationMeshID meshID, const MNM::TileID tileID);
	void                    Clear();
	void                    Enable();

	void                    OnNavigationMeshCreated(const NavigationMeshID& meshID);
	void                    OnNavigationMeshDestroyed(const NavigationMeshID& meshID);
	void                    OnNavigationLoadedComplete();

	// IOffMeshNavigationManager
	virtual void                    QueueCustomLinkAddition(const MNM::LinkAdditionRequest& request) override;
	virtual void                    QueueCustomLinkRemoval(const MNM::LinkRemovalRequest& request) override;
	virtual MNM::OffMeshLink*       GetOffMeshLink(const MNM::OffMeshLinkID& linkID) override;
	virtual const MNM::OffMeshLink* GetOffMeshLink(const MNM::OffMeshLinkID& linkID) const override;
	virtual void                    RegisterListener(IOffMeshNavigationListener* pListener, const char* listenerName) override;
	virtual void                    UnregisterListener(IOffMeshNavigationListener* pListener) override;
	virtual void                    RemoveAllQueuedAdditionRequestForEntity(const EntityId requestOwner) override;
	virtual bool                    IsRegisteringEnabled() const override { return m_objectRegistrationEnabled; }
	// ~IOffMeshNavigationManager

	void ProcessQueuedRequests();

#if DEBUG_MNM_ENABLED
	ProfileMemoryStats GetMemoryStats(ICrySizer* pSizer, const NavigationMeshID meshID) const;
#endif

private:
	bool       AddCustomLink(const NavigationMeshID& meshID, MNM::OffMeshLinkPtr& pLinkData, MNM::OffMeshLinkID& linkID, MNM::TriangleID* pOutStartTriangleID, MNM::TriangleID* pOutEndTriangleID, const bool bCloneLinkData);
	void       RemoveCustomLink(const MNM::OffMeshLinkID& linkID);

	bool       IsLinkRemovalRequested(const MNM::OffMeshLinkID& linkID) const;

	ILINE bool CanRegisterObject() const { return m_objectRegistrationEnabled; };

	void       NotifyAllListenerAboutLinkDeletion(const MNM::OffMeshLinkID& linkID);

	// For every navigation mesh, there will be always an off-mesh navigation object
	typedef id_map<uint32, MNM::OffMeshNavigation> TOffMeshMap;
	TOffMeshMap            m_offMeshMap;

	MNM::OffMeshNavigation m_emptyOffMeshNavigation;

	// List of registered links
	typedef std::map<MNM::OffMeshLinkID, SLinkInfo> TLinkInfoMap;
	TLinkInfoMap m_links;

	bool         m_objectRegistrationEnabled;

	typedef std::list<MNM::OffMeshOperationRequestBase> Operations; // it's a list to safely allow queuing further operations from their callbacks while the queue is being processed
	Operations m_operations;

	typedef CListenerSet<IOffMeshNavigationListener*> Listeners;
	Listeners m_listeners;
};

#endif  //__OFFMESH_NAVIGATION_MANAGER_H__
