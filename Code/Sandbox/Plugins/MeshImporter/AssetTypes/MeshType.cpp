// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MeshType.h"
#include "SandboxPlugin.h"
#include "ImporterUtil.h"
#include "AssetTypeHelpers.h"

#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/AssetEditor.h>

#include <ProxyModels/ItemModelAttribute.h>

REGISTER_ASSET_TYPE(CMeshType);

// Detail attributes.
CItemModelAttribute CMeshType::s_triangleCountAttribute("Triangle count", eAttributeType_Int, CItemModelAttribute::StartHidden);
CItemModelAttribute CMeshType::s_vertexCountAttribute("Vertex count", eAttributeType_Int, CItemModelAttribute::StartHidden);
CItemModelAttribute CMeshType::s_materialCountAttribute("Material count", eAttributeType_Int, CItemModelAttribute::StartHidden);

CAssetEditor* CMeshType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Mesh", asset);
}

string CMeshType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "cgf");
}

std::vector<string> CMeshType::GetImportExtensions() const
{
	return { "fbx" };
}

CryIcon CMeshType::GetIcon() const
{
	return CryIcon("icons:Assets/Mesh.ico");
}

CAsset* CMeshType::Import(const string&, CAssetImportContext& context) const
{
	auto initMetaData = [&context](FbxMetaData::SMetaData& metaData)
	{
		metaData.outputFileExt = "cgf";
		metaData.m_forwardUpAxes = GetFormattedAxes(context);
		metaData.materialFilename = GetMaterialName(context);

		ApplySubMaterialMapping(metaData, context.GetSharedData(IMPORT_DATA_MATERIAL_MAPPING).toMap());
	};

	const string assetFilePath = ImportAsset(context.GetInputFilePath(), context.GetOutputDirectoryPath(), initMetaData);
	if (assetFilePath.empty())
	{
		return nullptr;
	}
	return context.LoadAsset(assetFilePath);
}

std::vector<CAssetType::SDetail> CMeshType::GetDetails() const
{
	return
	{
		{ "triangleCount", &s_triangleCountAttribute },
		{ "vertexCount", &s_vertexCountAttribute },
		{ "materialCount", &s_materialCountAttribute }
	};
}
