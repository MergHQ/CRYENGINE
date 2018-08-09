// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <vector>

class CAsset;

//! This class is responsible for checking out assets.
class EDITOR_COMMON_API CAssetsVCSCheckerOut
{
public:
	//! Checks out assets.
	static void CheckOutAssets(std::vector<CAsset*> assets);
};
