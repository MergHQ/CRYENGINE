// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef MovementSystem_h
	#define MovementSystem_h

	#include <CryAISystem/IMovementSystem.h>
	#include <CryAISystem/MovementRequest.h>
	#include "MovementActor.h"


class StandardMovementBlocksFactory : public IStandardMovementBlocksFactory
{
public:
	virtual Movement::BlockPtr CreateFollowPathBlock(Movement::IPlan& plan, NavigationAgentTypeID navigationAgentTypeID, const INavPath& path, const float endDistance, const MovementStyle& style, const bool endsInCover) const override;
	virtual Movement::BlockPtr CreateUseExactPositioningBlock(const INavPath& path, const MovementStyle& style) const override;
	virtual Movement::BlockPtr CreateUseSmartObjectBlock(const INavPath& path, const PathPointDescriptor::OffMeshLinkData& mnmData, const MovementStyle& style, IUseSmartObjectAdapter* & pOutUseSmartObjectAdapter) const override;
	virtual Movement::BlockPtr CreateHarshStopBlock() const override;
	virtual Movement::BlockPtr CreateTurnTowardsPosition(const Vec3& positionToTurnTowards) const override;
	virtual Movement::BlockPtr CreateCustomBlock(const INavPath& path, const PathPointDescriptor::OffMeshLinkData& mnmData, const MovementStyle& style) const override;
};

struct MovementUpdateContext;

class MovementSystem : public IMovementSystem
{
public:
	MovementSystem();

	// IMovementSystem
	virtual void              RegisterEntity(const EntityId entityId, MovementActorCallbacks callbacksConfiguration, IMovementActorAdapter& adapter) override;
	virtual void              UnregisterEntity(const EntityId entityId) override;
	virtual bool              IsEntityRegistered(const EntityId entityId) override;
	virtual MovementRequestID QueueRequest(const MovementRequest& request) override;
	virtual void              UnsuscribeFromRequestCallback(const MovementRequestID& id) override;
	virtual void              GetRequestStatus(const MovementRequestID& id, MovementRequestStatus& status) const override;
	virtual void              Update(const CTimeValue frameStartTime, const float frameDeltaTime) override;
	virtual void              Reset() override;
	virtual void              RegisterFunctionToConstructMovementBlockForCustomNavigationType(Movement::CustomNavigationBlockCreatorFunction blockFactoryFunction) override;
	virtual bool              AddActionAbilityCallbacks(const EntityId entityId, const SMovementActionAbilityCallbacks& ability) override;
	virtual bool              RemoveActionAbilityCallbacks(const EntityId entityId, const SMovementActionAbilityCallbacks& ability) override;
	virtual const IStandardMovementBlocksFactory& GetStandardMovementBlocksFactory() const override;
	virtual std::shared_ptr<Movement::IPlan> CreateAndReturnDefaultPlan() override;
	// ~IMovementSystem

	Movement::BlockPtr CreateCustomBlock(const CNavPath& path, const PathPointDescriptor::OffMeshLinkData& mnmData, const MovementStyle& style);

	typedef std::pair<MovementRequestID, MovementRequest> MovementRequestPair;

private:
	enum ActorUpdateStatus
	{
		KeepUpdatingActor,
		ActorCanBeRemoved
	};

	MovementRequestID GenerateUniqueMovementRequestID();
	void              FinishRequest(MovementActor& actor, const MovementRequestID requestId, MovementRequestResult::Result result,
	                                MovementRequestResult::FailureReason failureReason = MovementRequestResult::NoReason);
	void              CleanUpAfterFinishedRequest(MovementActor& actor, MovementRequestID activeRequestID);
	MovementRequest&  GetActiveRequest(MovementActor& actor, MovementRequestID* outRequestID = NULL);
	MovementActor&    GetExistingActorOrCreateNewOne(const EntityId entityId, IMovementActorAdapter& adapter, const MovementActorCallbacks& callbacks);
	MovementActor*    GetExistingActor(const EntityId entityId);
	bool              IsPlannerWorkingOnRequestID(const MovementActor& actor, const MovementRequestID& id) const;
	void              UpdateActors();
	ActorUpdateStatus UpdateActor(MovementActor& actor);
	void              UpdatePlannerAndDealWithResult(const MovementUpdateContext& context);
	void              StartWorkingOnNewRequestIfPossible(const MovementUpdateContext& context);

	#if defined(COMPILE_WITH_MOVEMENT_SYSTEM_DEBUG)
	void DrawDebugInformation() const;
	void DrawMovementRequestsForActor(const MovementActor& actor) const;
	#endif

	// Contains pairs of MovementRequestID and MovementRequest
	typedef std::vector<MovementRequestPair> Requests;
	Requests m_requests;

	// The actors are added and removed when they get new requests and
	// when they run out of them.
	typedef std::vector<MovementActor> Actors;
	Actors                                         m_actors;

	MovementRequestID                              m_nextUniqueRequestID;
	Movement::CustomNavigationBlockCreatorFunction m_createMovementBlockToHandleCustomNavigationType;

	StandardMovementBlocksFactory                  m_builtinMovementBlocksFactory;

	CTimeValue                                     m_frameStartTime;
	float                                          m_frameDeltaTime;
};

#endif // MovementSystem_h
