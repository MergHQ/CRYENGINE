// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include <UnitTest.h>

#include <CryMemory/CrySizer.h>
#include <XML/xml.h>

TEST(CryCXmlNode, Construction)
{
	CXmlNode n;
	REQUIRE(n.GetRefCount() == 0);
}

TEST(CryCXmlNode, Construction2)
{
	CXmlNode n{"???", true};
	REQUIRE(n.GetRefCount() == 0);
}

