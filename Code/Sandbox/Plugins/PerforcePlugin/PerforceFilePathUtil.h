// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "CryString/CryString.h"
#include <vector>

namespace PerforceFilePathUtil
{

//! Adjusts given folder paths to ones suitable to use by perforce.
std::vector<string> AdjustFolders(const std::vector<string>& folders);

//! Adjusts given paths to ones suitable to use by perforce.
std::vector<string> AdjustPaths(const std::vector<string>& paths);

//! Splits given vector of paths into 2 vectors: files and folders.
void SeparateFolders(const std::vector<string>& paths, std::vector<string>& outputFiles, std::vector<string>& outputFolders);

}
