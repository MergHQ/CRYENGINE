// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
	CUndoSwapPrefab(const std::vector<CPrefabObject*>& objects, const std::vector<SPreviouslySetPrefabData> & previousPrefab, const char* undoDescription);

protected:
	virtual const char* GetDescription() override { return m_undoDescription; }
	virtual const char* GetObjectName() override;

	virtual void        Undo(bool bUndo) override;
	virtual void        Redo() override;

private:

	std::vector<CryGUID>     m_guids;
	string                   m_undoDescription;
	std::vector<SPreviouslySetPrefabData> m_undoPreviousPrefabAssets;
	std::vector<SPreviouslySetPrefabData> m_redoPreviousPrefabAssets;

};

CUndoSwapPrefab::CUndoSwapPrefab(const std::vector<CPrefabObject*>& objects, const std::vector<SPreviouslySetPrefabData>& previousPrefabsData, const char* undoDescription) :
	m_undoPreviousPrefabAssets(objects.size()),
	m_redoPreviousPrefabAssets(objects.size()),
	m_undoDescription(undoDescription)
{
	CRY_ASSERT(!objects.empty());
	//store all prefab objects guids and change data
	for (CPrefabObject* pObj : objects)
	{
		m_guids.push_back(pObj->GetId());
	}

	for (int i = 0; i < previousPrefabsData.size(); i++)
	{
		m_undoPreviousPrefabAssets[i] = previousPrefabsData[i];
	}
}

const char* CUndoSwapPrefab::GetObjectName()
{
	string result = "";
	int guidsSize = m_guids.size();

	if (guidsSize == 1) //If we only have one prefab object we can directly show the name
	{
		CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guids[0]);
		if (pObject)
		{
			result = pObject->GetName();
		}
	}
	else if (guidsSize > 1) //if we have more than one prefab object we show how many we have
	{
		result.Format("%d prefabs", guidsSize);
	}

	return result.c_str();
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

	for (int i = 0; i < prefabObjects.size(); i++)
	{
		//if we have accepted the operation apply to all instances and register an undo
		if (applyChanges && previousPrefabsData[i].changed)
		{
			//This updates the CPrefabItem with the new prefab guids
			prefabObjects[i]->UpdatePrefab(eOCOT_Modify);

			if (!CUndo::IsRecording())
			{
				CUndo undo("Prefab Swap");
				CUndo::Record(new CUndoSwapPrefab(prefabObjects, previousPrefabsData, "Prefab Swap"));
			}
			GetIEditor()->GetObjectManager()->AddObjectToSelection(prefabObjects[i]);
		} 		//if we have not accepted the operation revert to original asset if necessary
		else if (!applyChanges && previousPrefabsData[i].changed)
		{
			SetPrefabFromAssetFilename(prefabObjects[i], previousPrefabsData[i].assetFilename);
			GetIEditor()->GetObjectManager()->AddObjectToSelection(prefabObjects[i]);
		}
	}
}

bool CPrefabPicker::SetPrefabFromAsset(CPrefabObject* pPrefabObject, const CAsset* pNewPrefabAsset)
{
	string filename = pPrefabObject->GetAssetPath();
	CAssetManager* const pManager = CAssetManager::GetInstance();
	const CAsset* pAsset = pManager->FindAssetForFile(filename);

	CBaseObject* pParent = pPrefabObject->GetParent();

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
