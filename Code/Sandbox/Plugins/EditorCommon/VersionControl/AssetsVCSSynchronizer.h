// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>
#include <functional>

class CAsset;
struct IFilesGroupController;
struct IObjectLayer;

//! This class is responsible for synchronizing assets with the version on repository.
class EDITOR_COMMON_API CAssetsVCSSynchronizer
{
public:
	//! Synchronizes given assets and folders with version in the repository.
	static void Sync(const std::vector<CAsset*>& assets, std::vector<string> folders , std::function<void()> callback = nullptr);

	//! Synchronizes files group with version in the repository.
	static void Sync(const std::shared_ptr<IFilesGroupController>&, std::function<void()> callback = nullptr);

	//! Synchronizes layers with version in the repository.
	static void Sync(const std::vector<IObjectLayer*>& layers, std::vector<string> folders, std::function<void()> callback = nullptr);

	//! Synchronizes files groups with version in the repository.
	static void Sync(std::vector<std::shared_ptr<IFilesGroupController>>, std::vector<string> folders, std::function<void()> callback = nullptr);
};
