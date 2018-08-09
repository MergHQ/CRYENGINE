// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;

//! This is a convenient class that provides lists of files for specific asset(s).
class EDITOR_COMMON_API CAssetFilesProvider
{
public:
	static std::vector<string> GetForAsset(const CAsset& asset);

	static std::vector<string> GetForAssets(const std::vector<CAsset*>& assets);

	static std::vector<string> GetForAssetWithoutMetafile(const CAsset& asset);

};
