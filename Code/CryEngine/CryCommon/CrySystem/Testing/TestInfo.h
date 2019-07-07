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
};

}
