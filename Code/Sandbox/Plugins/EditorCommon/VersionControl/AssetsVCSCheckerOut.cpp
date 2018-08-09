// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSCheckerOut.h"
#include "VersionControl/VersionControl.h"
#include "AssetFilesProvider.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSSynchronizer.h"
#include <algorithm>

namespace Private_AssetsVCSCheckerOut
{

void SyncDeletedAssets(std::vector<CAsset*>& assets, std::vector<CAsset*>::iterator firstDeletedIt)
{
	auto numDeleted = std::distance(firstDeletedIt, assets.end());
	if (numDeleted > 0)
	{
		std::vector<CAsset*> deletedAssets;
		deletedAssets.reserve(numDeleted);
		std::move(firstDeletedIt, assets.end(), std::back_inserter(deletedAssets));
		assets.erase(firstDeletedIt, assets.end());
		CAssetsVCSSynchronizer::Sync(std::move(deletedAssets), {});
	}
}

void DoCheckOutAssets(const std::vector<CAsset*>& assets)
{
	CVersionControl::GetInstance().EditFiles(CAssetFilesProvider::GetForAssets(assets), false, [](const auto& result)
	{
		if (result.GetError().type == EVersionControlError::AlreadyCheckedOutByOthers)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't check out exclusively checked out file(s)");
		}
	});
}

void UpdateChangedAndCheckOutAll(std::vector<CAsset*> assets, std::vector<CAsset*>::iterator firstChangedRemotelyIt)
{
	auto numUpdated = std::distance(firstChangedRemotelyIt, assets.end());
	if (numUpdated > 0)
	{
		std::vector<CAsset*> updatedAssets;
		updatedAssets.reserve(numUpdated);
		std::copy(firstChangedRemotelyIt, assets.end(), std::back_inserter(updatedAssets));
		CAssetsVCSSynchronizer::Sync(std::move(updatedAssets), {}, [assets = std::move(assets)]()
		{
			DoCheckOutAssets(assets);
		});
	}
	else
	{
		DoCheckOutAssets(assets);
	}
}

}

void CAssetsVCSCheckerOut::CheckOutAssets(std::vector<CAsset*> assets)
{
	using namespace Private_AssetsVCSCheckerOut;

	auto it = std::remove_if(assets.begin(), assets.end(), [](CAsset* pAsset)
	{
		return strcmp(pAsset->GetType()->GetTypeName(), "Level") == 0;
	});
	if (it != assets.end())
	{
		assets.erase(it);
	}

	CAssetsVCSStatusProvider::UpdateStatus(assets, {}, [assets = std::move(assets)]() mutable
	{
		// move all remotely change assets to the back of the array.
		auto firstChangedRemotelyIt = std::partition(assets.begin(), assets.end(), [](CAsset* pAsset)
		{
			using FS = CVersionControlFileStatus;
			static const int changedRemotelyState = FS::eState_DeletedRemotely | FS::eState_UpdatedRemotely;
			return !CAssetsVCSStatusProvider::HasStatus(*pAsset, changedRemotelyState);
		});
		// from remotely changes range move remotely deleted assets to the back.
		auto firstDeletedIt = std::partition(firstChangedRemotelyIt, assets.end(), [](CAsset* pAsset)
		{
			return !CAssetsVCSStatusProvider::HasStatus(*pAsset, CVersionControlFileStatus::eState_DeletedRemotely);
		});

		// this syncs deleted assets and removes them from the array.
		SyncDeletedAssets(assets, firstDeletedIt);

		UpdateChangedAndCheckOutAll(std::move(assets), firstChangedRemotelyIt);
	});
}
