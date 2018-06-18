// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>

#include <CryMath/Cry_Math.h>
#include <Navigation/MNM/MNM.h>

TEST(CryMNMTest, IntersectSegmentSegment)
{
	const MNM::vector2_t A(MNM::real_t(0.0f), MNM::real_t(0.0f));
	const MNM::vector2_t B(MNM::real_t(0.0f), MNM::real_t(1.0f));
	const MNM::vector2_t C(MNM::real_t(1.0f), MNM::real_t(1.0f));
	const MNM::vector2_t D(MNM::real_t(1.0f), MNM::real_t(0.0f));
	const MNM::vector2_t E(MNM::real_t(0.5f), MNM::real_t(0.5f));
	
	MNM::real_t s, t;

	REQUIRE(MNM::DetailedIntersectSegmentSegment(A, B, A, B, s, t) == MNM::eIR_ParallelOrCollinearSegments);
	REQUIRE(MNM::DetailedIntersectSegmentSegment(A, B, C, D, s, t) == MNM::eIR_ParallelOrCollinearSegments);
	REQUIRE(MNM::DetailedIntersectSegmentSegment(A, B, E, D, s, t) == MNM::eIR_NoIntersection);
	REQUIRE(MNM::DetailedIntersectSegmentSegment(A, C, B, D, s, t) == MNM::eIR_Intersection);
	REQUIRE(MNM::DetailedIntersectSegmentSegment(A, B, A, C, s, t) == MNM::eIR_Intersection);
	REQUIRE(s == 0); REQUIRE(t == 0);
}


