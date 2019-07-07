// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// Common projection operations like "project point onto plane"

#include "Cry_Math.h"
#include "Cry_Geo.h"

namespace Projection
{

	//////////////////////////////////////////////////////////////////////
	//
	// Declarations
	//
	//////////////////////////////////////////////////////////////////////

	//! Projects a point onto an arbitrary plane.
	//! The plane does *not* have to be at the origin.
	Vec3 PointOntoPlane(const Vec3& point, const Plane& plane);

	//! Projects a point onto an infinite line.
	Vec3 PointOntoLine(const Vec3& point, const Line& line);

	//////////////////////////////////////////////////////////////////////
	//
	// Inlined implementations
	//
	//////////////////////////////////////////////////////////////////////

	inline Vec3 PointOntoPlane(const Vec3& point, const Plane& plane)
	{
		return Vec3::CreateProjection(point, plane.n) - plane.n * plane.d;
	}

	inline Vec3 PointOntoLine(const Vec3& point, const Line& line)
	{
		const Vec3 v = point - line.pointonline;
		const float d = line.direction * v;
		return line.pointonline + line.direction * d;
	}

}
