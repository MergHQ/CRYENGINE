// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "AssetFilesGroupProvider.h"
#include "Asset.h"
#include "AssetManager.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "FilePathUtil.h"

std::vector<string> CAssetFilesGroupProvider::GetFiles(bool includeGeneratedFile /*= true*/) const
{
	return m_pAsset ? m_pAsset->GetType()->GetAssetFiles(*m_pAsset, m_shouldIncludeSourceFile, false, includeGeneratedFile) : std::vector<string>();
}

string CAssetFilesGroupProvider::GetGeneratedFile() const
{
	return !m_pAsset || !m_pAsset->GetType()->HasThumbnail() ? "" : m_pAsset->GetThumbnailPath();
}

void CAssetFilesGroupProvider::Update()
{
	if (PathUtil::FileExists(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), m_metadata)))
	{
		auto pAssetManager = CAssetManager::GetInstance();
		CryLog("Executing reloading of asset %s", m_metadata.c_str());
		pAssetManager->MergeAssets(std::move(AssetLoader::CAssetFactory::LoadAssetsFromMetadataFiles({ m_metadata })));
		m_pAsset = pAssetManager->FindAssetForMetadata(m_metadata);
	}
	else
	{
		CryLog("No reloading of asset %s because the file doesn't exist.", m_metadata.c_str());
		m_pAsset = nullptr;
	}
}
