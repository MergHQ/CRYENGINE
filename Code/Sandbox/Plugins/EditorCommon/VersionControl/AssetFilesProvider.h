// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;
struct IFilesGroupController;
struct IObjectLayer;

//! This is a convenient class that provides lists of files for specific asset(s) or layer(s).
class EDITOR_COMMON_API CAssetFilesProvider
{
public:
	// Filter for file provider that allows to specify which file of an item (asset or layer) should be provided.
	enum class EInclude
	{
		OnlyMainFile,
		AllButMainFile,
		AllFiles
	};

	//! Returns the list of files that comprise the given assets including metadata file.
	//! \param include Specifies which files need to be included.
	//! \param shouldReplaceFolderWithContent Specifies if instead of path to folder (if any) there will be
	//! listed it's content (recursively).
	static std::vector<string> GetForAssets(const std::vector<CAsset*>& assets, EInclude include = EInclude::AllFiles, bool shouldReplaceFoldersWithContent = false);

	//! Returns the list of files that comprise the given layers including .lyr file.
	//! \param include Specifies which files need to be included.
	static std::vector<string> GetForLayers(const std::vector<IObjectLayer*>& layers, EInclude include = EInclude::AllFiles);

	//! Returns the list of files that belong to given file groups.
	//! \param include Specifies which files need to be included.
	static std::vector<string> GetForFileGroups(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups, EInclude include = EInclude::AllFiles);

	//! Returns the list of files that belong to given file group.
	//! \param include Specifies which files need to be included.
	static std::vector<string> GetForFileGroup(const IFilesGroupController& fileGroup, EInclude include = EInclude::AllFiles);

	//! Converts given list of layers into a list of file groups.
	static std::vector<std::shared_ptr<IFilesGroupController>> ToFileGroups(const std::vector<IObjectLayer*>& layers);

	//! Converts given list of assets into a list of file groups.
	static std::vector<std::shared_ptr<IFilesGroupController>> ToFileGroups(const std::vector<CAsset*>& assets);
};
