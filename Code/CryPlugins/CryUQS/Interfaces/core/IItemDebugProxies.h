// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{
		//
		// An IItemDebugProxy_* is a geometric representation of an item that was generated and evaluated throughout the query.
		// It's used to render a visual representation of all items after the query has finished.
		//

		//===================================================================================
		//
		// IItemDebugProxy_Sphere
		//
		//===================================================================================

		struct IItemDebugProxy_Sphere
		{
			virtual             ~IItemDebugProxy_Sphere() {}
			virtual void        SetPosAndRadius(const Vec3& pos, float radius) = 0;
		};

		//===================================================================================
		//
		// IItemDebugProxy_Path
		//
		//===================================================================================

		struct IItemDebugProxy_Path
		{
			virtual             ~IItemDebugProxy_Path() {}
			virtual void        AddPoint(const Vec3& point) = 0;
			virtual void        SetClosed(bool bClosed) = 0;
		};

		//===================================================================================
		//
		// IItemDebugProxy_AABB
		//
		//===================================================================================

		struct IItemDebugProxy_AABB
		{
			virtual             ~IItemDebugProxy_AABB() {}
			virtual void        SetAABB(const AABB& aabb) = 0;
		};

	}
}
