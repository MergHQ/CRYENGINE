// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "AssetSystem/Asset.h"
#include "VersionControlFileStatus.h"
#include <vector>

class CAsset;
struct IFilesGroupController;
struct IObjectLayer;

//! This class is responsible for providing current VCS status of assets as well as refreshing it.
class EDITOR_COMMON_API CAssetsVCSStatusProvider
{
public:
	//! Returns the status of the given file.
	static int  GetStatus(const string& filePath);
	//! Returns the status of the given asset.
	static int  GetStatus(const CAsset&);
	//! Returns the status of the given file group.
	static int  GetStatus(const IFilesGroupController&);
	//! Returns the status of the given layer.
	static int  GetStatus(const IObjectLayer&);

	//! Specifies if the given file has a specific status.
	static bool HasStatus(const string& filePath, int status);
	//! Specifies if the given asset has a specific status.
	static bool HasStatus(const CAsset&, int status);
	//! Specifies if the given file group has a specific status.
	static bool HasStatus(const IFilesGroupController&, int status);
	//! Specifies if the given layer has a specific status.
	static bool HasStatus(const IObjectLayer&, int status);

	//! Updates the status of given assets or folders. This call is asynchronous.
	//! \param callback Callback to be called once the status is updated.
	static void UpdateStatus(const std::vector<CAsset*>&, const std::vector<string>& folders = {}, std::function<void()> callback = nullptr);
	//! Updates the status of given layers. This call is asynchronous.
	//! \param callback Callback to be called once the status is updated.
	static void UpdateStatus(const std::vector<IObjectLayer*>&, std::function<void()> callback = nullptr);
	//! Updates the status of given file group. This call is asynchronous.
	//! \param callback Callback to be called once the status is updated.
	static void UpdateStatus(const IFilesGroupController&, std::function<void()> callback = nullptr);
	//! Updates the status of given list of file groups. This call is asynchronous.
	//! \param callback Callback to be called once the status is updated.
	static void UpdateStatus(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups, std::vector<string> folder = {}, std::function<void()> callback = nullptr);
};
