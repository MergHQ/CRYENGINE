// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetFilesProvider.h"

#include "AssetSystem/AssetFilesGroupProvider.h"
#include "AssetSystem/IFilesGroupProvider.h"
#include "FileUtils.h"
#include "LevelEditor/LayerFileGroupProvider.h"
#include "Objects/IObjectLayer.h"
#include "PathUtils.h"
#include "QtUtil.h"
#include "VersionControl.h"

#include <QFileInfo>

namespace Private_AssetFileProvider
{

using EInclude = CAssetFilesProvider::EInclude;

void ReplaceFoldersWithContent(std::vector<string>& files, std::vector<string>& outputFiles)
{
	for (int i = files.size() - 1; i >= 0; --i)
	{
		QString qPath = QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), files[i]));
		QFileInfo info(qPath);
		if (info.isDir())
		{
			files.erase(files.begin() + i);
			std::vector<string> dirsContent = FileUtils::GetDirectorysContent(qPath);
			outputFiles.reserve(outputFiles.size() + dirsContent.size());
			std::move(dirsContent.begin(), dirsContent.end(), std::back_inserter(outputFiles));
		}
	}
}

void InsertAssetFilePaths(const CAsset& asset, std::vector<string>& outputFiles, EInclude include, bool shouldReplaceFoldersWithContent)
{
	if (FileUtils::IsFileInPakOnly(asset.GetMetadataFile()))
	{
		return;
	}

	if (include == EInclude::OnlyMainFile)
	{
		outputFiles.push_back(PathUtil::MatchGamePathToCaseOnFileSystem(asset.GetMetadataFile()));
		return;
	}

	std::vector<string> files = asset.GetType()->GetAssetFiles(asset, false, false, false);

	if (include == EInclude::AllButMainFile)
	{
		files.erase(std::remove(files.begin(), files.end(), asset.GetMetadataFile()), files.end());
	}

	for (string& file : files)
	{
		file = PathUtil::MatchGamePathToCaseOnFileSystem(file);
	}

	if (shouldReplaceFoldersWithContent)
	{
		ReplaceFoldersWithContent(files, outputFiles);
	}

	std::move(files.begin(), files.end(), std::back_inserter(outputFiles));
}

void InsertLayerFilePaths(const IObjectLayer& layer, std::vector<string>& outputFiles, EInclude include)
{
	const string mainFile = layer.GetLayerFilepath();
	if (FileUtils::IsFileInPakOnly(mainFile))
	{
		return;
	}

	if (include == EInclude::OnlyMainFile || include == EInclude::AllFiles)
	{
		outputFiles.push_back(PathUtil::MatchGamePathToCaseOnFileSystem(PathUtil::MakeGamePath(mainFile)));
		if (include == EInclude::OnlyMainFile)
		{
			return;
		}
	}

	const string levelFolder = PathUtil::GetParentDirectory(PathUtil::GetDirectory(mainFile));
	std::vector<string> files = layer.GetFiles();
	for (string& file : files)
	{
		outputFiles.push_back(PathUtil::MatchGamePathToCaseOnFileSystem(PathUtil::MakeGamePath(PathUtil::Make(levelFolder, file))));
	}
}

void InsertFileGroupPaths(const IFilesGroupProvider& fileGroup, std::vector<string>& outputFiles, EInclude include)
{
	if (FileUtils::IsFileInPakOnly(fileGroup.GetMainFile()))
	{
		return;
	}

	if (include == EInclude::OnlyMainFile)
	{
		outputFiles.push_back(PathUtil::MatchGamePathToCaseOnFileSystem(fileGroup.GetMainFile()));
		return;
	}

	std::vector<string> files = fileGroup.GetFiles(false);

	if (include == EInclude::AllButMainFile)
	{
		files.erase(std::remove(files.begin(), files.end(), fileGroup.GetMainFile()), files.end());
	}

	for (string& file : files)
	{
		outputFiles.push_back(PathUtil::MatchGamePathToCaseOnFileSystem(file));
	}
}

}

std::vector<string> CAssetFilesProvider::GetForAssets(const std::vector<CAsset*>& assets, EInclude include, bool shouldReplaceFoldersWithContent)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	filePaths.reserve(assets.size() * 2 * 1.2);
	for (const CAsset* pAsset : assets)
	{
		InsertAssetFilePaths(*pAsset, filePaths, include, shouldReplaceFoldersWithContent);
	}
	return filePaths;
}

std::vector<string> CAssetFilesProvider::GetForLayers(const std::vector<IObjectLayer*>& layers, EInclude include /*= EInclude::AllFiles*/)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	filePaths.reserve(layers.size() * 1.2);
	for (const IObjectLayer* pLayer : layers)
	{
		InsertLayerFilePaths(*pLayer, filePaths, include);
	}
	return filePaths;
}

std::vector<string> CAssetFilesProvider::GetForFileGroups(const std::vector<std::shared_ptr<IFilesGroupProvider>>& fileGroups, EInclude include /*= EInclude::AllFiles*/)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	filePaths.reserve(fileGroups.size() * 1.2);
	for (const std::shared_ptr<IFilesGroupProvider>& pFileGroup : fileGroups)
	{
		InsertFileGroupPaths(*pFileGroup, filePaths, include);
	}
	return filePaths;
}

std::vector<string> CAssetFilesProvider::GetForFileGroup(const IFilesGroupProvider& fileGroup, EInclude include /*= EInclude::AllFiles*/)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	InsertFileGroupPaths(fileGroup, filePaths, include);
	return filePaths;
}

std::vector<std::shared_ptr<IFilesGroupProvider>> CAssetFilesProvider::ToFileGroups(const std::vector<IObjectLayer*>& layers)
{
	std::vector<std::shared_ptr<IFilesGroupProvider>> fileGroups;
	fileGroups.reserve(layers.size());
	std::transform(layers.cbegin(), layers.cend(), std::back_inserter(fileGroups), [](IObjectLayer* pLayer)
	{
		return std::make_shared<CLayerFileGroupProvider>(*pLayer);
	});
	return fileGroups;
}

std::vector<std::shared_ptr<IFilesGroupProvider>> CAssetFilesProvider::ToFileGroups(const std::vector<CAsset*>& assets)
{
	std::vector<std::shared_ptr<IFilesGroupProvider>> fileGroups;
	fileGroups.reserve(assets.size());
	std::transform(assets.cbegin(), assets.cend(), std::back_inserter(fileGroups), [](CAsset* pAsset)
	{
		return std::make_shared<CAssetFilesGroupProvider>(pAsset, false);
	});
	return fileGroups;
}
