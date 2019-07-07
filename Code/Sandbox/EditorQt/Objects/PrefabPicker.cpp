// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "IEditor.h"
#include "PrefabObject.h"
#include "PrefabPicker.h"
#include "Prefabs/PrefabManager.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetResourceSelector.h>
#include <AssetSystem/Browser/AssetBrowserDialog.h>

struct SPreviouslySetPrefabData
{
	//! This is stored by filename instead of pointer because it
	//! could have been deleted in the meantime (e.g you delete it in the asset browser dialog before selecting a new one and then undo)
	string assetFilename = "";
	bool   changed = false;
};

class CUndoSwapPrefab : public IUndoObject
{
public:
	CUndoSwapPrefab(const std::vector<CPrefabObject*>& objects, const std::vector<SPreviouslySetPrefabData> & previousPrefab, const char* undoDescription, const string& swappingWithAsset);

protected:
	virtual const char* GetDescription() override { return m_undoDescription; }
	virtual const char* GetObjectName() override;

	virtual void        Undo(bool bUndo) override;
	virtual void        Redo() override;

private:

	std::vector<CryGUID>     m_guids;
	string                   m_undoDescription;
	string                   m_swappingWithAsset;
	std::vector<SPreviouslySetPrefabData> m_undoPreviousPrefabAssets;
	std::vector<SPreviouslySetPrefabData> m_redoPreviousPrefabAssets;

};

CUndoSwapPrefab::CUndoSwapPrefab(const std::vector<CPrefabObject*>& objects, const std::vector<SPreviouslySetPrefabData> & previousPrefab, const char* undoDescription, const string& swappingWithAsset) :
	m_undoPreviousPrefabAssets(objects.size()),
	m_redoPreviousPrefabAssets(objects.size()),
	m_undoDescription(undoDescription),
	m_swappingWithAsset(swappingWithAsset)
{
	CRY_ASSERT(!objects.empty());
	//store all prefab objects guids and change data
	for (CPrefabObject* pObj : objects)
	{
		m_guids.push_back(pObj->GetId());
	}

	for (int i = 0; i < previousPrefab.size(); i++)
	{
		m_undoPreviousPrefabAssets[i] = previousPrefab[i];
	}
}

const char* CUndoSwapPrefab::GetObjectName()
{
	string result;
	result.Format(" with prefab asset \"%s\"", m_swappingWithAsset.c_str());
	return result;
}

void CUndoSwapPrefab::Undo(bool bUndo)
{
	for (int i = 0; i < m_guids.size(); i++)
	{
		CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guids[i]);
		if (!object)
			break;

		CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(object);

		if (bUndo)
		{
			m_redoPreviousPrefabAssets[i].assetFilename = pPrefabObject->GetAssetPath();
			m_redoPreviousPrefabAssets[i].changed = true;
		}

		if (m_undoPreviousPrefabAssets[i].changed)
		{
			CPrefabPicker::SetPrefabFromAssetFilename(pPrefabObject, m_undoPreviousPrefabAssets[i].assetFilename);
			pPrefabObject->UpdatePrefab(eOCOT_Modify);
		}
	}
}

void CUndoSwapPrefab::Redo()
{
	for (int i = 0; i < m_guids.size(); i++)
	{
		CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guids[i]);
		if (!object)
			break;

		CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(object);
		if (m_redoPreviousPrefabAssets[i].changed)
		{
			CPrefabPicker::SetPrefabFromAssetFilename(pPrefabObject, m_redoPreviousPrefabAssets[i].assetFilename);
			pPrefabObject->UpdatePrefab(eOCOT_Modify);
		}
	}
}

