// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include "AssetSystem/EditableAsset.h"
#include "EditorCommonAPI.h"

class CAsset;

namespace AssetLoader
{

struct SAssetMetadata;

//! \brief Returns the name of the asset with path \p szPath.
//! If \p szPath is a path to a file, the function returns the filename without extension.
//! If it is path to a directory instead, everything but the last directory is removed.
//! Examples:
//! GetAssetName("Assets/Models/Monster.cgf") = "Monster"
//! GetAssetName("Assets/Models/Monster.bak.cgf") = "Monster"
//! GetAssetName("Assets/Levels/Woods") = "Woods"
string GetAssetName(const char* szPath);

//! Returns true if the path points to an asset metadata file.
bool EDITOR_COMMON_API IsMetadataFile(const char* szPath);

class EDITOR_COMMON_API CAssetFactory
{
public:
	static CAsset* CreateFromMetadata(const char* szAssetPath, const SAssetMetadata& metadata);

	//! Read given metadata files from file system and update existing assets.
	static std::vector<CAsset*> LoadAssetsFromMetadataFiles(const std::vector<string>& metadataFiles);

	//! Reads xml with metadata from memory.
	//! \param szAssetPath Must be relative to the assets root. 
	static CAsset* LoadAssetFromXmlFile(const char* szAssetPath);

	//! Reads xml with metadata from memory.
	//! \param szAssetPath Must be relative to the assets root. 
	static CAsset* LoadAssetFromInMemoryXmlFile(const char* szAssetPath, uint64 timestamp, const char* pBuffer, size_t numberOfBytes);

	//! Loads assets from pak file.
	//! \param szArchivePath the path should be relative to the project root folder. The pak must be opened by one of the ICryPak::OpenPack(s) methods.
	//! \param Unary predicate which returns true for the required assets.
	static std::vector<CAsset*> LoadAssetsFromPakFile(const char* szArchivePath, std::function<bool(const char* szAssetRelativePath, uint64 timestamp)> predicate);
};

} // namespace AssetLoader
