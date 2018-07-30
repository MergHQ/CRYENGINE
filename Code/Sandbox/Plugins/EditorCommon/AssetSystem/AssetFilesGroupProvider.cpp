// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "AssetFilesGroupProvider.h"
#include "Asset.h"
#include "AssetManager.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"

std::vector<string> CAssetFilesGroupProvider::GetFiles() const
{
	return m_pAsset->GetType()->GetAssetFiles(*m_pAsset, m_shouldIncludeSourceFile);
}

const string& CAssetFilesGroupProvider::GetName() const  
{
	return m_pAsset->GetName();
}

const string& CAssetFilesGroupProvider::GetMainFile() const  
{
	return m_metadata;
}

void CAssetFilesGroupProvider::Update()
{
	auto pAssetManager = CAssetManager::GetInstance();
	pAssetManager->MergeAssets(std::move(AssetLoader::CAssetFactory::LoadAssetsFromMetadataFiles({ m_metadata })));
	m_pAsset = pAssetManager->FindAssetForMetadata(m_metadata);
}
