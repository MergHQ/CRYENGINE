// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include <UnitTest.h>
#include <CryString/CryPath.h>

template<typename TString>
void TestUnixPathA()
{
	REQUIRE(PathUtil::ToUnixPath(TString("some\\path\\etc")) == "some/path/etc");
	REQUIRE(PathUtil::ToUnixPath(TString("some/path\\etc")) == "some/path/etc");
	REQUIRE(PathUtil::ToUnixPath(TString("some\\path/etc")) == "some/path/etc");
	REQUIRE(PathUtil::ToUnixPath(TString("some\\path\\/etc")) == "some/path/etc");
	REQUIRE(PathUtil::ToUnixPath(TString("some\\path\\")) == "some/path/");
	REQUIRE(PathUtil::ToUnixPath(TString("somepath")) == "somepath");
	REQUIRE(PathUtil::ToUnixPath(TString("\\")) == "/");
	REQUIRE(PathUtil::ToUnixPath(TString("C:\\a.b\\")) == "C:/a.b/");
	REQUIRE(PathUtil::ToUnixPath(TString("")) == TString(""));
}

template<typename TString>
void TestUnixPathW()
{
	REQUIRE(PathUtil::ToUnixPath(TString(L"C:\\中文路径\\")) == TString(L"C:/中文路径/"));
}

TEST(CryPath, ToUnixPath)
{
	TestUnixPathA<string>();
	TestUnixPathA<CryStackStringT<char, 512>>();
	TestUnixPathA<CryFixedStringT<512>>();
	TestUnixPathW<wstring>();
	TestUnixPathW<CryStackStringT<wchar_t, 512>>();
	TestUnixPathW<CryFixedWStringT<512>>();
}