void CPrefabPicker::SwapPrefab(const std::vector<CPrefabObject*>& prefabObjects)
{
	if (prefabObjects.empty())
	{
		return;
	}

	//We should never allow swap of nested selections (aka selections where some of the objects are parents of other objects)
	for (CPrefabObject* pPrefabObject : prefabObjects)
	{
		if (CPrefabPicker::ArePrefabsInParentHierarchy(pPrefabObject, prefabObjects))
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Cannot Swap this selection as some of the objects in the selection are parents of %s", pPrefabObject->GetName().c_str());
			return;
		}
	}

	CAssetManager* const pManager = CAssetManager::GetInstance();
	std::vector<SPreviouslySetPrefabData> previousPrefabsData(prefabObjects.size());

	CAsset* pLastSelectedPrefabAsset = pManager->FindAssetForFile(prefabObjects[prefabObjects.size() - 1]->GetAssetPath());
	if (!pLastSelectedPrefabAsset)
	{
		return;
	}

	CAssetSelector selector({ pLastSelectedPrefabAsset->GetType()->GetTypeName() });

	//Spawn the selection dialog and if the newly selected asset can be applied to all prefabs objects in the selection proceed with the change
	//This is only selection, not full update. The whole prefab instances are updated and undo/redo is applied only when the asset browser dialog Execute() is completed
	bool applyChanges = selector.Execute([&previousPrefabsData, &prefabObjects](const char* newValue)
	{
		CAssetManager* const pManager = CAssetManager::GetInstance();
		CAsset* pNewAsset = pManager->FindAssetForFile(newValue);

		//if this asset is not valid for all prefab objects just stop here
		bool isValidForAllPrefabs = true;
		for (int i = 0; i < prefabObjects.size(); i++)
		{
			if (!IsValidAssetForPrefab(prefabObjects[i], *pNewAsset))
			{
				isValidForAllPrefabs = false;
				break;
			}
		}

		if (!isValidForAllPrefabs)
		{
			return;
		}

		//apply to all prefab objects in the selection
		//note that we are note sending a prefab modify event, we apply to all instances only if the change is confirmed
		for (int i = 0; i < prefabObjects.size(); i++)
		{
			//We store the first time we change the asset for preview since it's the one we want to restore if the change operation is canceled
			if (!previousPrefabsData[i].changed)
			{
				previousPrefabsData[i].assetFilename = prefabObjects[i]->GetAssetPath();
				previousPrefabsData[i].changed = true;
			}
			else if(previousPrefabsData[i].assetFilename == newValue) // we are going back to the original asset, no change required after this
			{
				previousPrefabsData[i].changed = false;
			}
			//actually set the new asset
			SetPrefabFromAssetFilename(prefabObjects[i], newValue);
		}
	},
	{pLastSelectedPrefabAsset->GetType()->GetTypeName()},
	prefabObjects[prefabObjects.size() - 1]->GetAssetPath());

	std::vector<CPrefabObject*> objectsToUndo;
	std::vector<SPreviouslySetPrefabData> objectsToUndoPreviousData;

	//We only need to update a specific prefab item only once, the modify will propagate the changes to every instance
	//Create a map so that we can track which object has been modified in which item and avoid repeated changes
	std::unordered_map<CPrefabItem*, std::vector<CryGUID>> updatedObjectItem;
	
	for (int i = 0; i < prefabObjects.size(); i++)
	{
		//if we have accepted the operation apply to all instances and register an undo
		if (applyChanges && previousPrefabsData[i].changed)
		{
			//we should only undo the objects we have actually changed and let modify update the rest
			bool shouldUndo = false;

			//we need the parent of the prefab we are updating so that we can track its changes
			CPrefabObject* pParentPrefab = static_cast<CPrefabObject*>(prefabObjects[i]->GetPrefab());
			CPrefabItem* pParentItem = pParentPrefab ? static_cast<CPrefabItem*>(pParentPrefab->GetPrefabItem()) : nullptr;
			
			//this prefab has a parent to update, we can use the map to avoid multiple useless modify prefab calls
			if (pParentItem)
			{
				//find if we have already modified something in pParentItem
				std::unordered_map<CPrefabItem*, std::vector<CryGUID>>::iterator it = updatedObjectItem.find(pParentItem);
				
				//if we need to call a modify on this prefab item 
				bool shouldUpdate = false;
				
				//we don't know this item, definitely needs modify
				if (it == updatedObjectItem.end())
				{
					shouldUpdate = true;
				}
				else if(std::find(it->second.begin(), it->second.end(), prefabObjects[i]->GetIdInPrefab()) == it->second.end()) //we know this item and might know this object guid
				{
					shouldUpdate = true;
				}

				//no changes applied to this item yet, needs a call to modify
				if (shouldUpdate)
				{
					//This updates the CPrefabItem with the new prefab guids
					prefabObjects[i]->UpdatePrefab(eOCOT_Modify);
					updatedObjectItem[pParentItem].push_back(prefabObjects[i]->GetIdInPrefab());

					//we only undo the prefab items we actually called modify on
					shouldUndo = true;
				}
			}
			else
			{
				//This updates the CPrefabItem with the new prefab guids
				prefabObjects[i]->UpdatePrefab(eOCOT_Modify);
				shouldUndo = true;
			}

			if (shouldUndo)
			{
				//add this objects to the undo list
				objectsToUndo.push_back(prefabObjects[i]);
				objectsToUndoPreviousData.push_back(previousPrefabsData[i]);
			}

			GetIEditor()->GetObjectManager()->AddObjectToSelection(prefabObjects[i]);
		} 		//if we have not accepted the operation revert to original asset if necessary
		else if (!applyChanges && previousPrefabsData[i].changed)
		{
			SetPrefabFromAssetFilename(prefabObjects[i], previousPrefabsData[i].assetFilename);
			GetIEditor()->GetObjectManager()->AddObjectToSelection(prefabObjects[i]);
		}
	}

	if (objectsToUndo.size())
	{
		if (!CUndo::IsRecording())
		{
			//we do not undo the whole objects list, we only need one set per prefab item parent, the modify calls in Swap Undo will do the rest
			CUndo undo("Prefab Swap");
			CUndo::Record(new CUndoSwapPrefab(objectsToUndo, objectsToUndoPreviousData, "Prefab Swap", objectsToUndo[0]->GetPrefabItem()->GetName()));
		}
	}
}

