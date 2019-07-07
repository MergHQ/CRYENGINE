// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <vector>

class CAsset;
struct IObjectLayer;
struct IFilesGroupController;

//! This class is responsible for checking out assets.
class EDITOR_COMMON_API CAssetsVCSCheckerOut
{
public:
	//! Checks out assets.
	static void CheckOut(const std::vector<CAsset*>& assets);

	//! Checks out layers
	static void CheckOut(const std::vector<IObjectLayer*>& layers);

	//! Check out file groups
	static void CheckOut(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups);
};
