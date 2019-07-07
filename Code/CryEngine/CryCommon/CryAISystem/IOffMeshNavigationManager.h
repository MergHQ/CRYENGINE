// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "NavigationSystem/NavigationIdTypes.h"
#include "NavigationSystem/MNMBaseTypes.h"
#include "NavigationSystem/OffMeshLink.h"

namespace MNM
{

enum class EOffMeshLinkRemovalReason
{
	MeshDestroyed,
	MeshUpdated,
	Count,
};

enum class EOffMeshLinkAdditionResult
{
	Success,
	InvalidStartTriangle,
	InvalidEndTriangle,
	NotSameMesh,
	InvalidMesh,
	AlreadyExists,
	AcquireCallbackNotSet,
	AcquireDataFailed,
	Count,
};

//! Function parameter for link addition callbacks.
struct SOffMeshAdditionCallbackData
{
	const MNM::TriangleID    startTriangleID;
	const MNM::TriangleID    endTriangleID;
	const EOffMeshLinkAdditionResult result;

	explicit SOffMeshAdditionCallbackData(const MNM::TriangleID startTriangleId, const MNM::TriangleID endTriangleId, const EOffMeshLinkAdditionResult result)
		: startTriangleID(startTriangleId)
		, endTriangleID(endTriangleId)
		, result(result)
	{}
};

struct SOffMeshLinkCallbacks
{
	typedef Functor3wRet<NavigationAgentTypeID, MNM::IOffMeshLink*, MNM::IOffMeshLink::SNavigationData&, EOffMeshLinkAdditionResult> AcquireDataCallback;
	typedef Functor2<MNM::OffMeshLinkID, const MNM::SOffMeshAdditionCallbackData&> AdditionCallback;
	typedef Functor3<MNM::OffMeshLinkID, NavigationMeshID, MNM::EOffMeshLinkRemovalReason> RemovalCallback;
	
	//! Called to the user code when the addition operation is being processed to retrieve navigation information of the link. When link annotation isn't set default one is used.
	AcquireDataCallback acquireDataCallback;

	//! Called after the addition operation is completed (either success or fail)
	AdditionCallback additionCompletedCallback;
	
	//! Called just before the link is removed internally from the Off-Mesh Navigation due to NavMesh removal or tile update. User code should clean up all references to cached link data.
	RemovalCallback removedCallback;
};

}

//! Interface class for OffMesh Navigation Manager
//!
//! OffMeshNavigationManager is used for registering and unregistering off-mesh links. 
//! Since the class can alter NavMesh data, link additions, removals and annotation changes aren't executed instantly 
//! but are queued instead and performed at the right time when it is safe to do so.
struct IOffMeshNavigationManager
{
	virtual ~IOffMeshNavigationManager() {}

	//! Queue link addition request
	//! It is safe to queue link addition requests in link's callbacks.
	//! \param linkId Identifier of the off-mesh link. If linkId is equal to MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID, new id is generated and returned by the funcion
	//! \param agentTypeId Navigation agent type id for whom the link should be created.
	//! \param pLinkDataPtr _smart_ptr pointer to the instance of IOffMeshLink data.
	//! \param callbacks Structure of callbacks used for link creation and notifying about link removal.
	//! \return OffMeshLink id, that can be used for referencing OffMesh link
	virtual MNM::OffMeshLinkID      RequestLinkAddition(const MNM::OffMeshLinkID linkId, const NavigationAgentTypeID agentTypeId, _smart_ptr<MNM::IOffMeshLink> pLinkData, const MNM::SOffMeshLinkCallbacks& callbacks) = 0;
	
	//! Queue link removal request
	//! It is safe to queue link removal requests in link's callbacks.
	//! \param linkId Identifier of the off-mesh link to be removed.
	virtual void                    RequestLinkRemoval(const MNM::OffMeshLinkID linkId) = 0;

	//! Cancel link addition request
	//! It is safe to cancel addition requests in link's callbacks.
	//! \param linkId Identifier of the off-mesh link whose addition was requested.
	//! \return True if the link addition was previously requested and now canceled.
	virtual bool                    CancelLinkAddition(const MNM::OffMeshLinkID linkId) = 0;

	//! Queue link annotation change request
	//! Link's annotation is used in similar way as for NavMesh triangles for filtering during the path-finding and reachability queries.
	//! \param linkId Identifier of the off-mesh link for which the annotation is to be changed
	//! \param annotation Annotation to be applied (area type and flags)
	virtual void                    RequestLinkAnnotationChange(const MNM::OffMeshLinkID linkId, const MNM::AreaAnnotation annotation) = 0;

	//! Returns off-mesh link and it's annotation
	//! \param linkId Identifier of the off-mesh link to be returned.
	//! \param annotation Reference to annotation return parameter. It is set only when the link is found
	//! \return Pointer to off-mesh link if it was found, null otherwise
	virtual const MNM::IOffMeshLink* GetLinkAndAnnotation(const MNM::OffMeshLinkID linkId, MNM::AreaAnnotation& annotation) const = 0;

	//! Returns off-mesh link
	//! \param linkId Identifier of the off-mesh link to be returned.
	//! \return Pointer to off-mesh link if it was found, null otherwise
	virtual MNM::IOffMeshLink*       GetOffMeshLink(const MNM::OffMeshLinkID linkId) = 0;

	//! Returns off-mesh link (const)
	//! \param linkId Identifier of the off-mesh link to be returned.
	//! \return Pointer to off-mesh link if it was found, null otherwise
	virtual const MNM::IOffMeshLink* GetOffMeshLink(const MNM::OffMeshLinkID linkId) const = 0;

protected:
	void SetIdToOffMeshLink(MNM::IOffMeshLink& offmeshLink, const MNM::OffMeshLinkID linkId) const
	{
		offmeshLink.SetLinkId(linkId);
	}
};

