// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

namespace VersionControlPathUtils
{

//! Corrects the casing for given list of files and remove ones that can't be matched.
EDITOR_COMMON_API void MatchCaseAndRemoveUnmatched(std::vector<string>& files);

//! Pushes a path with correct case only if correct case can be found.
EDITOR_COMMON_API void MatchCaseAndPushBack(std::vector<string>& outputFiles, const string& file);

};
