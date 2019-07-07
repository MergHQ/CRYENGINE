// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "EditorCommonAPI.h"
#include <CryCore/StlUtils.h>
#include <set>

//! Class passed to resource gathering functions
class EDITOR_COMMON_API CUsedResources
{
public:
	typedef std::set<string, stl::less_stricmp<string>> TResourceFiles;

	CUsedResources();
	void Add(const char* pResourceFileName);
	//! validate gathered resources, reports warning if resource is not found
	void Validate();

	TResourceFiles files;
};
