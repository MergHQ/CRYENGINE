// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "LibraryAssetType.h"

#include "SchematycUtils.h"

#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/EditableAsset.h>
#include <QtUtil.h>

namespace CrySchematycEditor {

REGISTER_ASSET_TYPE(CLibraryAssetType)

bool CLibraryAssetType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	const string dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
	const string assetName = asset.GetName();

	// TODO: Actually the backend should ensure that the name is valid!
	Schematyc::CStackString uniqueAssetName(assetName.c_str());
	MakeScriptElementNameUnique(uniqueAssetName);
	// ~TODO

	Schematyc::IScriptRegistry& scriptRegistry = gEnv->pSchematyc->GetScriptRegistry();
	Schematyc::IScriptModule* pModule = scriptRegistry.AddModule(uniqueAssetName.c_str(), PathUtil::RemoveExtension(dataFilePath));
	if (pModule)
	{
		if (Schematyc::IScript* pScript = pModule->GetScript())
		{
			asset.AddFile(dataFilePath);

			scriptRegistry.SaveScript(*pScript);
			return true;
		}
	}

	return false;
}

CAssetEditor* CLibraryAssetType::Edit(CAsset* pAsset) const
{
	return CAssetEditor::OpenAssetForEdit("Schematyc Editor (Experimental)", pAsset);
}

bool CLibraryAssetType::RenameAsset(CAsset* pAsset, const char* szNewName) const
{
	if (pAsset)
	{
		Schematyc::IScript* pScript = GetScript(*pAsset);
		if (pScript)
		{
			stack_string prevName = pAsset->GetName();
			if (CAssetType::RenameAsset(pAsset, szNewName))
			{
				stack_string filePath = pScript->GetFilePath();
				filePath.replace(prevName.c_str(), szNewName);

				gEnv->pSchematyc->GetScriptRegistry().OnScriptRenamed(*pScript, filePath);
				return true;
			}
		}
	}

	return false;
}

void CLibraryAssetType::PreDeleteAssetFiles(const CAsset& asset) const
{
	Schematyc::IScript* pScript = GetScript(asset);
	if (pScript)
	{
		gEnv->pSchematyc->GetScriptRegistry().RemoveElement(pScript->GetRoot()->GetGUID());
	}
}

CryIcon CLibraryAssetType::GetIconInternal() const
{
	return CryIcon("icons:schematyc/assettype_library.ico");
}

Schematyc::IScript* CLibraryAssetType::GetScript(const CAsset& asset) const
{
	ICrySchematycCore* pSchematycCore = gEnv->pSchematyc;
	if (pSchematycCore)
	{
		Schematyc::IScriptRegistry& scriptRegistry = pSchematycCore->GetScriptRegistry();
		const size_t fileCount = asset.GetFilesCount();
		CRY_ASSERT_MESSAGE(fileCount == 1, "Asset '%s' has an unexpected amount of script files.", asset.GetName());
		if (fileCount > 0)
		{
			const stack_string szFile = PathUtil::GetGameFolder() + "/" + string(asset.GetFile(0));
			return scriptRegistry.GetScriptByFileName(szFile);
		}
	}
	return nullptr;
}
}
