// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SkinnedMeshType.h"
#include "SandboxPlugin.h"
#include "AssetTypeHelpers.h"
#include "GenerateCharacter.h"

// For shared detail attributes.
#include "MeshType.h"
#include "SkeletonType.h"

#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/AssetEditor.h>

#include <FilePathUtil.h>

REGISTER_ASSET_TYPE(CSkinnedMeshType)

CAssetEditor* CSkinnedMeshType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Mesh", asset);
}

string CSkinnedMeshType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "skin");
}

std::vector<string> CSkinnedMeshType::GetImportExtensions() const
{
	return { "fbx" };
}

CryIcon CSkinnedMeshType::GetIcon() const
{
	return CryIcon("icons:Assets/Skin.ico");
}

CAsset* CSkinnedMeshType::Import(const string&, CAssetImportContext& context) const
{
	auto initMetaData = [&context](FbxMetaData::SMetaData& metaData)
	{
		metaData.outputFileExt = "skin";
		metaData.m_forwardUpAxes = GetFormattedAxes(context);
		metaData.materialFilename = GetMaterialName(context);

		ICharacterGenerator* const pCharGen = CreateCharacterGenerator(context);
		CRY_ASSERT(pCharGen);
		const FbxTool::CScene* pScene = pCharGen->GetScene();
		CRY_ASSERT(pScene);

		const FbxTool::SNode* pSkinNode = nullptr;
		for (int i = 0, N = pScene->GetNodeCount(); !pSkinNode && i < N; ++i)
		{
			const FbxTool::SNode* const pNode = pScene->GetNodeByIndex(i);
			for (int i = 0; !pSkinNode && i < pNode->numMeshes; ++i)
			{
				if (pNode->ppMeshes[i]->bHasSkin)
				{
					pSkinNode = pNode;
				}
			}
		}

		if (pSkinNode)
		{
			FbxMetaData::SNodeMeta nodeMeta;
			nodeMeta.name = pSkinNode->szName;
			nodeMeta.nodePath = FbxTool::GetPath(pSkinNode);
			metaData.nodeData.push_back(nodeMeta);
		}

		ApplySubMaterialMapping(metaData, context.GetSharedData(IMPORT_DATA_MATERIAL_MAPPING).toMap());
	};

	const string assetFilePath = ImportAsset(context.GetInputFilePath(), context.GetOutputDirectoryPath(), initMetaData);
	if (assetFilePath.empty())
	{
		return nullptr;
	}
	return context.LoadAsset(assetFilePath);
}

std::vector<CAssetType::SDetail> CSkinnedMeshType::GetDetails() const
{
	return
	{
		{ "triangleCount", &CMeshType::s_triangleCountAttribute },
		{ "vertexCount", &CMeshType::s_vertexCountAttribute },
		{ "materialCount", &CMeshType::s_materialCountAttribute },
		{ "bonesCount", &CSkeletonType::s_bonesCountAttribute }
	};
}