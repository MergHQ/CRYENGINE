// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>
#include <functional>

class CAsset;
struct IFilesGroupProvider;

//! This class is responsible for synchronizing assets with the version on repository.
class EDITOR_COMMON_API CAssetsVCSSynchronizer
{
public:
	static void Sync(std::vector<CAsset*> assets, std::vector<string> folders , std::function<void()> callback = nullptr);
	static void Sync(std::shared_ptr<IFilesGroupProvider>, std::function<void()> callback = nullptr);

	//! Finds strings in vec1 that are absent in vec2
	static std::vector<string> FindMissingStrings(std::vector<string>& vec1, std::vector<string>& vec2);

	//! Compares current assets' file with given list and returns the difference list.
	static std::vector<string> GetMissingAssetsFiles(const std::vector<CAsset*>& assets, std::vector<string>& originalFiles);
};
