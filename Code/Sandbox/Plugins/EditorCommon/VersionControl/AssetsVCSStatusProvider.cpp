// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSStatusProvider.h"
#include "VersionControl.h"
#include "AssetSystem/IFilesGroupProvider.h"
#include "AssetSystem/AssetManagerHelpers.h"

namespace Private_AssetsVCSStatusProvider
{

void UpdateFilesStatus(std::vector<string> files, const std::vector<string>& folders, std::function<void()> callback)
{
	CVersionControl::GetInstance().UpdateFileStatus(std::move(files), folders, false
		, [callback = std::move(callback)](const auto& result)
	{
		if (callback)
		{
			callback();
		}
	});
}

int GetFileStatus(const string& file)
{
	auto fs = CVersionControl::GetInstance().GetFileStatus(file);
	return fs ? fs->GetState() : CVersionControlFileStatus::eState_NotTracked;
}

bool HasFileStatus(const string& file, int status)
{
	auto fs = CVersionControl::GetInstance().GetFileStatus(file);
	return fs ? fs->HasState(status) : false;
}

}

int CAssetsVCSStatusProvider::GetStatus(const CAsset& asset)
{
	using namespace Private_AssetsVCSStatusProvider;
	return GetFileStatus(asset.GetMetadataFile());
}

int CAssetsVCSStatusProvider::GetStatus(const IFilesGroupProvider& pFileGroup)
{
	using namespace Private_AssetsVCSStatusProvider;
	return GetFileStatus(pFileGroup.GetMainFile());
}

bool CAssetsVCSStatusProvider::HasStatus(const CAsset& asset, int status)
{
	using namespace Private_AssetsVCSStatusProvider;
	return HasFileStatus(asset.GetMetadataFile(), status);
}

bool CAssetsVCSStatusProvider::HasStatus(const IFilesGroupProvider& pFileGroup, int status)
{
	using namespace Private_AssetsVCSStatusProvider;
	return HasFileStatus(pFileGroup.GetMainFile(), status);
}

void CAssetsVCSStatusProvider::UpdateStatus(const std::vector<CAsset*>& assets, const std::vector<string>& folders, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus(AssetManagerHelpers::GetListOfMetadataFiles(assets), folders, std::move(callback));
}

void CAssetsVCSStatusProvider::UpdateStatus(const IFilesGroupProvider& pFilesGroup, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus(pFilesGroup.GetFiles(), {}, std::move(callback));
}
