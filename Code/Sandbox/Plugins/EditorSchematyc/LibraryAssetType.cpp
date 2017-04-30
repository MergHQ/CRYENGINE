// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "LibraryAssetType.h"

#include "SchematycUtils.h"

#include <AssetSystem/EditableAsset.h>
#include <QtUtil.h>

namespace CrySchematycEditor {

REGISTER_ASSET_TYPE(CLibraryAssetType)

bool CLibraryAssetType::OnCreate(CEditableAsset& editAsset) const
{
	const string szFilePath = PathUtil::RemoveExtension(PathUtil::RemoveExtension(editAsset.GetAsset().GetMetadataFile()));
	const QString basePath = szFilePath.c_str();
	const QString assetName = basePath.section('/', -1);

	// TODO: Actually the backend should ensure that the name is valid!
	Schematyc::CStackString uniqueAssetName = QtUtil::ToString(assetName).c_str();
	MakeScriptElementNameUnique(uniqueAssetName);
	// ~TODO

	Schematyc::IScriptRegistry& scriptRegistry = gEnv->pSchematyc->GetScriptRegistry();
	Schematyc::IScriptModule* pModule = scriptRegistry.AddModule(uniqueAssetName.c_str(), szFilePath.c_str());
	if (pModule)
	{
		if (Schematyc::IScript* pScript = pModule->GetScript())
		{
			const QString assetFilePath = basePath + "." + GetFileExtension();
			editAsset.AddFile(QtUtil::ToString(assetFilePath).c_str());

			scriptRegistry.SaveScript(*pScript);
			return true;
		}
	}

	return false;
}

CAssetEditor* CLibraryAssetType::Edit(CAsset* pAsset) const
{
	return CAssetEditor::OpenAssetForEdit("Schematyc Editor", pAsset);
}

bool CLibraryAssetType::DeleteAssetFiles(const CAsset& asset, bool bDeleteSourceFile, size_t& numberOfFilesDeleted) const
{
	if (bDeleteSourceFile)
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
				Schematyc::IScript* pScript = scriptRegistry.GetScriptByFileName(szFile);
				if (pScript)
				{
					scriptRegistry.RemoveElement(pScript->GetRoot()->GetGUID());
				}
			}
			return CAssetType::DeleteAssetFiles(asset, bDeleteSourceFile, numberOfFilesDeleted);
		}
		return false;
	}

	return true;
}

CryIcon CLibraryAssetType::GetIcon() const
{
	return CryIcon();
}

}
