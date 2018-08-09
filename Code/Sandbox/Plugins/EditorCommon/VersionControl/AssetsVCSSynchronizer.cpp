// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSSynchronizer.h"
#include "VersionControl/VersionControl.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "AssetSystem/IFileOperationsExecutor.h"
#include "AssetSystem/AssetManagerHelpers.h"
#include "AssetsVCSStatusProvider.h"
#include <algorithm>

namespace Private_AssetsVCSSynchronizer
{

void CompareFilesAndDownloadMissing(std::vector<string> newFiles, std::vector<string>& originalFiles
	, std::function<void()> callback)
{
	std::vector<string> missingFiles = CAssetsVCSSynchronizer::FindMissingStrings(newFiles, originalFiles);

	if (!missingFiles.empty())
	{
		CVersionControl::GetInstance().GetLatest(std::move(missingFiles), {}, false, false
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

void UpdateAssetsAndDownloadMissingFiles(const std::vector<string>& metadataFiles, std::vector<string>& originalFiles
	, std::function<void()> callback)
{
	auto pAssetManager = GetIEditor()->GetAssetManager();
	pAssetManager->MergeAssets(std::move(AssetLoader::CAssetFactory::LoadAssetsFromMetadataFiles(metadataFiles)));
	std::vector<CAsset*> assets;
	assets.reserve(metadataFiles.size());
	for (const string& metadata : metadataFiles)
	{
		assets.push_back(pAssetManager->FindAssetForMetadata(metadata));
	}

	CompareFilesAndDownloadMissing(CAssetFilesProvider::GetForAssets(assets), originalFiles, std::move(callback));
}

}

std::vector<string> CAssetsVCSSynchronizer::FindMissingStrings(std::vector<string>& vec1, std::vector<string>& vec2)
{
	std::sort(vec2.begin(), vec2.end(), stl::less_stricmp<string>());
	std::sort(vec1.begin(), vec1.end(), stl::less_stricmp<string>());

	std::vector<string> missingStrings;
	std::set_difference(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), back_inserter(missingStrings));
	return missingStrings;
}

void CAssetsVCSSynchronizer::Sync(std::vector<CAsset*> assets, std::vector<string> folders
	, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSSynchronizer;
	if (!callback)
	{
		callback = []{};
	}

	auto pAssets = std::make_shared<decltype(assets)>(std::move(assets));

	CAssetsVCSStatusProvider::UpdateStatus(*pAssets, {}, [pAssets, folders = std::move(folders)
		, callback = std::move(callback)]() mutable
	{
		auto firstDeleteRemotelyIt = std::partition(pAssets->begin(), pAssets->end(), [](CAsset* pAsset)
		{
			return !CAssetsVCSStatusProvider::HasStatus(*pAsset, CVersionControlFileStatus::eState_DeletedRemotely);
		});
		auto firstDeletedIndex = std::distance(pAssets->begin(), firstDeleteRemotelyIt);

		std::vector<string> metadataFiles = AssetManagerHelpers::GetListOfMetadataFiles(*pAssets);
		std::vector<string> originalFiles = CAssetFilesProvider::GetForAssets(*pAssets);

		CVersionControl::GetInstance().GetLatest(std::move(originalFiles), std::move(folders), false, false
            , [originalFiles, metadataFiles = std::move(metadataFiles), firstDeletedIndex, callback = std::move(callback)](const auto& result) mutable
		{
			if (firstDeletedIndex == metadataFiles.size())
			{
                callback();
				return;
			}

			metadataFiles.erase(metadataFiles.begin() + firstDeletedIndex, metadataFiles.end());

			UpdateAssetsAndDownloadMissingFiles(metadataFiles, originalFiles, std::move(callback));
		});
	});
}

void CAssetsVCSSynchronizer::Sync(std::shared_ptr<IFilesGroupProvider> pFileGroup, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSSynchronizer;
	if (!callback)
	{
		callback = [] {};
	}
	bool isDeleted = CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_DeletedRemotely);
	if (isDeleted || CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_UpdatedRemotely))
	{
		auto pOriginalFiles = std::make_shared<std::vector<string>>(pFileGroup->GetFiles());
		CVersionControl::GetInstance().GetLatest(*pOriginalFiles, {}, false, false
			, [callback = std::move(callback), pFileGroup, pOriginalFiles, isDeleted](const auto& result)
		{
			if (isDeleted)
			{
				callback();
				return;
			}

			pFileGroup->Update();
			CompareFilesAndDownloadMissing(pFileGroup->GetFiles(), *pOriginalFiles, std::move(callback));
		});
	}
}

std::vector<string> CAssetsVCSSynchronizer::GetMissingAssetsFiles(const std::vector<CAsset*>& assets, std::vector<string>& originalFiles)
{
	using namespace Private_AssetsVCSSynchronizer;
	return FindMissingStrings(CAssetFilesProvider::GetForAssets(assets), originalFiles);
}
