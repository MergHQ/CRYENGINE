// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceFilePathUtil.h"

#include "FileUtils.h"
#include "PathUtils.h"

namespace Private_PerforceFilePathUtil
{

static bool DoesLookLikeFolder(const string& path)
{
	return !path.empty() && path[path.size() - 1] == '/';
}

static string GetAdjustedFolderPath(const string& folderPath)
{
	return folderPath.empty() || folderPath[folderPath.size() - 1] == '/' ? folderPath + "..." : folderPath + "/...";
}

static bool DoesFolderNeedAdjusting(const string& folderPath)
{
	return folderPath != "..." && folderPath.find("/...") != folderPath.size() - 4;
}

}

namespace PerforceFilePathUtil
{

std::vector<string> AdjustFolders(const std::vector<string>& folders)
{
	using namespace Private_PerforceFilePathUtil;
	std::vector<string> adjustedFolders;
	adjustedFolders.reserve(folders.size());
	for (const string& folder : folders)
	{
		adjustedFolders.push_back(DoesFolderNeedAdjusting(folder) ? GetAdjustedFolderPath(folder) : folder);
	}
	return adjustedFolders;
}

std::vector<string> AdjustPaths(const std::vector<string>& paths)
{
	std::vector<string> files;
	std::vector<string> folders;
	SeparateFolders(paths, files, folders);
	if (!folders.empty())
	{
		folders = AdjustFolders(folders);
		std::move(folders.begin(), folders.end(), std::back_inserter(files));
	}

	return files;
}

void SeparateFolders(const std::vector<string>& paths, std::vector<string>& outputFiles, std::vector<string>& outputFolders)
{
	using namespace Private_PerforceFilePathUtil;
	for (const string& path : paths)
	{
		if (DoesLookLikeFolder(path) || FileUtils::FolderExists(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), path)))
		{
			outputFolders.push_back(path);
		}
		else
		{
			outputFiles.push_back(path);
		}
	}
}

}
