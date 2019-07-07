// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <UnitTest.h>

// This unit test is for the Resource Compiler only, which is only built on Windows
#if WIN32

#include "../../../Tools/RC/ResourceCompilerPC/AnimList.h"

TEST(AnimListRcMatchingTest, MatchSingleFile)
{
	CAnimList animList;
	animList.AddFile("objects/cat/meow.caf");

	EXPECT_TRUE(animList.MatchFile("objects/Cat/mEow.caf"));
	EXPECT_TRUE(animList.MatchFile("objects/cat/meOw.i_caf"));
	EXPECT_FALSE(animList.MatchFile("obJects/cat/woof.caf"));
	EXPECT_FALSE(animList.MatchFile("objects/foobar.caf"));
	EXPECT_FALSE(animList.MatchFile("Animals.json"));
}

TEST(AnimListRcMatchingTest, MatchSubdirectories)
{
	CAnimList animList;
	animList.AddWildcardFile("objects/dude/*/*.caf");
	animList.AddWildcardFile("objects/dude/animations/blendspaces/*/*.bspace");

	EXPECT_TRUE(animList.MatchFile("objects/dude/anim.caf"));
	EXPECT_TRUE(animList.MatchFile("objects/dude/anim.i_caf"));
	EXPECT_TRUE(animList.MatchFile("objects/dude/animations/death.caf"));
	EXPECT_FALSE(animList.MatchFile("objects/duDe/animations/animals.json"));
	EXPECT_TRUE(animList.MatchFile("objects/dude/aniMations/behaviours/scAred.caf"));
	EXPECT_TRUE(animList.MatchFile("objects/dude/ANIMATIONS/blendspaces/locoMotion.bspace"));
	EXPECT_FALSE(animList.MatchFile("Objects/duDe/animations/locomotion.bspace"));
}

TEST(AnimListRcMatchingTest, MatchPrefixes)
{
	CAnimList animList;
	animList.AddWildcardFile("*/dude_*.caf");
	animList.AddWildcardFile("objects/woman/*/woman_*.caf");

	EXPECT_TRUE(animList.MatchFile("objects/dude/dUde_anim.caf"));
	EXPECT_FALSE(animList.MatchFile("objects/dude/anim.caf"));
	EXPECT_TRUE(animList.MatchFile("objects/woman/woman_anim.caf"));
	EXPECT_FALSE(animList.MatchFile("objects/woman/woman.caf"));
}

TEST(AnimListRcMatchingTest, BrokenPatterns)
{
	{
		CAnimList animList;
		animList.AddWildcardFile("");

		EXPECT_FALSE(animList.MatchFile("anim.caf"));
		EXPECT_FALSE(animList.MatchFile(""));
	}

	{
		CAnimList animList;
		animList.AddWildcardFile("*");

		EXPECT_TRUE(animList.MatchFile("anim.caf"));
		EXPECT_FALSE(animList.MatchFile(""));
	}

	{
		CAnimList animList;
		animList.AddFile("/");
		animList.AddWildcardFile("/");

		EXPECT_FALSE(animList.MatchFile("anim.caf"));
		EXPECT_FALSE(animList.MatchFile(""));
	}
}

#endif