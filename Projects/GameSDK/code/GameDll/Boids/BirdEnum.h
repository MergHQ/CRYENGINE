// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Bird
{
	typedef enum
	{
		FLYING,
		TAKEOFF,
		LANDING,
		ON_GROUND,
		DEAD
	} EStatus;

	// sub-stati of EStatus::ON_GROUND
	typedef enum
	{
		OGS_WALKING,
		OGS_SLOWINGDOWN,	// transitioning from walking to idle
		OGS_IDLE
	} EOnGroundStatus;

	enum
	{
		ANIM_FLY,
		ANIM_TAKEOFF,
		ANIM_WALK,
		ANIM_IDLE,
		ANIM_LANDING_DECELERATING,	// we've been in the landing state for a while and are now decelerating to land softly on the ground
	};

} // namespace
