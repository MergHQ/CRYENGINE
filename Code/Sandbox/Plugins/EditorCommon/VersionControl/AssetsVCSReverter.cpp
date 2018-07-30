// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSReverter.h"
#include "VersionControl/VersionControl.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetSystem/AssetManager.h"
#include <algorithm>

namespace Private_AssetsVCSReverter
{

std::vector<CAsset*> FindAddedLocallyAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	auto isAddedPred = [](CAsset* pAsset)
	{
		return CAssetsVCSStatusProvider::HasStatus(*pAsset, CVersionControlFileStatus::eState_AddedLocally);
	};

	std::vector<CAsset*> addedAssets;

	std::copy_if(assets.cbegin(), assets.cend(), std::back_inserter(addedAssets), isAddedPred);

	for (const string& folder : folders)
	{
		auto tmpAssets = CAssetManager::GetInstance()->GetAssetsFromDirectory(folder, isAddedPred);
		std::move(tmpAssets.begin(), tmpAssets.end(), std::back_inserter(addedAssets));
	}
	return addedAssets;
}

}

void CAssetsVCSReverter::RevertAssets(const std::vector<CAsset*>& assets, std::vector<string> folders)
{
	using namespace Private_AssetsVCSReverter;

	std::vector<CAsset*> addedAssets = FindAddedLocallyAssets(assets, folders);
	
	const auto question = addedAssets.empty() ? QObject::tr("Are you sure you want to revert all changes?") :
		QObject::tr("Are you sure you want to revert all changes? Some assets are added locally and will be deleted");

	if (CQuestionDialog::SQuestion(QObject::tr("Revert assets"), question) != QDialogButtonBox::Yes)
	{
		return;
	}

	if (!addedAssets.empty())
	{
		CAssetManager::GetInstance()->DeleteAssetsWithFiles(std::move(addedAssets));
	}

	std::vector<string> files;
	files.reserve(assets.size() * 4);
	for (CAsset* pAsset : assets)
	{
		const auto& assetFiles = pAsset->GetFiles();
		std::copy(assetFiles.cbegin(), assetFiles.cend(), std::back_inserter(files));
		files.push_back(pAsset->GetMetadataFile());
	}

	CVersionControl::GetInstance().Revert(std::move(files), std::move(folders));
}
