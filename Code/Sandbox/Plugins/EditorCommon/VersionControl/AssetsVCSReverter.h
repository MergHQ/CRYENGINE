// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;

//! This class is responsible for reverting assets.
class EDITOR_COMMON_API CAssetsVCSReverter
{
public:
	//! Reverts assets.
	static void RevertAssets(const std::vector<CAsset*>& assets, std::vector<string> folders);
};
