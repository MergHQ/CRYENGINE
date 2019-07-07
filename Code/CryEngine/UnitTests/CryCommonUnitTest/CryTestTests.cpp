// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>
#include <CrySystem/Testing/CryTest.h>
#include "Utils.h"

static int s_updateCounter;

class CTestCommand
{
public:
	bool Update()
	{
		s_updateCounter++;
		return true;
	}

	CCopyMoveInspector inspector;
};

TEST(CryTestCommandTest, ConstructionRValue)
{
	CCopyMoveInspector::Reset();
	CryTest::CCommand cmd = CTestCommand{};
	REQUIRE(CCopyMoveInspector::copyCount == 0);
}

TEST(CryTestCommandTest, ConstructionLValue)
{
	CCopyMoveInspector::Reset();
	CTestCommand cmdRaw;
	CryTest::CCommand cmd = cmdRaw;
	REQUIRE(CCopyMoveInspector::copyCount == 1);
}

TEST(CryTestCommandTest, Copy)
{
	CryTest::CCommand cmd = CTestCommand{};
	//Copying must be deep, because commands may have internal states and every repeated command has a different state
	CCopyMoveInspector::Reset();
	CryTest::CCommand cmdCopy = cmd;
	REQUIRE(CCopyMoveInspector::copyCount == 1);
	REQUIRE(CCopyMoveInspector::moveCount == 0);
}

TEST(CryTestCommandTest, Update)
{
	s_updateCounter = 0;
	CryTest::CCommand cmd = CTestCommand{};
	cmd.Update();
	REQUIRE(s_updateCounter == 1);
}
