// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryString/CryString.h>
#include <CrySerialization/IArchive.h>

namespace CryTest
{

struct STestInfo
{
	string suite;
	string name;
	string fileName;
	int    lineNumber = 0;
	string module;    //dll name

	void   Serialize(Serialization::IArchive& ar)
	{
		ar(suite, "suite");
		ar(name, "name");
		ar(fileName, "fileName");
		ar(lineNumber, "lineNumber");
		ar(module, "module");
	}

	string GetQualifiedName() const
	{
		if (suite.empty())
			return string().Format("[%s] %s", module.c_str(), name.c_str());
		else
			return string().Format("[%s] %s:%s", module.c_str(), suite.c_str(), name.c_str());
	}
};

}
