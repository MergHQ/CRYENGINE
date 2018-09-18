// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "IEditor.h"
#include "PrefabObject.h"
#include "PrefabPicker.h"
#include "Prefabs/PrefabManager.h"

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
	CUndoSwapPrefab(CPrefabObject* obj, SPreviouslySetPrefabData& previousPrefab, const char* undoDescription);

protected:
	virtual const char* GetDescription() override { return m_undoDescription; }
	virtual const char* GetObjectName() override;

	virtual void        Undo(bool bUndo) override;
	virtual void        Redo() override;

private:

	CryGUID                  m_guid;
	string                   m_undoDescription;
	SPreviouslySetPrefabData m_undoPreviousPrefabAsset;
	SPreviouslySetPrefabData m_redoPreviousPrefabAsset;

};

CUndoSwapPrefab::CUndoSwapPrefab(CPrefabObject* obj, SPreviouslySetPrefabData& previousPrefabData, const char* undoDescription) :
	m_undoDescription(undoDescription),
	m_guid(obj->GetId()),
	m_undoPreviousPrefabAsset(previousPrefabData)
{
	assert(obj != nullptr);
}

const char* CUndoSwapPrefab::GetObjectName()
{
	CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!object)
		return "";

	return object->GetName();
}

void CUndoSwapPrefab::Undo(bool bUndo)
{
	CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!object)
		return;

	CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(object);

	if (bUndo)
	{
		m_redoPreviousPrefabAsset.assetFilename = pPrefabObject->GetAssetPath();
		m_redoPreviousPrefabAsset.changed = true;
	}

	if (m_undoPreviousPrefabAsset.changed)
	{
		CPrefabPicker::SetPrefabFromAssetFilename(pPrefabObject, m_undoPreviousPrefabAsset.assetFilename);
		pPrefabObject->UpdatePrefab(eOCOT_Modify);
		pPrefabObject->GetPrefabItem()->UpdateObjects();
	}
}

void CUndoSwapPrefab::Redo()
{
	CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!object)
		return;

	CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(object);
	if (m_redoPreviousPrefabAsset.changed)
	{
		CPrefabPicker::SetPrefabFromAssetFilename(pPrefabObject, m_redoPreviousPrefabAsset.assetFilename);
		pPrefabObject->UpdatePrefab(eOCOT_Modify);
		pPrefabObject->GetPrefabItem()->UpdateObjects();
	}
}

void CPrefabPicker::SwapPrefab(CPrefabObject* pPrefabObject)
{
	SPreviouslySetPrefabData previousPrefabData;

	CAssetManager* const pManager = CAssetManager::GetInstance();
	CAsset* pAsset = pManager->FindAssetForFile(pPrefabObject->GetAssetPath());

	if (!pAsset)
	{
		return;
	}

	const char* pType = pAsset->GetType()->GetTypeName();

	//This is only selection, not full update. The whole prefab chain update and undo/redo are applied only when the asset browser dialog Execute() is completed
	dll_string assetFilename = SStaticAssetSelectorEntry::SelectFromAsset([pPrefabObject, &previousPrefabData](const char* newValue)
	{
		//We store the first time we change the asset for preview since it's the one we want to restore
		if (!previousPrefabData.changed)
		{
		  previousPrefabData.assetFilename = pPrefabObject->GetAssetPath();
		}

		if (SetPrefabFromAssetFilename(pPrefabObject, newValue))
		{
		  previousPrefabData.changed = true;
		}

		GetIEditor()->GetObjectManager()->AddObjectToSelection(pPrefabObject);
	},
	{ pType },
	pPrefabObject->GetAssetPath());

	//Start the Asset Browser dialog
	if (!assetFilename.empty())
	{
		CAssetManager* const pManager = CAssetManager::GetInstance();
		CAsset* pPreviousAsset = pManager->FindAssetForFile(assetFilename.c_str());
		//If the set is not valid we don't want to register an undo
		if (IsValidAssetForPrefab(pPrefabObject, *pPreviousAsset))
		{
			SetPrefabFromAssetFilename(pPrefabObject, assetFilename.c_str());

			//This updates the CPrefabItem with the new prefab guids
			pPrefabObject->UpdatePrefab(eOCOT_Modify);
			//This updates the visuals of all the CPrefabObjects deriving from the CPrefabItem
			pPrefabObject->GetPrefabItem()->UpdateObjects();

			if (!CUndo::IsRecording())
			{
				CUndo undo("Prefab Swap");
				CUndo::Record(new CUndoSwapPrefab(pPrefabObject, previousPrefabData, "Prefab Swap"));
			}
		}
		GetIEditor()->GetObjectManager()->AddObjectToSelection(pPrefabObject);
	}
	else if (assetFilename.empty() && previousPrefabData.changed)
	{
		if (SetPrefabFromAssetFilename(pPrefabObject, previousPrefabData.assetFilename))
		{
			previousPrefabData.changed = false;
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
