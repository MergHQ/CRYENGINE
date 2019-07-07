// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct INavigationUpdatesManager
{
	enum class EUpdateMode
	{
		Continuous,
		AfterStabilization,
		Disabled,
		DisabledDuringLoad,
	};

	virtual ~INavigationUpdatesManager() {}

	virtual void Update(const CTimeValue frameStartTime, const float frameDeltaTime) = 0;

	virtual void EntityChanged(int physicalEntityId, const AABB& aabb) = 0;
	virtual void WorldChanged(const AABB& aabb) = 0;

	virtual void RequestGlobalUpdate() = 0;
	virtual void RequestGlobalUpdateForAgentType(NavigationAgentTypeID agentTypeID) = 0;

	//! Returns true if there are any buffered regeneration requests
	virtual bool HasBufferedRegenerationRequests() const = 0;

	//! Moves buffered regeneration requests to the active requests queue
	virtual void ApplyBufferedRegenerationRequests(bool applyAll) = 0;

	//! Clear the buffered regeneration requests that were received when execution was disabled
	virtual void ClearBufferedRegenerationRequests() = 0;

	virtual bool WasRegenerationRequestedThisCycle() const = 0;
	virtual void ClearRegenerationRequestedThisCycleFlag() = 0;

	virtual void SetUpdateMode(const EUpdateMode updateMode) = 0;
	virtual EUpdateMode GetUpdateMode() const = 0;
};