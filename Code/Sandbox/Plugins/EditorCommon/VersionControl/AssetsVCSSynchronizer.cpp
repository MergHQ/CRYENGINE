// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSSynchronizer.h"
#include "VersionControl/VersionControl.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/IFileOperationsExecutor.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "AssetSystem/AssetManager.h"
#include "AssetsVCSStatusProvider.h"
#include "Objects/IObjectLayer.h"
#include "Objects/IObjectLayerManager.h"
#include "Objects/ObjectManager.h"
#include "PathUtils.h"
#include "QtUtil.h"
#include <QDir>

namespace Private_AssetsVCSSynchronizer
{

//! Finds strings in vec1 that are absent in vec2
std::vector<string> FindMissingStrings(std::vector<string>& vec1, std::vector<string>& vec2)
{
	std::sort(vec2.begin(), vec2.end(), stl::less_stricmp<string>());
	std::sort(vec1.begin(), vec1.end(), stl::less_stricmp<string>());

	std::vector<string> missingStrings;
	std::set_difference(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), back_inserter(missingStrings));
	return missingStrings;
}

//! Compares 2 lists of file names and downloads those files from the first list that are not present in the second one.
void CompareFilesAndDownloadMissing(std::vector<string> newFiles, std::vector<string>& originalFiles
	, std::function<void()> callback)
{
	std::vector<string> missingFiles = FindMissingStrings(newFiles, originalFiles);

	if (!missingFiles.empty())
	{
		CVersionControl::GetInstance().GetLatest(std::move(missingFiles), {}, {}, false, false
			, [callback = std::move(callback)](const CVersionControlResult&) 
		{
			callback();
		});
	}
	else
	{
		callback();
	}
}

//! Returns the list of all files that comprise given file groups.
std::vector<string> GetAllFiles(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups)
{
	std::vector<string> result;
	result.reserve(fileGroups.size() * 2);
	for (auto const& pFileGroup : fileGroups)
	{
		std::vector<string> files = pFileGroup->GetFiles(false);
		std::move(files.begin(), files.end(), std::back_inserter(result));
	}
	return result;
}

//! Returns the list of main files of given list of file groups.
std::vector<string> GetAllMainFiles(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups)
{
	std::vector<string> mainFiles;
	mainFiles.reserve(fileGroups.size());
	std::transform(fileGroups.cbegin(), fileGroups.cend(), std::back_inserter(mainFiles), [](const auto& pFileGroup)
	{
		return pFileGroup->GetMainFile();
	});
	return mainFiles;
}

//! Looks for .lyr files in the given directory and sub-directories and adds them to the vector.
void AddLayerFilesInFolderToVec(QDir dir, std::vector<string>& layersFiles)
{
	const auto& layers = dir.entryList({ "*.lyr" }, QDir::Files);
	std::transform(layers.cbegin(), layers.cend(), std::back_inserter(layersFiles), [&dir](const QString& str)
	{
		return QtUtil::ToString(dir.path() + '/' + str);
	});
	const auto& folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const auto& folder : folders)
	{
		AddLayerFilesInFolderToVec(dir.path() + '/' + folder, layersFiles);
	}
}

//! Returns the list of all .lyr files in the given folders recursively.
std::vector<string> GetLayersFilesInFolders(const std::vector<string>& folders)
{
	std::vector<string> layersFiles;
	for (const string& folder : folders)
	{
		AddLayerFilesInFolderToVec(QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameFolder(), folder)), layersFiles);
	}
	for (string& layersFile : layersFiles)
	{
		layersFile = PathUtil::ToGamePath(layersFile);
	}
	return layersFiles;
}

std::shared_ptr<std::vector<std::shared_ptr<IFilesGroupController>>> ToSharedVectorOfFileGroups(const std::vector<CAsset*>& vec)
{
	return std::make_shared<std::vector<std::shared_ptr<IFilesGroupController>>>(std::move(CAssetFilesProvider::ToFileGroups(vec)));
}

void GetLatestForFileGroups(std::shared_ptr<std::vector<std::shared_ptr<IFilesGroupController>>> pFileGroups, std::function<void()> callback = nullptr)
{
	if (!callback)
	{
		callback = [] {};
	}
	// move all remotely deleted items to the end of the vector.
	auto firstDeleteRemotelyIt = std::partition(pFileGroups->begin(), pFileGroups->end(), [](const auto& pFileGroup)
	{
		return !CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_DeletedRemotely);
	});
	auto firstDeletedIndex = std::distance(pFileGroups->begin(), firstDeleteRemotelyIt);

	auto originalFiles = GetAllFiles(*pFileGroups);

	auto onGetLatest = [originalFiles, pFileGroups, firstDeletedIndex
		, callback = std::move(callback)](const auto& result) mutable
	{
		// for the rest of the file we want to update theirs content to find missing files that we need to sync.
		for (auto& pFileGroup : *pFileGroups)
		{
			pFileGroup->Update();
		}

		// erase all deleted items because at this point (after sync) all local files should be gone.
		if (firstDeletedIndex != pFileGroups->size())
		{
			pFileGroups->erase(pFileGroups->begin() + firstDeletedIndex, pFileGroups->end());
		}

		if (pFileGroups->empty())
		{
			callback();
			return;
		}

		CompareFilesAndDownloadMissing(GetAllFiles(*pFileGroups), originalFiles, std::move(callback));
	};

	CVersionControl::GetInstance().GetLatest(std::move(originalFiles), {}, {}, false, false
		, std::move(onGetLatest));
}

