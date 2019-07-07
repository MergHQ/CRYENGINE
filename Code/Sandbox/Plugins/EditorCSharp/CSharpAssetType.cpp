// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "CSharpAssetType.h"

#include <AssetSystem/Loader/AssetLoaderHelpers.h>
#include <AssetSystem/Asset.h>

#include <CrySystem/IProjectManager.h>

#include <cctype> 
#include <clocale>
#include "CSharpEditorPlugin.h"

REGISTER_ASSET_TYPE(CSharpSourcefileAssetType)

CryIcon CSharpSourcefileAssetType::GetIconInternal() const
{
	return CryIcon("icons:csharp/assets_csharp.ico");
}

string CSharpSourcefileAssetType::GetCleanName(const string& name) const
{
	string cleanName;
	cleanName.reserve(name.size());
	for (int c : name)
	{
		if (std::isalpha(c) != 0)
		{
			cleanName += c;
		}
		else if (std::isdigit(c) != 0)
		{
			cleanName += c;
		}
	}
	return cleanName;
}

CAssetEditor* CSharpSourcefileAssetType::Edit(CAsset* pAsset) const
{
#if CRY_PLATFORM_WINDOWS
	if (pAsset->GetFilesCount() > 0)
	{
		string filePath = PathUtil::Make(gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryAbsolute(), pAsset->GetFile(0));
		CCSharpEditorPlugin::GetInstance()->OpenCSharpFile(filePath, 10);
	}
#endif

	return nullptr;
}

bool CSharpSourcefileAssetType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	const string basePath = PathUtil::RemoveExtension(PathUtil::RemoveExtension(asset.GetMetadataFile()));
	const string csFilePath = basePath + ".cs";

	string projectName = gEnv->pSystem->GetIProjectManager()->GetCurrentProjectName();

	string cleanProjectName = GetCleanName(projectName);
	string cleanAssetName = GetCleanName(asset.GetName());
	CryGUID guid = CryGUID::Create();

	CCryFile assetFile(csFilePath.c_str(), "wb", ICryPak::FLAGS_NO_LOWCASE);
	if (assetFile.GetHandle() != nullptr)
	{
		string assetContents = gEnv->pSystem->GetIProjectManager()->LoadTemplateFile("%ENGINE%/EngineAssets/Templates/ManagedAsset.cs.txt", [this, cleanProjectName, cleanAssetName, guid](const char* szAlias) -> string
		{
			if (!strcmp(szAlias, "namespace"))
			{
				return cleanProjectName;
			}
			else if (!strcmp(szAlias, "guid"))
			{
				return guid.ToString();
			}
			else if (!strcmp(szAlias, "class_name"))
			{
				return cleanAssetName;
			}

			CRY_ASSERT_MESSAGE(false, "Unhandled alias!");
			return "";
		});

		if (assetFile.Write(assetContents.data(), assetContents.size()))
		{
			asset.SetFiles({ csFilePath });
		}
	}

	return true;
}
