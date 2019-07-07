// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlPathUtils.h"
#include "VersionControl.h"
#include "PathUtils.h"
#include <IEditor.h>

namespace VersionControlPathUtils
{

void MatchCaseAndRemoveUnmatched(std::vector<string>& files)
{
	auto numFiles = files.size();
	for (int i = numFiles - 1; i >= 0; --i)
	{
		const string& file = files[i];
		string correctFilePath = PathUtil::MatchGamePathToCaseOnFileSystem(file);
		if (correctFilePath.empty())
		{
			const auto fs = CVersionControl::GetInstance().GetFileStatus(file);
			if (fs)
			{
				files[i] = fs->GetFileName();
			}
			else
			{
				using namespace std;
				swap(files[i], files[--numFiles]);
			}
		}
		else
		{
			files[i] = correctFilePath;
		}
	}
	files.erase(files.begin() + numFiles, files.end());
}

void MatchCaseAndPushBack(std::vector<string>& outputFiles, const string& file)
{
	string correctPath = PathUtil::MatchGamePathToCaseOnFileSystem(file);
	if (!correctPath.empty())
	{
		outputFiles.push_back(correctPath);
	}
	else
	{
		const auto fs = CVersionControl::GetInstance().GetFileStatus(file);
		if (fs)
		{
			outputFiles.push_back(fs->GetFileName());
		}
	}
}

}