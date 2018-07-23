// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PrefabAssetType.h"

#include <IEditor.h>
#include <Objects/ISelectionGroup.h>

// Prefab implementation detail
#include "Objects/ObjectManager.h"
#include "Objects/BaseObject.h"
#include <Objects/PrefabObject.h>
#include "Objects/SelectionGroup.h"

#include <Prefabs/PrefabItem.h>
#include <Prefabs/PrefabManager.h>
#include <IUndoObject.h>

#include <FilePathUtil.h>

REGISTER_ASSET_TYPE(CPrefabAssetType)

namespace Private_PrefabAssetType
{

bool MakeFromSelection(INewAsset& asset, const ISelectionGroup& selectedItems)
{
	CRY_ASSERT(selectedItems.GetCount());

	const string dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());

	CPrefabManager* const pPrefabManager = GetIEditor()->GetPrefabManager();
	CPrefabItem* const pPrefab = (CPrefabItem*)pPrefabManager->CreateItem(dataFilePath);
	if (!pPrefab)
	{
		return false;
	}

	CSelectionGroup selectionGroup;
	for (size_t i = 0, n = selectedItems.GetCount(); i < n; ++i)
	{
		selectionGroup.AddObject(selectedItems.GetObject(i));
	}

	pPrefab->SetName(asset.GetName());
	pPrefab->MakeFromSelection(selectionGroup);
	pPrefab->Update();

	pPrefabManager->SetSelectedItem(pPrefab);
	pPrefab->GetLibrary()->Save();

	asset.SetGUID(pPrefab->GetGUID());
	asset.AddFile(pPrefab->GetLibrary()->GetFilename());
	return true;
}

}

string CPrefabAssetType::GetObjectFilePath(const CAsset* pAsset) const
{
	return pAsset ? pAsset->GetGUID().ToString() : string();
}

bool CPrefabAssetType::DeleteAssetFiles(const CAsset& asset, bool bDeleteSourceFile, size_t& numberOfFilesDeleted) const
{
	CPrefabManager* pManager = GetIEditor()->GetPrefabManager();
	IDataBaseItem* item = pManager->FindItem(asset.GetGUID());
	if (!item)
	{
		return false;
	}
	pManager->DeleteItem(item);
	return CAssetType::DeleteAssetFiles(asset, bDeleteSourceFile, numberOfFilesDeleted);
}

bool CPrefabAssetType::OnCreate(INewAsset& asset, const void* pCreateParams) const
{
	using namespace Private_PrefabAssetType;

	const ISelectionGroup* const pSelectedItems = pCreateParams ? static_cast<const ISelectionGroup*>(pCreateParams) : GetIEditor()->GetISelectionGroup();
	if (!pSelectedItems || pSelectedItems->IsEmpty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Creating a New Prefab: No object is selected.\n"
		                                                       "Place several objects in the level that you wish to change to a prefab. Select all the objects that you require.");
		return false;
	}

	return MakeFromSelection(asset, *pSelectedItems);
}

CryIcon CPrefabAssetType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_prefab.ico");
}
