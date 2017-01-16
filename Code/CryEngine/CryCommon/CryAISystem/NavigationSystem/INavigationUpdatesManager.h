// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IPathGraphUpdatesManager
{
	virtual ~IPathGraphUpdatesManager() {}

	virtual void Update() = 0;

	virtual void EntityChanged(int id, const AABB& aabb) = 0;
	virtual void WorldChanged(const AABB& aabb) = 0;

	virtual void RequestGlobalUpdateForAgentType(NavigationAgentTypeID agentTypeID) = 0;

	//! Allow regeneration requests to be executed. 
	//! note: at this point any previously buffered requests that were not executed when disabled are discarded
	virtual void EnableRegenerationRequestsExecution() = 0;

	//! Block regeneration requests from being executed.
	//! note: they will be buffered, but not implicitly executed when they are allowed again
	virtual void DisableRegenerationRequestsAndBuffer() = 0;
	virtual bool AreRegenerationRequestsDisabled() const = 0;

	//! Clear the buffered regeneration requests that were received when execution was disabled
	virtual void ClearBufferedRegenerationRequests() = 0;

	virtual bool WasRegenerationRequestedThisCycle() const = 0;
	virtual void ClearRegenerationRequestedThisCycleFlag() = 0;

	virtual void EnableUpdatesAfterStabilization() = 0;
	virtual void DisableUpdatesAfterStabilization() = 0;
};