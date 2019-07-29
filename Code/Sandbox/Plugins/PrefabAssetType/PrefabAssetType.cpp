// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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

class CUndoItemChange : public IUndoObject
{
public:
	CUndoItemChange(CPrefabItem* pItem)
	{
		CRY_ASSERT(pItem);
		m_itemId = pItem->GetGUID();
		//Get the current state of the item
		m_undoSerializedItem = GetSerializedItemNode(pItem);
	}

	//! Return description of this Undo object.
	virtual const char* GetDescription()
	{
		return "Undo Prefab Item Change";
	}

	//! Undo this object.
	//! @param bUndo If true this operation called in response to Undo operation.
	virtual void Undo(bool bUndo = true)
	{
		//Make sure this item is still around (might be null if we delete the related prefab asset)
		CPrefabItem* pItem = static_cast<CPrefabItem*>(GetIEditor()->GetPrefabManager()->FindItem(m_itemId));
		CRY_ASSERT(pItem);
		if (pItem)
		{
			//Record data for the redo operation
			if (bUndo)
			{
				m_redoSerializedItem = GetSerializedItemNode(pItem);
			}

			//Serialize undo archive into the item
			SerializeNodeIntoItem(pItem, m_undoSerializedItem);

			//Regenerate all prefab instances
			pItem->UpdateObjects();
		}
	}

	//! Redo undone changes on object.
	virtual void Redo()
	{
		//Make sure this item is still around (might be null if we delete the related prefab asset)
		CPrefabItem* pItem = static_cast<CPrefabItem*>(GetIEditor()->GetPrefabManager()->FindItem(m_itemId));
		CRY_ASSERT(pItem);
		if (pItem)
		{
			//Serialize redo archive into the item
			SerializeNodeIntoItem(pItem, m_redoSerializedItem);

			//Regenerate all prefab instances
			pItem->UpdateObjects();
		}
	}

	//!Serializes an xml hierarchy into a prefab item
	void SerializeNodeIntoItem(CPrefabItem* pItem, XmlNodeRef node)
	{
		IDataBaseItem::SerializeContext serializationContext{ node, true };
		pItem->Serialize(serializationContext);
	}

	//!Saves a prefab item xml info into a new xml hierarchy
	XmlNodeRef GetSerializedItemNode(CPrefabItem* pItem)
	{
		//Create a new xml node
		XmlNodeRef node = XmlHelpers::CreateXmlNode("Root");
		//Serialize the item into it
		IDataBaseItem::SerializeContext serializationContext{ node, false };
		pItem->Serialize(serializationContext);
		//Serialize will just assign m_objectsNode to root as a child, we want an entirely new node hierarchy as m_objectsNode will be reset when we call CPrefabItem::ResetItem
		return node->clone();
	}

	// Returns the name of undo object
	virtual const char* GetObjectName()
	{
		return "Undo Prefab Item Override";
	}

private:
	//! The item being changed
	CryGUID    m_itemId;
	//Undo and redo serialized archives
	XmlNodeRef m_undoSerializedItem = nullptr;
	XmlNodeRef m_redoSerializedItem = nullptr;
};

//! If the item related to this data file already exists
bool HasPrefabItem(const string& dataFilePath)
{
	const string libraryName = PathUtil::RemoveExtension(dataFilePath);
	IDataBaseLibrary* pLibrary = GetIEditor()->GetPrefabManager()->FindLibrary(libraryName);

	//Prefab items are always at index 0 of a library
	//make sure that we have a valid library and a valid item
	if (pLibrary && pLibrary->GetItemCount() && pLibrary->GetItem(0))
	{
		return true;
	}

	return false;
}

//Create a new prefab item from an asset
CPrefabItem* const CreatePrefabItem(INewAsset& prefabAsset)
{
	//Find the data file path that we will use to create this item and it's library
	const string dataFilePath = PathUtil::RemoveExtension(prefabAsset.GetMetadataFile());

	CPrefabManager* const pPrefabManager = GetIEditor()->GetPrefabManager();

	//check if we already have a prefab with this data file  path, if yes we will need to overwrite it
	bool prefabItemAlreadyExists = HasPrefabItem(dataFilePath);

	//Create the item (or return an existing one)
	CPrefabItem* const pPrefabItem = (CPrefabItem*)pPrefabManager->CreateItem(dataFilePath);
	if (!pPrefabItem)
	{
		return nullptr;
	}

	//If the item was already present the we need to reset it and serialize the new prefab inside it
	if (prefabItemAlreadyExists)
	{
		if (CUndo::IsRecording())
		{
			CUndo::Record(new CUndoItemChange(pPrefabItem));
		}

		//Find all the instances of this prefab item
		CBaseObjectsArray instances = GetIEditor()->GetPrefabManager()->FindAllInstancesOfItem(pPrefabItem);

		//Undo is suspended here because undoing deletion will cause object to reappear after undo, and we are handling instance regeneration after undo
		//by resetting the prefab item with SetPrefab in each prefab object. It's done this way because it is safer and has less side effects
		CScopedSuspendUndo undoSuspend{};

		//Just delete all the children of one instance, the prefab update system will propagate the deletion to all the other instances
		//We are leaving all the prefab objects alive so that they will be repopulated when we start adding objects into the new prefab
		std::vector<CBaseObject*> children;
		instances[0]->GetAllChildren(children);
		GetIEditor()->GetObjectManager()->DeleteObjects(children);


		//finally reset the item xml hierarchy
		pPrefabItem->ResetObjectsNode();
	}

	pPrefabItem->SetName(prefabAsset.GetName());
	pPrefabItem->Update();
	pPrefabItem->GetLibrary()->Save();
	prefabAsset.SetGUID(pPrefabItem->GetGUID());
	prefabAsset.AddFile(pPrefabItem->GetLibrary()->GetFilename());

	return pPrefabItem;
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
	//First find out if we have creation params
	const SPrefabCreateParams* pPrefabCreateParams = pCreateParams ? static_cast<const SPrefabCreateParams*>(pCreateParams) : nullptr;

	//Then check out if we need to also create a Prefab object from a selection
	//Note that this is mainly for legacy support, an asset type should only create assets and item, then the calling code can instantiate a prefab object
	const ISelectionGroup* pSelectionToUse = nullptr;
	if (pPrefabCreateParams)
	{
		//if we have creation params try to use this selection
		pSelectionToUse = pPrefabCreateParams->pNewMembersSelection;
	}
	else
	{
		//if not use the global selection
		pSelectionToUse = GetIEditor()->GetISelectionGroup();
	}

	CUndo undo("Create Prefab Item");

	//Create the item
	CPrefabItem* pNewItem = Private_PrefabAssetType::CreatePrefabItem(asset);

	//Create the prefab object
	//If item has been created and we have a valid selection proceed to create a new prefab object with the selection and serialize it to prefab
	if (pNewItem && pSelectionToUse && pSelectionToUse->GetCount())
	{
		CSelectionGroup selectionGroup;
		for (size_t i = 0, n = pSelectionToUse->GetCount(); i < n; ++i)
		{
			selectionGroup.AddObject(pSelectionToUse->GetObject(i));
		}

		pNewItem->MakeFromSelection(selectionGroup);
	}

	return pNewItem != nullptr;
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