// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>

#include <CryCore/BaseTypes.h>
#include <CryMath/Cry_Math.h>
#include <CryMath/Cry_Math_SSE.h>
#include <CryMath/NumberVector.h>
#include <CryMath/Cry_Vector3.h>

#include <CryAISystem/NavigationSystem/MNMFixedAABB.h>
#include <CryAISystem/NavigationSystem/MNMTile.h>

using namespace MNM;

TEST(CryMNMTest, FixedAABBConstruction)
{
	// Test default ctor doesn't crash
	const aabb_t emptyStruct;
}

TEST(CryMNMTest, FixedAABBConstructionInitEmptyStruct)
{
	const aabb_t::init_empty emptyStruct;
	const aabb_t aabbDefault(emptyStruct);
	const aabb_t::value_type min(aabb_t::value_type::value_type::min());
	const aabb_t::value_type max(aabb_t::value_type::value_type::max());

	REQUIRE(aabbDefault.min == max);
	REQUIRE(aabbDefault.max == min);
}

TEST(CryMNMTest, FixedAABBConstructionParameterized)
{
	const vector3_t min(-1, -2, -3);
	const vector3_t max(1, 2, 3);
	const aabb_t aabb(min, max);
	REQUIRE(aabb.min == min);
	REQUIRE(aabb.max == max);
}

TEST(CryMNMTest, FixedAABBConstructionCopy)
{
	const aabb_t aabbOriginal(vector3_t(-1, -2, -3), vector3_t(1, 2, 3));
	const aabb_t aabbCopy(aabbOriginal);
	REQUIRE(aabbCopy.min == aabbOriginal.min);
	REQUIRE(aabbCopy.max == aabbOriginal.max);
}

TEST(CryMNMTest, FixedAABBOverlapsAABB)
{
	const aabb_t aabb(vector3_t(0, 0, 0), vector3_t(2, 3, 4));

	// AABBs overlap 
	const aabb_t aabbOverlapArea(vector3_t(1, 1, 1), vector3_t(3, 4, 5));
	REQUIRE(aabb.overlaps(aabbOverlapArea) == true);

	// AABBs only overlap an edge
	const aabb_t aabbOverlapEdge(vector3_t(0, 3, 0), vector3_t(2, 6, 4));
	REQUIRE(aabb.overlaps(aabbOverlapEdge) == true);

	// AABB fully contains the other one
	const aabb_t aabbContained(vector3_t(1, 1, 1), vector3_t(2, 2, 2));
	REQUIRE(aabb.overlaps(aabbContained) == true);

	// AABBs do not overlap
	const aabb_t aabbNoOverlap(vector3_t(3, 4, 5), vector3_t(4, 5, 6));
	REQUIRE(aabb.overlaps(aabbNoOverlap) == false);
}

TEST(CryMNMTest, FixedAABBOverlapsTriangle)
{
	const aabb_t aabb(vector3_t(0, 0, 0), vector3_t(2, 3, 4));

	// Triangle with vertex on edge and intersecting aabb
	const vector3_t triangleV0EdgeIntersecting(0, 0, 0);
	const vector3_t triangleV1EdgeIntersecting(-1, 2, 2);
	const vector3_t triangleV2EdgeIntersecting(5, 5, 5);
	REQUIRE(aabb.overlaps(triangleV0EdgeIntersecting, triangleV1EdgeIntersecting, triangleV2EdgeIntersecting) == true);

	// Triangle with vertex on edge and not intersecting aabb
	const vector3_t triangleV0EdgeNotIntersecting(2, 0, 0);
	const vector3_t triangleV1EdgeNotIntersecting(3, 1, 0);
	const vector3_t triangleV2EdgeNotIntersecting(3, 3, 2);
	REQUIRE(aabb.overlaps(triangleV0EdgeNotIntersecting, triangleV1EdgeNotIntersecting, triangleV2EdgeNotIntersecting) == true);

	// Triangle fully contained inside aabb
	const vector3_t triangleV0FullyContained(1, 1, 1);
	const vector3_t triangleV1FullyContained(1, 1, 3);
	const vector3_t triangleV2FullyContained(1, 2, 2);
	REQUIRE(aabb.overlaps(triangleV0FullyContained, triangleV1FullyContained, triangleV2FullyContained) == true);

	// Triangle with only one vertex contained inside aabb
	const vector3_t triangleV0PartiallyContained(1, 1, 1);
	const vector3_t triangleV1PartiallyContained(3, 4, 4);
	const vector3_t triangleV2PartiallyContained(5, 4, 6);
	REQUIRE(aabb.overlaps(triangleV0PartiallyContained, triangleV1PartiallyContained, triangleV2PartiallyContained) == true);

	// Triangle with all vertices outside and intersecting aabb
	const vector3_t triangleV0Intersecting(-1, 0, 0);
	const vector3_t triangleV1Intersecting(-1, 2, 2);
	const vector3_t triangleV2Intersecting(5, 5, 5);
	REQUIRE(aabb.overlaps(triangleV0Intersecting, triangleV1Intersecting, triangleV2Intersecting) == true);

	// Triangle not intersecting aabb
	const vector3_t triangleV0NoOverlap(-1, 0, 0);
	const vector3_t triangleV1NoOverlap(1, 1, 5);
	const vector3_t triangleV2NoOverlap(3, 1, 1);
	REQUIRE(aabb.overlaps(triangleV0NoOverlap, triangleV1NoOverlap, triangleV2NoOverlap) == true);
}

TEST(CryMNMTest, FixedAABBContainsAABB)
{
	const aabb_t aabb(vector3_t(0, 0, 0), vector3_t(2, 3, 4));

	// AABB fully contains the other
	const aabb_t aabbContained(vector3_t(1, 1, 1), vector3_t(2, 2, 2));
	REQUIRE(aabb.contains(aabbContained) == true);

	// AABBs only overlap an edge
	const aabb_t aabbContainedEdge(vector3_t(1, 1, 1), vector3_t(2, 3, 4));
	REQUIRE(aabb.contains(aabbContainedEdge) == true);

	// AABBs partially overlap
	const aabb_t aabbOverlapArea(vector3_t(1, 1, 1), vector3_t(3, 4, 5));
	REQUIRE(aabb.contains(aabbOverlapArea) == false);

	// AABBs do not overlap at all
	const aabb_t aabbNoOverlap(vector3_t(3, 4, 5), vector3_t(4, 5, 6));
	REQUIRE(aabb.contains(aabbNoOverlap) == false);
}

TEST(CryMNMTest, FixedAABBContainsPoint)
{
	const aabb_t aabb(vector3_t(0, 0, 0), vector3_t(2, 3, 4));

	// Point is inside AABB
	const vector3_t pointInside(1, 1, 1);
	REQUIRE(aabb.contains(pointInside) == true);

	// Point lies on the edge of AABB
	const vector3_t pointEdge(0, 0, 0);
	REQUIRE(aabb.contains(pointEdge) == true);

	// Point is not contained inside AABB
	const vector3_t pointOutside(5, 2, 5);
	REQUIRE(aabb.contains(pointOutside) == false);
}