// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "CryString/CryString.h"

enum class EConflictResolution
{
    None,
    Their,
    Ours
};

using SVersionControlFileConflictStatus = std::pair<string, EConflictResolution>;