void GetLatestForFolders(std::vector<string> folders, std::function<void()> callback)
{
	auto pAssetManager = CAssetManager::GetInstance();
	std::vector<CAsset*> existingAssets;
	for (const string& folder : folders)
	{
		auto folderAssets = pAssetManager->GetAssetsFromDirectory(folder);
		existingAssets.reserve(existingAssets.size() + folderAssets.size());
		existingAssets.insert(existingAssets.end(), folderAssets.cbegin(), folderAssets.cend());
	}
	if (!existingAssets.empty())
	{
		GetLatestForFileGroups(ToSharedVectorOfFileGroups(existingAssets));
	}

	CVersionControl::GetInstance().GetLatest({}, std::move(folders), { "cryasset", "lyr" }, false, false,
		[callback = std::move(callback)](const auto& result) mutable
	{
		std::vector<string> metadataFiles;
		metadataFiles.reserve(result.GetStatusChanges().size());
		IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
		for (const auto& update : result.GetStatusChanges())
		{
			if (update.GetFileName().compareNoCase(update.GetFileName().size() - 4, 4, ".lyr") == 0)
			{
				if (pLayerManager->IsLayerFileOfOpenedLevel(update.GetFileName()))
				{
					if (IObjectLayer* pLayer = pLayerManager->GetLayerByFileIfOpened(update.GetFileName()))
					{
						pLayerManager->DeleteLayer(pLayer);
					}
					pLayerManager->ImportLayerFromFile(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), update.GetFileName()));
				}
			}
			else
			{
				metadataFiles.push_back(update.GetFileName());
			}
		}

		if (metadataFiles.empty())
		{
			callback();
			return;
		}

		auto pAssetManager = CAssetManager::GetInstance();
		pAssetManager->MergeAssets(AssetLoader::CAssetFactory::LoadAssetsFromMetadataFiles(metadataFiles));
		std::vector<CAsset*> assets;
		assets.reserve(metadataFiles.size());
		for (const string& metadataFile : metadataFiles)
		{
			assets.push_back(pAssetManager->FindAssetForMetadata(metadataFile));
		}
		GetLatestForFileGroups(ToSharedVectorOfFileGroups(assets), std::move(callback));
	});
}

}

void CAssetsVCSSynchronizer::Sync(std::vector<std::shared_ptr<IFilesGroupController>> fileGroups, std::vector<string> folders
	, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSSynchronizer;
	if (!callback)
	{
		callback = [] {};
	}

	// convert to shared_ptr to avoid copying of the vector
	auto pFileGroups = std::make_shared<decltype(fileGroups)>(std::move(fileGroups));

	CAssetsVCSStatusProvider::UpdateStatus(*pFileGroups, {}, [pFileGroups, folders = std::move(folders)
		, callback = std::move(callback)]() mutable
	{
		using FS = CVersionControlFileStatus;
		// move all not changed remotely items to the end of the vector.
		auto firstNotChangedRemotelyIt = std::partition(pFileGroups->begin(), pFileGroups->end(), [](const auto& pFileGroup)
		{
			return CAssetsVCSStatusProvider::HasStatus(*pFileGroup, FS::eState_UpdatedRemotely | FS::eState_DeletedRemotely);
		});
		// remove them since they don't need to be synchronized.
		if (firstNotChangedRemotelyIt != pFileGroups->end())
		{
			pFileGroups->erase(firstNotChangedRemotelyIt, pFileGroups->end());
		}

		if (pFileGroups->empty())
		{
			if (folders.empty())
			{
				callback();
			}
			else
			{
				GetLatestForFolders(std::move(folders), std::move(callback));
			}
			return;
		}

		GetLatestForFileGroups(pFileGroups, callback = folders.empty() ? std::move(callback) : [] {});

		if (!folders.empty())
		{
			GetLatestForFolders(std::move(folders), std::move(callback));
		}
	});
}

void CAssetsVCSSynchronizer::Sync(const std::vector<CAsset*>& assets, std::vector<string> folders
	, std::function<void()> callback /*= nullptr*/)
{
	Sync(CAssetFilesProvider::ToFileGroups(assets), std::move(folders), std::move(callback));
}

void CAssetsVCSSynchronizer::Sync(const std::shared_ptr<IFilesGroupController>& pFileGroup, std::function<void()> callback /*= nullptr*/)
{
	Sync({ pFileGroup }, {}, std::move(callback));
}

void CAssetsVCSSynchronizer::Sync(const std::vector<IObjectLayer*>& layers, std::vector<string> folders
	, std::function<void()> callback /*= nullptr*/)
{
	Sync(CAssetFilesProvider::ToFileGroups(layers), std::move(folders), std::move(callback));
}
