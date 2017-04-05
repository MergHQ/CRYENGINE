// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Metadata.h"

class IConfig;
struct IResourceCompiler;

namespace AssetManager
{

const char* GetAssetExtension();

void InitMetadata(AssetManager::SAssetMetadata& metadata, const IConfig* pConfig, const string& sourceFilepath, const std::vector<string>& assetFiles);

bool SaveAsset(IResourceCompiler* pRc, const IConfig* pConfig, const string& sourceFilepath, const std::vector<string>& assetFilepaths);

std::vector<std::pair<string, string>> CollectMetadataDetails(const char* szFilename);

}
