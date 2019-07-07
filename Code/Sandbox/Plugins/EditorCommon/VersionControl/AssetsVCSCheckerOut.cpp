// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSCheckerOut.h"
#include "VersionControl/VersionControl.h"
#include "AssetFilesProvider.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSSynchronizer.h"
#include "AssetSystem/AssetType.h"

#include <CrySystem/ISystem.h>

#include <algorithm>

namespace Private_AssetsVCSCheckerOut
{

void SyncDeletedAssets(std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups, std::vector<std::shared_ptr<IFilesGroupController>>::iterator firstDeletedIt)
{
	const auto numDeleted = std::distance(firstDeletedIt, fileGroups.end());
	if (numDeleted > 0)
	{
		std::vector<std::shared_ptr<IFilesGroupController>> deletedFileGroups;
		deletedFileGroups.reserve(numDeleted);
		std::move(firstDeletedIt, fileGroups.end(), std::back_inserter(deletedFileGroups));
		fileGroups.erase(firstDeletedIt, fileGroups.end());
		CAssetsVCSSynchronizer::Sync(std::move(deletedFileGroups), {});
	}
}

void DoCheckOutAssets(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups)
{
	CVersionControl::GetInstance().EditFiles(CAssetFilesProvider::GetForFileGroups(fileGroups));
}

void UpdateChangedAndCheckOutAll(std::vector<std::shared_ptr<IFilesGroupController>> fileGroups, std::vector<std::shared_ptr<IFilesGroupController>>::iterator firstChangedRemotelyIt)
{
	auto numUpdated = std::distance(firstChangedRemotelyIt, fileGroups.end());
	if (numUpdated > 0)
	{
		std::vector<std::shared_ptr<IFilesGroupController>> updatedFileGroups;
		updatedFileGroups.reserve(numUpdated);
		std::copy(firstChangedRemotelyIt, fileGroups.end(), std::back_inserter(updatedFileGroups));
		CAssetsVCSSynchronizer::Sync(std::move(updatedFileGroups), {}, [fileGroups = std::move(fileGroups)]()
		{
			DoCheckOutAssets(fileGroups);
		});
	}
	else
	{
		DoCheckOutAssets(fileGroups);
	}
}

}

void CAssetsVCSCheckerOut::CheckOut(const std::vector<CAsset*>& assets)
{
	CheckOut(CAssetFilesProvider::ToFileGroups(assets));
}

void CAssetsVCSCheckerOut::CheckOut(const std::vector<IObjectLayer*>& layers)
{
	CheckOut(CAssetFilesProvider::ToFileGroups(layers));
}

void CAssetsVCSCheckerOut::CheckOut(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups)
{
	using namespace Private_AssetsVCSCheckerOut;

	CAssetsVCSStatusProvider::UpdateStatus(fileGroups, {}, [fileGroups = fileGroups]() mutable
	{
		// move all remotely change assets to the back of the array.
		auto firstChangedRemotelyIt = std::partition(fileGroups.begin(), fileGroups.end(), [](const auto& pFileGroup)
		{
			using FS = CVersionControlFileStatus;
			static const int changedRemotelyState = FS::eState_DeletedRemotely | FS::eState_UpdatedRemotely;
			return !CAssetsVCSStatusProvider::HasStatus(*pFileGroup, changedRemotelyState);
		});
		// from remotely changes range move remotely deleted assets to the back.
		auto firstDeletedIt = std::partition(firstChangedRemotelyIt, fileGroups.end(), [](const auto& pFileGroup)
		{
			return !CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_DeletedRemotely);
		});

		// this syncs deleted assets and removes them from the array.
		SyncDeletedAssets(fileGroups, firstDeletedIt);

		UpdateChangedAndCheckOutAll(std::move(fileGroups), firstChangedRemotelyIt);
	});
}
