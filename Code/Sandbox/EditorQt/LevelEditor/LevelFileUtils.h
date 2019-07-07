// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

class QString;

// Used by the file dialogs
namespace LevelFileUtils
{

typedef QString AbsolutePath; // native filesystem global
typedef QString EnginePath;   // path relative to the current path
typedef QString AssetPath;    // path relative to the asset toot folder

// Path conversion between absolute and CryEngine local paths
AbsolutePath GetEngineBasePath();
EnginePath   ConvertAbsoluteToEnginePath(const AbsolutePath& path);
AbsolutePath ConvertEngineToAbsolutePath(const EnginePath& enginePath);

// Path conversion between absolute and asset local paths
AbsolutePath GetAssetBasePathAbsolute();
AssetPath    ConvertAbsoluteToAssetPath(const AbsolutePath& path);
AbsolutePath ConvertAssetToAbsolutePath(const AssetPath& assetPath);

// Returns true if a path contains a level
bool IsPathToLevel(const AssetPath& assetPath);

// Returns true if any parent folder contains a level
bool IsAnyParentPathLevel(const AssetPath& assetPath);

// Returns true if any subfolder contains a level
bool IsAnySubFolderLevel(const AssetPath& path);

} // namespace LevelFileUtils
