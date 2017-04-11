// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SkeletonType.h"
#include "SandboxPlugin.h"
#include "AssetTypeHelpers.h"

#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/AssetEditor.h>

#include <ProxyModels/ItemModelAttribute.h>

REGISTER_ASSET_TYPE(CSkeletonType)

// Detail attributes.
CItemModelAttribute CSkeletonType::s_bonesCountAttribute("Bones count", eAttributeType_Int, CItemModelAttribute::StartHidden);

CAssetEditor* CSkeletonType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Skeleton", asset);
}

string CSkeletonType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "chr");
}

std::vector<string> CSkeletonType::GetImportExtensions() const
{
	return { "fbx" };
}

CryIcon CSkeletonType::GetIcon() const
{
	return CryIcon("icons:Assets/Skeleton.ico");
}

CAsset* CSkeletonType::Import(const string&, CAssetImportContext& context) const
{
	auto initMetaData = [](FbxMetaData::SMetaData& metaData)
	{
		metaData.outputFileExt = "chr";
	};

	const string assetFilePath = ImportAsset(context.GetInputFilePath(), context.GetOutputDirectoryPath(), initMetaData);
	if (assetFilePath.empty())
	{
		return nullptr;
	}
	return context.LoadAsset(assetFilePath);
}

std::vector<CAssetType::SDetail> CSkeletonType::GetDetails() const
{
	return
	{
		{ "bonesCount", &s_bonesCountAttribute }
	};
}
