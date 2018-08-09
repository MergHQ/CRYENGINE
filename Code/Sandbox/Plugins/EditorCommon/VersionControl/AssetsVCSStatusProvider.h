// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "AssetSystem/Asset.h"
#include "VersionControlFileStatus.h"
#include <vector>

class CAsset;
struct IFilesGroupProvider;

//! This class is responsible for providing current VCS status of assets as well as refreshing it.
class EDITOR_COMMON_API CAssetsVCSStatusProvider
{
public:
	static int  GetStatus(const CAsset&);
	static int  GetStatus(const IFilesGroupProvider&);

	static bool HasStatus(const CAsset&, int status);
	static bool HasStatus(const IFilesGroupProvider&, int status);

	static void UpdateStatus(const std::vector<CAsset*>&, const std::vector<string>& folder = {}, std::function<void()> callback = nullptr);
	static void UpdateStatus(const IFilesGroupProvider&, std::function<void()> callback = nullptr);
};
