// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSStatusProvider.h"
#include "VersionControl.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/IFilesGroupController.h"
#include "AssetSystem/AssetManagerHelpers.h"
#include "Objects/IObjectLayer.h"
#include <CryString/CryPath.h>

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

}

int CAssetsVCSStatusProvider::GetStatus(const string& file)
{
	auto fs = CVersionControl::GetInstance().GetFileStatus(file);
	return fs ? fs->GetState() : CVersionControlFileStatus::eState_NotTracked;
}

int CAssetsVCSStatusProvider::GetStatus(const CAsset& asset)
{
	using namespace Private_AssetsVCSStatusProvider;
	return GetStatus(asset.GetMetadataFile());
}

int CAssetsVCSStatusProvider::GetStatus(const IFilesGroupController& pFileGroup)
{
	using namespace Private_AssetsVCSStatusProvider;
	return GetStatus(pFileGroup.GetMainFile());
}

int CAssetsVCSStatusProvider::GetStatus(const IObjectLayer& layer)
{
	using namespace Private_AssetsVCSStatusProvider;
	return GetStatus(PathUtil::MakeGamePath(layer.GetLayerFilepath()));
}

bool CAssetsVCSStatusProvider::HasStatus(const string& file, int status)
{
	auto fs = CVersionControl::GetInstance().GetFileStatus(file);
	return fs ? fs->HasState(status) : status & CVersionControlFileStatus::eState_NotTracked;
}

bool CAssetsVCSStatusProvider::HasStatus(const CAsset& asset, int status)
{
	using namespace Private_AssetsVCSStatusProvider;
	return HasStatus(asset.GetMetadataFile(), status);
}

bool CAssetsVCSStatusProvider::HasStatus(const IFilesGroupController& fileGroup, int status)
{
	using namespace Private_AssetsVCSStatusProvider;
	return HasStatus(fileGroup.GetMainFile(), status);
}

bool CAssetsVCSStatusProvider::HasStatus(const IObjectLayer& layer, int status)
{
	using namespace Private_AssetsVCSStatusProvider;
	return HasStatus(PathUtil::MakeGamePath(layer.GetLayerFilepath()), status);
}

void CAssetsVCSStatusProvider::UpdateStatus(const std::vector<CAsset*>& assets, const std::vector<string>& folders, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus(CAssetFilesProvider::GetForAssets(assets, CAssetFilesProvider::EInclude::OnlyMainFile), folders, std::move(callback));
}

void CAssetsVCSStatusProvider::UpdateStatus(const std::vector<IObjectLayer*>& layers, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus(CAssetFilesProvider::GetForLayers(layers, CAssetFilesProvider::EInclude::OnlyMainFile), {}, std::move(callback));
}

void CAssetsVCSStatusProvider::UpdateStatus(const IFilesGroupController& pFilesGroup, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus({ pFilesGroup.GetMainFile() }, {}, std::move(callback));
}

void CAssetsVCSStatusProvider::UpdateStatus(const std::vector<std::shared_ptr<IFilesGroupController>>& fileGroups, std::vector<string> folders, std::function<void()> callback /*= nullptr*/)
{
	using namespace Private_AssetsVCSStatusProvider;
	UpdateFilesStatus(CAssetFilesProvider::GetForFileGroups(fileGroups, CAssetFilesProvider::EInclude::OnlyMainFile), {}, std::move(callback));
}
