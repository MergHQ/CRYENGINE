// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavigationUpdatesManager.h>

#include "Navigation/MNM/BoundingVolume.h"

class NavigationSystem;

class CMNMUpdatesManager : public IPathGraphUpdatesManager
{
public:
	enum class EUpdateRequestStatus
	{
		RequestInQueue = 0,
		RequestDelayedAndBuffered,
		RequestInvalid,
		Count
	};

	struct TileUpdateRequest
	{
		TileUpdateRequest()
			: aborted(false) {}

		inline bool operator==(const TileUpdateRequest& other) const
		{
			return (meshID == other.meshID) && (x == other.y) && (y == other.y) && (z == other.z);
		}

		inline bool operator<(const TileUpdateRequest& other) const
		{
			if (meshID != other.meshID)
				return meshID < other.meshID;

			if (x != other.x)
				return x < other.x;

			if (y != other.y)
				return y < other.y;

			if (z != other.z)
				return z < other.z;

			return false;
		}

		NavigationMeshID meshID;
		uint16           x;
		uint16           y;
		uint16           z;
		bool             aborted;
	};

	typedef MNM::BoundingVolume NavigationBoundingVolume;

	CMNMUpdatesManager() = delete;

	CMNMUpdatesManager(NavigationSystem* pNavSystem);

	virtual ~CMNMUpdatesManager() override {}

	virtual void Update() override;

	virtual void EntityChanged(int id, const AABB& aabb) override;
	virtual void WorldChanged(const AABB& aabb) override;

	virtual void RequestGlobalUpdateForAgentType(NavigationAgentTypeID agentTypeID) override;

	virtual void EnableRegenerationRequestsExecution() override     { m_isMNMRegenerationRequestExecutionEnabled = true; }
	virtual void DisableRegenerationRequestsAndBuffer() override    { m_isMNMRegenerationRequestExecutionEnabled = false; }
	virtual bool AreRegenerationRequestsDisabled() const override   { return m_isMNMRegenerationRequestExecutionEnabled; }

	virtual bool WasRegenerationRequestedThisCycle() const override { return m_wasMNMRegenerationRequestedThisUpdateCycle; }
	virtual void ClearRegenerationRequestedThisCycleFlag() override { m_wasMNMRegenerationRequestedThisUpdateCycle = false; }

	virtual void EnableUpdatesAfterStabilization() override         { m_postponeUpdatesForStabilization = true; }
	virtual void DisableUpdatesAfterStabilization() override        { m_postponeUpdatesForStabilization = false; }

	virtual void ClearBufferedRegenerationRequests() override;

	//! Request MNM regeneration on a specific AABB for a specific meshID
	//! If MNM regeneration is disabled internally, requests will be stored in a buffer
	//! Return values
	//!   - RequestInQueue: request was successfully validated and is in execution queue
	//!   - RequestDelayedAndBuffered: MNM regeneration is turned off, so request is stored in buffer
	//!	  - RequestInvalid: there was something wrong with the request so it was ignored
	EUpdateRequestStatus     RequestQueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb);
	EUpdateRequestStatus     RequestQueueDifferenceUpdate(NavigationMeshID meshID, const NavigationBoundingVolume& oldVolume, const NavigationBoundingVolume& newVolume);

	size_t                   QueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb);
	void                     QueueDifferenceUpdate(NavigationMeshID meshID, const NavigationBoundingVolume& oldVolume, const NavigationBoundingVolume& newVolume);

	void                     Clear();
	void                     OnMeshDestroyed(NavigationMeshID meshID);

	size_t                   GetRequestQueueSize() const { return m_activeUpdateRequestsQueue.size(); }
	bool                     HasUpdateRequests() const   { return !m_activeUpdateRequestsQueue.empty(); }
	const TileUpdateRequest& GetFrontRequest() const     { return m_activeUpdateRequestsQueue.front(); }
	void                     PopFrontRequest()           { m_activeUpdateRequestsQueue.pop_front(); }

	void                     DebugDraw();

private:
	struct EntityUpdate
	{
		AABB aabbOld;
		AABB aabbNew;
	};

	typedef std::pair<NavigationMeshID, AABB>                                                          MNMRegenerationUpdateRequest;
	typedef std::pair<NavigationMeshID, std::pair<NavigationBoundingVolume, NavigationBoundingVolume>> MNMRegenerationDifferenceRequest;
	typedef std::deque<TileUpdateRequest>                                                              TileTaskQueue;
	typedef std::unordered_map<int, EntityUpdate>                                                      EntityUpdatesMap;

	EUpdateRequestStatus GetCurrentRequestsQueue(TileTaskQueue*& queue);
	void                 UpdateEntityChanges();

	NavigationSystem* m_pNavigationSystem;

	TileTaskQueue     m_activeUpdateRequestsQueue;

	// container that stores ignored tile updates requests
	// @note: todo - debug rendering that shows these 'not up to date' areas.
	TileTaskQueue    m_ignoredUpdateRequestsQueue;

	EntityUpdatesMap m_bufferedEntityUpdatesMap;

	bool             m_isMNMRegenerationRequestExecutionEnabled;
	bool             m_wasMNMRegenerationRequestedThisUpdateCycle;
	bool             m_postponeUpdatesForStabilization;

	CTimeValue       m_lastUpdateTime;
};
