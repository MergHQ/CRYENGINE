// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//! \cond INTERNAL

#pragma once

struct IMovementActor;
struct IMovementSystem;
struct IPathFollower;
struct SOBJECTSTATE;
struct MovementRequest;

class CPipeUser;

namespace Movement
{
struct IPlanner;
}

//! Contains information needed while updating the movement system.
//! Constructed every frame in the movement system update.
struct MovementUpdateContext
{
	MovementUpdateContext(
	  IMovementActor& _actor,
	  IMovementSystem& _movementSystem,
	  IPathFollower& _pathFollower,
	  Movement::IPlanner& _planner,
	  const float _updateTime,
	  const CTimeValue _frameStartTime
	  )
		: actor(_actor)
		, movementSystem(_movementSystem)
		, pathFollower(_pathFollower)
		, planner(_planner)
		, updateTime(_updateTime)
		, frameStartTime(_frameStartTime)
	{
	}

	IMovementActor&     actor;
	IMovementSystem&    movementSystem;
	IPathFollower&      pathFollower;
	Movement::IPlanner& planner;
	const float         updateTime;
	const CTimeValue    frameStartTime;
};

//! \endcond