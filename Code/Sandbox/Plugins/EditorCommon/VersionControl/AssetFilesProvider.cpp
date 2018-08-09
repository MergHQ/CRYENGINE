// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetFilesProvider.h"
#include "FilePathUtil.h"
#include "QtUtil.h"
#include <QFileInfo.h>

namespace Private_AssetFileProvider
{

void AddDirectorysContent(const QString& dirName, std::vector<string>& outputFiles)
{
	QDir dir(dirName);
	QFileInfoList infoList = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
	for (const QFileInfo& fileInfo : infoList)
	{
		const QString absolutePath = fileInfo.absoluteFilePath();
		if (fileInfo.isDir())
		{
			AddDirectorysContent(absolutePath, outputFiles);
		}
		else
		{
			outputFiles.push_back(PathUtil::ToGamePath(QtUtil::ToString(absolutePath)));
		}
	}
}

void ReplaceFoldersWithContent(std::vector<string>& files, std::vector<string>& outputFiles)
{
	for (int i = files.size() - 1; i >= 0; --i)
	{
		QString qPath = QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), files[i]));
		QFileInfo info(qPath);
		if (info.isDir())
		{
			files.erase(files.begin() + i);
			AddDirectorysContent(qPath, outputFiles);
		}
	}
}

void InsertFilePaths(const CAsset& asset, std::vector<string>& outputFiles)
{
	std::vector<string> files = asset.GetType()->GetAssetFiles(asset, false);

	ReplaceFoldersWithContent(files, outputFiles);

	std::move(files.begin(), files.end(), std::back_inserter(outputFiles));
}

}

std::vector<string> CAssetFilesProvider::GetForAsset(const CAsset& asset)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	InsertFilePaths(asset, filePaths);
	return filePaths;
}

std::vector<string> CAssetFilesProvider::GetForAssets(const std::vector<CAsset*>& assets)
{
	using namespace Private_AssetFileProvider;
	std::vector<string> filePaths;
	filePaths.reserve(assets.size() * 3 * 1.2);
	for (const CAsset* asset : assets)
	{
		InsertFilePaths(*asset, filePaths);
	}
	return filePaths;
}

std::vector<string> CAssetFilesProvider::GetForAssetWithoutMetafile(const CAsset& asset)
{
	std::vector<string> files = GetForAsset(asset);
	files.erase(std::remove(files.begin(), files.end(), asset.GetMetadataFile()));
	return files;
}