bool CPrefabPicker::SetPrefabFromAsset(CPrefabObject* pPrefabObject, const CAsset* pNewPrefabAsset)
{
	if (pNewPrefabAsset && IsValidAssetForPrefab(pPrefabObject, *pNewPrefabAsset))
	{
		IDataBaseItem* pItem = GetIEditor()->GetPrefabManager()->LoadItem(pNewPrefabAsset->GetGUID());
		
		//This can happen if the asset is in the asset browser but the prefab object has been deleted
		//(for example undo after creation)
		if (!pItem)
		{
			return false;
		}

		pPrefabObject->SetPrefab(static_cast<CPrefabItem*>(pItem), true);
		GetIEditor()->GetObjectManager()->NotifyPrefabObjectChanged(pPrefabObject);
		GetIEditor()->GetObjectManager()->NotifyObjectListeners(pPrefabObject, OBJECT_ON_UI_PROPERTY_CHANGED);
		return true;
	}
	return false;
}

bool CPrefabPicker::SetPrefabFromAssetFilename(CPrefabObject* pPrefabObject, const string& assetFilename)
{
	CAssetManager* const pManager = CAssetManager::GetInstance();
	CAsset* pPreviousAsset = pManager->FindAssetForFile(assetFilename);
	//This asset could have been deleted, check to see if it's still alive
	if (pPreviousAsset)
	{
		return SetPrefabFromAsset(pPrefabObject, pPreviousAsset);
	}

	return false;
}

bool CPrefabPicker::IsValidAssetForPrefab(const CPrefabObject* pPrefabObject, const CAsset& pNewPrefabAsset)
{
	string filename = pPrefabObject->GetAssetPath();
	CAssetManager* const pManager = CAssetManager::GetInstance();
	const CAsset* pAsset = pManager->FindAssetForFile(filename);

	if (!pAsset || pNewPrefabAsset.GetType()->GetTypeName() != pAsset->GetType()->GetTypeName())
	{
		return false;
	}

	//Start from the current prefab and add all the parent's guids to the blacklist 
	std::set<CryGUID> allGuids;
	const CBaseObject * pParent = pPrefabObject->GetPrefab();
	while (pParent)
	{
		allGuids.insert((static_cast<const CPrefabObject*>(pParent))->GetPrefabGuid());
		pParent = pParent->GetPrefab();
	}

	//Now load the new prefab item and find all the GUIDs
	IDataBaseItem* pItem = GetIEditor()->GetPrefabManager()->LoadItem(pNewPrefabAsset.GetGUID());
	//This can happen if the asset is in the asset browser but the prefab object has been deleted
	//(for example undo after creation)
	if (!pItem)
	{
		return false;
	}

	CPrefabItem * pPrefabItem = static_cast<CPrefabItem*>(pItem);

	//Now make sure that no parent guid in the hierarchy is referenced in the new prefab we are swapping in 
	std::set<CryGUID> newPrefabsChildGUIDs = pPrefabItem->FindAllPrefabsGUIDsInChildren({});
	//The top level prefab also needs to be check for repetition
	newPrefabsChildGUIDs.insert(pPrefabItem->GetGUID());
	for (const CryGUID & guid : allGuids)
	{
		if (std::find(newPrefabsChildGUIDs.begin(), newPrefabsChildGUIDs.end(), guid) != newPrefabsChildGUIDs.end())
		{
			return false;
		}
	}

	return true;
}

void CPrefabPicker::GetAllPrefabObjectDescendants(const CBaseObject* pObject, std::vector<CPrefabObject*>& outAllChildren)
{
	for (int i = 0, iChildCount(pObject->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pObject->GetChild(i);
		if (pChild == NULL)
			continue;
		if (pChild->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			outAllChildren.push_back(static_cast<CPrefabObject*>(pChild));
		}
		GetAllPrefabObjectDescendants(pChild, outAllChildren);
	}
}

bool CPrefabPicker::ArePrefabsInParentHierarchy(const CPrefabObject* pPrefab, const std::vector<CPrefabObject*>& possibleParents)
{
	CBaseObject* pParent = pPrefab->GetPrefab();
	while (pParent)
	{
		if (std::find(possibleParents.begin(), possibleParents.end(), pParent) != possibleParents.end())
		{
			return true;
		}

		pParent = pParent->GetPrefab();
	}

	return false;
}
