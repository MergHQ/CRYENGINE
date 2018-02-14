// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>
#include <CryCore/Containers/CryArray.h>


TEST(CryDynArray, Construction)
{
	const int ais[] = { 11, 7, 5, 3, 2, 1, 0 };
	DynArray<int> ai;
	REQUIRE(ai.size() == 0);
	DynArray<int> ai1(4);
	REQUIRE(ai1.size() == 4);
	DynArray<int> ai3(6, 0);
	REQUIRE(ai3.size() == 6);
	DynArray<int> ai4(ai3);
	REQUIRE(ai4.size() == 6);
	DynArray<int> ai6(ais);
	REQUIRE(ai6.size() == 7);
	DynArray<int> ai7(ais + 1, ais + 5);
	REQUIRE(ai7.size() == 4);
}