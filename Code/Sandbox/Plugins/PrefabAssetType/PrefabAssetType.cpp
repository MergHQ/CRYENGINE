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

#include <PathUtils.h>

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

CryIcon CPrefabAssetType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_prefab.ico");
}

string CPrefabAssetType::GetObjectFilePath(const CAsset* pAsset) const
{
	return pAsset ? pAsset->GetGUID().ToString() : string();
}

void CPrefabAssetType::PreDeleteAssetFiles(const CAsset& asset) const
{
	CPrefabManager* pManager = GetIEditor()->GetPrefabManager();
	IDataBaseItem* item = pManager->FindItem(asset.GetGUID());
	if (!item)
	{
		return;
	}
	pManager->DeleteItem(item);
}

bool CPrefabAssetType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	using namespace Private_PrefabAssetType;

	const ISelectionGroup* const pSelectedItems = pCreateParams ? static_cast<const SPrefabCreateParams*>(pCreateParams)->pGroup : GetIEditor()->GetISelectionGroup();
	if (!pSelectedItems || pSelectedItems->IsEmpty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Creating a New Prefab: No object is selected.\n"
		                                                       "Place several objects in the level that you wish to change to a prefab. Select all the objects that you require.");
		return false;
	}

	return MakeFromSelection(asset, *pSelectedItems);
}

bool CPrefabAssetType::OnCopy(INewAsset& asset, CAsset& assetToCopy) const
{
	CPrefabManager* const pManager = GetIEditor()->GetPrefabManager();

	if (!pManager->FindLibrary(assetToCopy.GetFile(0)))
	{
		pManager->LoadLibrary(assetToCopy.GetFile(0));
	}

	IDataBaseItem* const pPrefab = pManager->FindItem(assetToCopy.GetGUID());
	if (!pPrefab)
	{
		CryWarning(EValidatorModule::VALIDATOR_MODULE_EDITOR, EValidatorSeverity::VALIDATOR_ERROR,
			"Unable to copy prefab with id \"%s\".\n"
			"Please make sure that the file \"%s\" exists and has the corresponding id.",
			assetToCopy.GetGUID().ToString().c_str(),
			assetToCopy.GetFile(0).c_str());
		return false;
	}

	std::unique_ptr<CPrefabItem> pNewPrefab(static_cast<CPrefabItem*>(pPrefab)->CreateCopy());
	if (!pNewPrefab)
	{
		return false;
	}
	pNewPrefab->SetName(asset.GetName());

	const auto dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
	const auto libraryName = PathUtil::RemoveExtension(dataFilePath);
	IDataBaseLibrary* const pLibrary = pManager->AddLibrary(libraryName);
	if (!pLibrary)
	{
		return false;
	}

	asset.SetGUID(pNewPrefab->GetGUID());
	asset.AddFile(pLibrary->GetFilename());

	pLibrary->AddItem(pNewPrefab.release());
	return pLibrary->Save();
}

bool CPrefabAssetType::RenameAsset(CAsset* pAsset, const char* szNewName) const
{
	CPrefabManager* const pManager = GetIEditor()->GetPrefabManager();

	if (!pManager->FindLibrary(pAsset->GetFile(0)))
	{
		pManager->LoadLibrary(pAsset->GetFile(0));
	}

	IDataBaseItem* const pItem = pManager->FindItem(pAsset->GetGUID());
	if (!pItem)
	{
		CryWarning(EValidatorModule::VALIDATOR_MODULE_EDITOR, EValidatorSeverity::VALIDATOR_ERROR,
			"Unable to rename prefab with id \"%s\".\n"
			"Please make sure that the file \"%s\" exists and has the corresponding id.",
			pAsset->GetGUID().ToString().c_str(),
			pAsset->GetFile(0).c_str());
		return false;
	}

	if (!CAssetType::RenameAsset(pAsset, szNewName))
	{
		return false;
	}

	pItem->SetName(pAsset->GetName());
	CBaseLibrary* const pLibrary = static_cast<CBaseLibrary*>(pItem->GetLibrary());
	pLibrary->SetFilename(pAsset->GetFile(0));
	pLibrary->SetName(PathUtil::RemoveExtension(pAsset->GetFile(0)));
	return pLibrary->Save();
}

