// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AnimationType.h"
#include "SandboxPlugin.h"
#include "ImporterUtil.h"
#include "GenerateCharacter.h"
#include "RcCaller.h"

#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/AssetEditor.h>
#include <FilePathUtil.h>

REGISTER_ASSET_TYPE(CAnimationType)

CAssetEditor* CAnimationType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Animation", asset);
}

string CAnimationType::GetTargetFilename(const string& sourceFilename) const
{
	return PathUtil::ReplaceExtension(sourceFilename, "caf");
}

std::vector<string> CAnimationType::GetImportExtensions() const
{
	return { "fbx" };
}

CryIcon CAnimationType::GetIcon() const
{
	return CryIcon("icons:Assets/Animation.ico");
}

CAsset* CAnimationType::Import(const string&, CAssetImportContext& context) const
{	
	const auto absTargetFilePath = PathUtil::GetGameProjectAssetsPath() + "/" + context.GetOutputDirectoryPath() + "/" + PathUtil::GetFile(context.GetInputFilePath().c_str());

	CFileImporter fileImporter;
	if (!fileImporter.Import(context.GetInputFilePath(), absTargetFilePath))
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, fileImporter.GetError().c_str());
		return nullptr;
	}

	std::vector<string> outputFiles = GenerateAllAnimations(absTargetFilePath);
	if (outputFiles.empty())
	{
		return nullptr;
	}

	// TODO: Returns vector of assets :!
	return context.LoadAsset(PathUtil::ReplaceExtension(outputFiles[0], ".caf.cryasset"));
}
