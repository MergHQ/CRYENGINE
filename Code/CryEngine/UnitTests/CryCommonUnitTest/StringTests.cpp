// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>
#include <CryString/CryString.h>
#include <string>

TEST(CryString, Construction)
{
	const char* c_str = nullptr;
	EXPECT_NO_FATAL_FAILURE({ string str = c_str; });
}

TEST(CryString, FindLastOf)
{
	string strTestFindLastOfOverload1("abcd-1234-abcd-1234");
	size_t nPosition(string::npos);
	nPosition = strTestFindLastOfOverload1.find_last_of('d', 14);
	REQUIRE(nPosition == 13);
}