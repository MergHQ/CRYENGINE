// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MaterialType.h"
#include "AssetTypeHelpers.h"
#include "AsyncHelper.h"
#include "SandboxPlugin.h"
#include "ImporterUtil.h"
#include "GenerateCharacter.h"
#include "RcCaller.h"

#include <AssetSystem/EditableAsset.h>
#include <AssetSystem/AssetImportContext.h>
#include <FilePathUtil.h>
#include <ThreadingUtils.h>

REGISTER_ASSET_TYPE(CMaterialType)

string CMaterialType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "mtl");
}

std::vector<string> CMaterialType::GetImportExtensions() const
{
	return { "fbx" };
}

CryIcon CMaterialType::GetIcon() const
{
	return CryIcon("icons:Assets/Material.ico");
}

CAsset* CMaterialType::Import(const string&, CAssetImportContext& context) const
{
	const auto absTargetFilePath = PathUtil::GetGameProjectAssetsPath() + "/" + context.GetOutputDirectoryPath() + "/" + PathUtil::GetFile(context.GetInputFilePath().c_str());

	CFileImporter fileImporter;
	if (!fileImporter.Import(context.GetInputFilePath(), absTargetFilePath))
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, fileImporter.GetError().c_str());
		return nullptr;
	}

	auto charGen = CreateCharacterGenerator(context);
	ThreadingUtils::PostOnMainThread([charGen]() { charGen->CreateMaterial(); }).wait();
	const std::vector<string> outputFiles = charGen->GetOutputFiles();

	// Store sub-material ID mapping in context.
	{
		auto map = GetSubMaterialMapping(charGen->GetScene());
		if (!map.isEmpty())
		{
			context.SetSharedData(IMPORT_DATA_MATERIAL_MAPPING, map);
		}
	}

	string theMtlFile;

	// Write cryasset.
	{
		CRcCaller rcCaller;
		rcCaller.OverwriteExtension("cryasset");
		for (const string& f : outputFiles)
		{
			if (!stricmp(PathUtil::GetExt(f), "mtl"))
			{
				theMtlFile = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), f);
			}

			if (!rcCaller.Call(theMtlFile))
			{
				CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, string().Format("Compiling file '%s' failed.", f.c_str()));
			}
		}
	}

	CRY_ASSERT(!strcmp(PathUtil::GetExt(theMtlFile), "mtl"));
	const string assetMetaDataFile = PathUtil::ToGamePath(theMtlFile).append(".cryasset");

	CAsset* const pAsset = context.LoadAsset(assetMetaDataFile);
	if (!pAsset)
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "Compiling file '%s' failed. Cannot load asset '%s'", theMtlFile.c_str(), assetMetaDataFile.c_str());
		return nullptr;
	}

	CEditableAsset editableAsset = context.CreateEditableAsset(*pAsset);
	editableAsset.SetSourceFile(PathUtil::GetFile(context.GetInputFilePath()));
	editableAsset.WriteToFile();
	return pAsset;
}
