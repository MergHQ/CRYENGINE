// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>

#include <CryMath/Cry_Math.h>
#include <Navigation/MNM/MNM.h>

using namespace MNM;

TEST(CryMNMTest, IntersectSegmentSegment)
{
	const vector2_t A(real_t(0.0f), real_t(0.0f));
	const vector2_t B(real_t(0.0f), real_t(1.0f));
	const vector2_t C(real_t(1.0f), real_t(1.0f));
	const vector2_t D(real_t(1.0f), real_t(0.0f));
	const vector2_t E(real_t(0.5f), real_t(0.5f));

	real_t s, t;

	REQUIRE(DetailedIntersectSegmentSegment(A, B, A, B, s, t) == eIR_ParallelOrCollinearSegments);
	REQUIRE(DetailedIntersectSegmentSegment(A, B, C, D, s, t) == eIR_ParallelOrCollinearSegments);
	REQUIRE(DetailedIntersectSegmentSegment(A, B, E, D, s, t) == eIR_NoIntersection);
	REQUIRE(DetailedIntersectSegmentSegment(A, C, B, D, s, t) == eIR_Intersection);
	REQUIRE(DetailedIntersectSegmentSegment(A, B, A, C, s, t) == eIR_Intersection);
	REQUIRE(s == 0); REQUIRE(t == 0);
}

TEST(CryMNMTest, ProjectPointOnTriangleVertical)
{
	const vector3_t A(real_t(0.0f), real_t(0.0f), real_t(0.0f));
	const vector3_t B(real_t(0.0f), real_t(1.0f), real_t(0.0f));
	const vector3_t C(real_t(1.0f), real_t(0.0f), real_t(1.0f));
	const vector3_t D(real_t(0.0f), real_t(0.0f), real_t(1.0f));
	vector3_t projected;

	// Point above the triangle
	REQUIRE(ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.1f), real_t(2.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.1f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point below the triangle
	REQUIRE(ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.1f), real_t(-1.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.1f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point above the triangle's edge
	REQUIRE(ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.0f), real_t(2.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.0f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point at vertex
	REQUIRE(ProjectPointOnTriangleVertical(vector3_t(real_t(0.0f), real_t(0.0f), real_t(0.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.0f));
	REQUIRE(projected.y == real_t(0.0f));
	REQUIRE(projected.z == real_t(0.0f));

	// Degenerate triangle tests
	REQUIRE(!ProjectPointOnTriangleVertical(vector3_t(real_t(0.0f), real_t(0.5f), real_t(2.0f)), A, B, D, projected));
	REQUIRE(!ProjectPointOnTriangleVertical(A, A, A, A, projected));

	// Point outside the triangle
	REQUIRE(!ProjectPointOnTriangleVertical(vector3_t(real_t(-1.0f), real_t(0.5f), real_t(2.0f)), A, B, D, projected));
}
