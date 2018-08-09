// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceFilePathUtil.h"
#include "FilePathUtil.h"

namespace Private_PerforceFilePathUtil
{

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

void SeparateFolders(const std::vector<string>& paths, std::vector<string>& outputFiles, std::vector<string>& outputFolders)
{
	using namespace Private_PerforceFilePathUtil;
	for (const string& path : paths)
	{
		auto absolutePath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), path);
		if (PathUtil::FileExists(absolutePath))
		{
			outputFiles.push_back(path);
		}
		else if (PathUtil::FolderExists(absolutePath))
		{
			outputFolders.push_back(DoesFolderNeedAdjusting(path) ? GetAdjustedFolderPath(path) : path);
		}
	}
}

}
