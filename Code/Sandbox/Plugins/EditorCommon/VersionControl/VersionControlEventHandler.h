// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommon.h"

class CAsset;
struct IObjectLayer;

namespace VersionControlEventHandler
{

void EDITOR_COMMON_API HandleOnAssetBrowser(const QString& event, std::vector<CAsset*> assets, std::vector<string> folders);

void EDITOR_COMMON_API HandleOnLevelExplorer(const QString& event, std::vector<IObjectLayer*> layers, std::vector<IObjectLayer*> layerFolders);

}
