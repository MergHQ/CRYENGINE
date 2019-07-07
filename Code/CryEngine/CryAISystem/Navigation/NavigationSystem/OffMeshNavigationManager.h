// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __OFFMESH_NAVIGATION_MANAGER_H__
#define __OFFMESH_NAVIGATION_MANAGER_H__

#pragma once

#include "Navigation/MNM/OffGridLinks.h"

#include <CryAISystem/INavigationSystem.h>
#include <CryAISystem/IOffMeshNavigationManager.h>
#include <CryAISystem/NavigationSystem/Annotation.h>

//////////////////////////////////////////////////////////////////////////
/// OffMesh Navigation Manager
/// Keeps track of off-mesh navigation objects associated to each navigation mesh
///
/// Interfaces also with the Navigation System, to update the right objects
/// when meshes are created, updated or deleted
//////////////////////////////////////////////////////////////////////////
class OffMeshNavigationManager : public IOffMeshNavigationManager
{
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

	const MNM::OffMeshNavigation& GetOffMeshNavigationForMesh(const NavigationMeshID& meshId) const;

	void                          RefreshConnections(const NavigationMeshID meshId, const MNM::TileID tileId);
	void                          Clear();
	void                          Enable();
	bool                          IsLinkAdditionEnabled() const { return m_objectRegistrationEnabled; }

	void                          OnNavigationMeshCreated(const NavigationMeshID& meshId);
	void                          OnNavigationMeshDestroyed(const NavigationMeshID& meshId);
	void                          OnNavigationLoadedComplete();

	// IOffMeshNavigationManager
	virtual MNM::OffMeshLinkID      RequestLinkAddition(const MNM::OffMeshLinkID linkId, const NavigationAgentTypeID agentTypeId, _smart_ptr<MNM::IOffMeshLink> pLinkData, const MNM::SOffMeshLinkCallbacks& callbacks) override;
	virtual void                    RequestLinkRemoval(const MNM::OffMeshLinkID linkId) override;
	virtual bool                    CancelLinkAddition(const MNM::OffMeshLinkID linkId) override;

	virtual MNM::IOffMeshLink*       GetOffMeshLink(const MNM::OffMeshLinkID linkId) override;
	virtual const MNM::IOffMeshLink* GetOffMeshLink(const MNM::OffMeshLinkID linkId) const override;

	virtual void                    RequestLinkAnnotationChange(const MNM::OffMeshLinkID linkId, const MNM::AreaAnnotation annotation) override;
	virtual const MNM::IOffMeshLink* GetLinkAndAnnotation(const MNM::OffMeshLinkID linkId, MNM::AreaAnnotation& annotation) const override;
	// ~IOffMeshNavigationManager

	void ProcessQueuedRequests();

#if DEBUG_MNM_ENABLED
	ProfileMemoryStats GetMemoryStats(ICrySizer* pSizer, const NavigationMeshID meshId) const;
#endif

private:
	struct SLinkAdditionRequest
	{
		SLinkAdditionRequest() {}

		explicit SLinkAdditionRequest(const NavigationAgentTypeID agentTypeId, MNM::IOffMeshLink_AutoPtr pLinkDataPtr, const MNM::OffMeshLinkID linkId, const MNM::SOffMeshLinkCallbacks& callbacks)
			: linkId(linkId)
			, agentTypeId(agentTypeId)
			, pLinkData(pLinkDataPtr)
			, callbacks(callbacks)
		{}

		MNM::OffMeshLinkID         linkId;
		NavigationAgentTypeID      agentTypeId;
		MNM::IOffMeshLink_AutoPtr  pLinkData;
		MNM::SOffMeshLinkCallbacks callbacks;
	};

	struct SAnnotationChangeRequest
	{
		explicit SAnnotationChangeRequest(const MNM::OffMeshLinkID linkId, const MNM::AreaAnnotation annotation)
			: linkId(linkId)
			, annotation(annotation)
		{}
		MNM::OffMeshLinkID linkId;
		MNM::AreaAnnotation annotation;
	};

	struct SLinkControlInfo
	{
		explicit SLinkControlInfo(
			const NavigationMeshID meshId, const MNM::TriangleID startTriangleId, const MNM::TriangleID endTriangleId, const MNM::AreaAnnotation annotation, 
			MNM::IOffMeshLink_AutoPtr pOffmeshLink, 
			const MNM::SOffMeshLinkCallbacks& callbacks)
			: meshID(meshId)
			, startTriangleID(startTriangleId)
			, endTriangleID(endTriangleId)
			, annotation(annotation)
			, offMeshLink(pOffmeshLink)
			, removedCallback(callbacks.removedCallback)
		{}

		bool operator==(const NavigationMeshID other) const
		{
			CRY_ASSERT(false);
			return (meshID == other);
		}

		NavigationMeshID           meshID;
		MNM::TriangleID            startTriangleID;
		MNM::TriangleID            endTriangleID;
		MNM::AreaAnnotation        annotation;
		MNM::IOffMeshLink_AutoPtr  offMeshLink;
		MNM::SOffMeshLinkCallbacks::RemovalCallback removedCallback;
	};

	// For every navigation mesh, there will be always an off-mesh navigation object
	typedef id_map<uint32, MNM::OffMeshNavigation> TOffMeshMap;
	typedef std::unordered_map<MNM::OffMeshLinkID, SLinkControlInfo> TLinkInfoMap;
	typedef std::vector<SLinkAdditionRequest> AdditionRequests;
	typedef std::vector<MNM::OffMeshLinkID> RemovalRequests;
	typedef std::vector<SAnnotationChangeRequest> AnnotationChangeRequests;

	bool       ProcessLinkAdditionRequest(const SLinkAdditionRequest& request);
	void       ProcessLinkRemovalRequest(const MNM::OffMeshLinkID linkId);
	void       ProcessAnnotationChangeRequest(const SAnnotationChangeRequest& request);

	MNM::EOffMeshLinkAdditionResult TryCreateLink(const SLinkAdditionRequest& request, MNM::IOffMeshLink::SNavigationData& navigationLinkData);
	MNM::EOffMeshLinkAdditionResult GetNavigationLinkDataForAdditionRequest(const MNM::SOffMeshLinkCallbacks& callbacks, const NavigationAgentTypeID agentTypeId, MNM::IOffMeshLink* pLinkData, MNM::IOffMeshLink::SNavigationData& navigationLinkData) const;

	bool       IsLinkAdditionRequested(const MNM::OffMeshLinkID linkId) const;
	bool       IsLinkRemovalRequested(const MNM::OffMeshLinkID linkId) const;
	
	TOffMeshMap            m_offMeshMap;
	MNM::OffMeshNavigation m_emptyOffMeshNavigation;

	// List of registered links
	TLinkInfoMap m_links;
	bool         m_objectRegistrationEnabled;

	AdditionRequests m_additionRequests;
	RemovalRequests m_removalRequests;
	AnnotationChangeRequests m_annotationChangeRequests;

	size_t m_additionProcessingIdx;
	size_t m_removalProcessingIdx;

#ifdef USE_CRY_ASSERT
	MNM::OffMeshLinkID m_debugRemovingLinkId;
#endif // USE_CRY_ASSERT

};

#endif  //__OFFMESH_NAVIGATION_MANAGER_H__
