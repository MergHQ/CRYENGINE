// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;
struct IObjectLayer;
struct IFilesGroupController;

//! This class is responsible for reverting assets.
class EDITOR_COMMON_API CAssetsVCSReverter
{
public:
	//! Reverts assets.
	static void RevertAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders);

	//! Reverts layers.
	static void RevertLayers(const std::vector<IObjectLayer*>& layers);

	//! Reverts layers.
	static void Revert(std::vector<std::shared_ptr<IFilesGroupController>> filesGroups);
};
