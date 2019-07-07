// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>
#include <CryExtension/CryGUID.h>
#include <CryString/StringUtils.h>

// Meta test - constexpr behaviors
constexpr CryGUID gConstGuid = "{4DFD2FF0-6403-4B0C-87C3-840148B7598B}"_cry_guid;
static_assert(!gConstGuid.IsNull(), "constexpr IsNull test");
static_assert(gConstGuid == "4DFD2FF0-6403-4B0C-87C3-840148B7598B"_cry_guid, "constexpr equality test");
static_assert(gConstGuid != "296708CE-F570-4263-B067-C6D8B15990BD"_cry_guid, "constexpr inequality test");

TEST(CryGUIDTest, All)
{
	CryGUID guid;

	// Test that CryGUID constructor initialize it to null
	REQUIRE(guid.IsNull());

	guid = "296708CE-F570-4263-B067-C6D8B15990BD"_cry_guid;

	// Verify that all string-based ways to create CryGUID return the same result
	REQUIRE(guid == CryGUID::FromString("296708CE-F570-4263-B067-C6D8B15990BD"));

	// Test that GUID specified in string with or without brackets work reliably
	REQUIRE(guid == "{296708CE-F570-4263-B067-C6D8B15990BD}"_cry_guid);
	REQUIRE(guid == CryGUID::FromString("{296708CE-F570-4263-B067-C6D8B15990BD}"));

	// Verify that CryGUID is case insensitive
	REQUIRE(guid == "296708ce-f570-4263-b067-c6d8b15990bd"_cry_guid);
	REQUIRE(guid == "{296708ce-f570-4263-b067-c6d8b15990bd}"_cry_guid);

	// Test back conversion from GUID to string
	char str[64];
	guid.ToString(str);
	REQUIRE(CryStringUtils::toUpper(str) == "296708CE-F570-4263-B067-C6D8B15990BD");
	REQUIRE(CryStringUtils::toUpper(guid.ToString()) == "296708CE-F570-4263-B067-C6D8B15990BD");
}

