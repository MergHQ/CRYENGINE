// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>

#include <CryMath/Cry_Math.h>
#include <Navigation/MNM/MNMUtils.h>

using namespace MNM;

TEST(CryMNMTest, IntersectSegmentSegment)
{
	const vector2_t A(real_t(0.0f), real_t(0.0f));
	const vector2_t B(real_t(0.0f), real_t(1.0f));
	const vector2_t C(real_t(1.0f), real_t(1.0f));
	const vector2_t D(real_t(1.0f), real_t(0.0f));
	const vector2_t E(real_t(0.5f), real_t(0.5f));

	real_t s, t;

	REQUIRE(Utils::DetailedIntersectSegmentSegment(A, B, A, B, s, t) == Utils::eIR_ParallelOrCollinearSegments);
	REQUIRE(Utils::DetailedIntersectSegmentSegment(A, B, C, D, s, t) == Utils::eIR_ParallelOrCollinearSegments);
	REQUIRE(Utils::DetailedIntersectSegmentSegment(A, B, E, D, s, t) == Utils::eIR_NoIntersection);
	REQUIRE(Utils::DetailedIntersectSegmentSegment(A, C, B, D, s, t) == Utils::eIR_Intersection);
	REQUIRE(Utils::DetailedIntersectSegmentSegment(A, B, A, C, s, t) == Utils::eIR_Intersection);
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
	REQUIRE(Utils::ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.1f), real_t(2.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.1f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point below the triangle
	REQUIRE(Utils::ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.1f), real_t(-1.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.1f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point above the triangle's edge
	REQUIRE(Utils::ProjectPointOnTriangleVertical(vector3_t(real_t(0.5f), real_t(0.0f), real_t(2.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.5f));
	REQUIRE(projected.y == real_t(0.0f));
	REQUIRE(projected.z == real_t(0.5f));

	// Point at vertex
	REQUIRE(Utils::ProjectPointOnTriangleVertical(vector3_t(real_t(0.0f), real_t(0.0f), real_t(0.0f)), A, B, C, projected));
	REQUIRE(projected.x == real_t(0.0f));
	REQUIRE(projected.y == real_t(0.0f));
	REQUIRE(projected.z == real_t(0.0f));

	// Degenerate triangle tests
	REQUIRE(!Utils::ProjectPointOnTriangleVertical(vector3_t(real_t(0.0f), real_t(0.5f), real_t(2.0f)), A, B, D, projected));
	REQUIRE(!Utils::ProjectPointOnTriangleVertical(A, A, A, C, projected));
	
	REQUIRE(Utils::ProjectPointOnTriangleVertical(A, A, A, A, projected)); // All points have same z value, z value for projected point can be set
}
