// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "EditorCommonAPI.h"
#include <CryMath/Cry_Math.h>
#include <vector>

//! Generates 2D convex hull from ptsIn using Graham's scan.
void                   ConvexHull2DGraham(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn);
//! Generates 2D convex hull from ptsIn using Andrew's algorithm.
EDITOR_COMMON_API void ConvexHull2DAndrew(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn);

//! Generates 2D convex hull from ptsIn
inline void ConvexHull2D(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn)
{
	// [Mikko] Note: The convex hull calculation is bound by the sorting.
	// The sort in Andrew's seems to be about 3-4x faster than Graham's--using Andrew's for now.
	ConvexHull2DAndrew(ptsOut, ptsIn);
}
