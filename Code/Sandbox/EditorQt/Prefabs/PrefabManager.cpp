// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PrefabManager.h"

#include "GameEngine.h"
#include "LogFile.h"
#include "Objects/PrefabObject.h"
#include "Objects/SelectionGroup.h"
#include "PrefabEvents.h"
#include "PrefabItem.h"
#include "PrefabLibrary.h"

#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Browser/AssetBrowserDialog.h>
#include <Controls/QuestionDialog.h>
#include <FileUtils.h>
#include <PathUtils.h>
#include <Util/FileUtil.h>

namespace Private_PrefabManager
{

void ImportPrefabLibrary(XmlNodeRef& library, const char* szPath)
{
	CAssetManager* const pAssetManager = GetIEditor()->GetAssetManager();

	std::vector<CAsset*> assets;
	for (int i = 0; i < library->getChildCount(); i++)
	{
		XmlNodeRef itemNode = library->getChild(i);
		if (stricmp(itemNode->getTag(), "Prefab") != 0)
		{
			continue;
		}

		string name;
		CryGUID guid;
		if (!itemNode->getAttr("Name", name) || !itemNode->getAttr("Id", guid))
		{
			continue;
		}

		if (pAssetManager->FindAssetById(guid))
		{
			continue;
		}

		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, "Creating prefab asset %s in %s", name.c_str(), szPath);

		// Create a sub folder for each part of the item name separated by a period.
		name.replace('.', '/');
		const string dataFilePath(PathUtil::Make(szPath, name, "prefab"));

		GetISystem()->GetIPak()->MakeDir(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), PathUtil::GetPathWithoutFilename(dataFilePath)));

		itemNode->setAttr("Name", PathUtil::GetFileName(name).c_str());
		XmlHelpers::SaveXmlNode(itemNode, dataFilePath.c_str());

		CAsset* pAsset = new CAsset("Prefab", guid, PathUtil::GetFileName(dataFilePath));
		CAssetImportContext ctx;
		CEditableAsset asset = ctx.CreateEditableAsset(*pAsset);
		asset.SetMetadataFile(string().Format("%s.cryasset", dataFilePath.c_str()));
		asset.AddFile(dataFilePath);
		asset.WriteToFile();
		assets.push_back(pAsset);
	}

	if (!assets.empty())
	{
		pAssetManager->MergeAssets(assets);
	}
}

class CObjectsRefOwner
{
public:
	CObjectsRefOwner(const std::vector<CBaseObject*>& objects)
		: m_objects(objects)
	{
		for (auto pObject : m_objects)
		{
			pObject->AddRef();
		}
	}

	void Release()
	{
		if (m_isReleased)
		{
			return;
		}

		m_isReleased = true;
		for (auto pObject : m_objects)
		{
			pObject->Release();
		}
	}

	~CObjectsRefOwner()
	{
		Release();
	}

private:
	const std::vector<CBaseObject*>& m_objects;
	bool                             m_isReleased = { false };
};

void CollectAffectedPrefabsFor(const std::vector<CBaseObject*>& objects, std::unordered_map<CPrefabObject*, std::unordered_set<CBaseObject*>>& outResult)
{
	for (CBaseObject* pObject : objects)
	{
		// Get the current object's prefab
		CPrefabObject* pPrefab = (CPrefabObject*)pObject->GetPrefab();
		if (pPrefab)
		{
			// Add this object to the set of affected objects
			std::unordered_set<CBaseObject*>& objects = outResult[pPrefab];
			objects.insert(pObject);
		}
	}
}

void CloneObjects(const std::unordered_map<CPrefabObject*, std::unordered_set<CBaseObject*>>& objectsPerPrefabContainer)
{
	IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
	CUndo undo("Clone objects from prefab(s)");

	std::vector<CBaseObject*> objectsToSelect;
	for (auto objectsPerPrefab : objectsPerPrefabContainer)
	{
		CPrefabObject* pPrefab = objectsPerPrefab.first;
		std::unordered_set<CBaseObject*>& objects = objectsPerPrefab.second;

		std::vector<CBaseObject*> newChildren;
		pPrefab->ExtractChildrenClones(objects, newChildren);

		objectsToSelect.insert(objectsToSelect.cend(), newChildren.begin(), newChildren.end());
	}

	pObjectManager->SelectObjects(objectsToSelect);
}

void GetSelectedPrefabs(std::vector<CPrefabObject*>& outResult)
{
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();

	if (!pSelection)
		return;

	for (int i = 0, count = pSelection->GetCount(); i < count; i++)
	{
		if (pSelection->GetObject(i) && pSelection->GetObject(i)->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			CPrefabObject* pPrefab = static_cast<CPrefabObject*>(pSelection->GetObject(i));
			outResult.push_back(pPrefab);
		}
	}
}

}
//////////////////////////////////////////////////////////////////////////
// CUndoGroupObjectOpenClose implementation.
//////////////////////////////////////////////////////////////////////////
class CUndoGroupObjectOpenClose : public IUndoObject
{
public:
	CUndoGroupObjectOpenClose(CPrefabObject* prefabObj)
	{
		m_prefabObject = prefabObj;
		m_bOpenForUndo = m_prefabObject->IsOpen();
	}
protected:
	virtual const char* GetDescription() { return "Prefab's Open/Close"; }

	virtual void        Undo(bool bUndo)
	{
		m_bRedo = bUndo;
		Apply(m_bOpenForUndo);
	}
	virtual void Redo()
	{
		if (m_bRedo)
			Apply(!m_bOpenForUndo);
	}

	void Apply(bool bOpen)
	{
		if (m_prefabObject)
		{
			if (bOpen)
				m_prefabObject->Open();
			else
				m_prefabObject->Close();
		}
	}

private:

	CPrefabObjectPtr m_prefabObject;
	bool             m_bRedo;
	bool             m_bOpenForUndo;
};

//////////////////////////////////////////////////////////////////////////
// CPrefabManager implementation.
//////////////////////////////////////////////////////////////////////////

CPrefabManager::CPrefabManager()
	: m_pPrefabEvents(nullptr)
{
	m_bUniqNameMap = true;

	m_pPrefabEvents = new CPrefabEvents();

	m_skipPrefabUpdate = false;
}

CPrefabManager::~CPrefabManager()
{
	SAFE_DELETE(m_pPrefabEvents);
}

void CPrefabManager::ClearAll()
{
	CBaseLibraryManager::ClearAll();

	m_pPrefabEvents->RemoveAllEventData();
}

IDataBaseItem* CPrefabManager::CreateItem(const string& filename)
{
	const string libraryName = PathUtil::RemoveExtension(filename);

	IDataBaseLibrary* const pLibrary = AddLibrary(libraryName);
	if (!pLibrary)
	{
		return nullptr;
	}

	return CreateItem(pLibrary);
}

IDataBaseItem* CPrefabManager::CreateItem(IDataBaseLibrary* pLibrary)
{
	if (!pLibrary)
	{
		return nullptr;
	}

	IDataBaseItem* pItem = nullptr;
	//Prefabs libraries are supposed to only have one item, if it already exist just return it
	if (pLibrary->GetItemCount())
	{
		pItem = pLibrary->GetItem(0);
	}
	else //if the library is empty create the item
	{
		pItem = CBaseLibraryManager::CreateItem(pLibrary);
	}

	return pItem;
}

CBaseLibraryItem* CPrefabManager::MakeNewItem()
{
	return new CPrefabItem;
}

CBaseLibrary* CPrefabManager::MakeNewLibrary()
{
	return new CPrefabLibrary(this);
}

string CPrefabManager::GetRootNodeName()
{
	return "PrefabsLibrary";
}

string CPrefabManager::GetLibsPath()
{
	return {};
}

void CPrefabManager::Serialize(XmlNodeRef& node, bool bLoading)
{
	CRY_PROFILE_FUNCTION(PROFILE_LOADING_ONLY);
	if (bLoading)
	{
		CBaseLibraryManager::Serialize(node, bLoading);
	}
	else
	{
		// TODO: remove when DEV - 5324 is done, since prefabs are assets.
		SaveAllLibs();
	}
}

CPrefabItem* CPrefabManager::MakeFromSelection(const CSelectionGroup* pSelectionGroup)
{
	const CAssetType* const pAssetType = GetIEditor()->GetAssetManager()->FindAssetType("Prefab");
	if (!pAssetType)
	{
		return nullptr;
	}

	const string assetBasePath = CAssetBrowserDialog::SaveSingleAssetForType(pAssetType->GetTypeName());
	if (assetBasePath.empty())
	{
		return nullptr; // Operation cancelled by user.
	}

	const string newAssetPath = string().Format("%s.%s.cryasset", assetBasePath.c_str(), pAssetType->GetFileExtension());

	SPrefabCreateParams createParam(pSelectionGroup);

	pAssetType->Create(newAssetPath, &createParam);
	CAsset* const pAsset = GetIEditor()->GetAssetManager()->FindAssetForMetadata(newAssetPath);
	if (!pAsset)
	{
		return nullptr;
	}
	return static_cast<CPrefabItem*>(GetIEditor()->GetPrefabManager()->LoadItem(pAsset->GetGUID()));
}

void CPrefabManager::AddSelectionToPrefab(const CSelectionGroup& selection)
{
	//Get all the prefabs in the selection
	std::vector<CBaseObject*> prefabsInSelection = selection.GetObjectsByFilter([](CBaseObject* pObject)
	{
		return static_cast<bool>(pObject->IsKindOf(RUNTIME_CLASS(CPrefabObject)));
	});

	Private_PrefabManager::CObjectsRefOwner objectOwner(prefabsInSelection);

	//Make sure we have one prefab and log errors if we don't
	if (!prefabsInSelection.size())
	{
		Warning("Select a prefab and objects");
		return;
	}
	else if (prefabsInSelection.size() > 1)
	{
		Warning("Select only one prefab");
		return;
	}

	AddSelectionToPrefab(static_cast<CPrefabObject*>(prefabsInSelection[0]), selection);
}

void CPrefabManager::OpenSelected()
{
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	if (!pSelection)
		return;

	const int count = pSelection->GetCount();
	if (!count)
		return;

	std::vector<CPrefabObject*> selectedPrefabs(count);

	for (int i = 0; i < count; ++i)
		selectedPrefabs[i] = static_cast<CPrefabObject*>(pSelection->GetObject(i));

	CUndo undo("Open selected prefabs");
	OpenPrefabs(selectedPrefabs);
}

void CPrefabManager::CloseSelected()
{
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	if (!pSelection)
		return;

	const int count = pSelection->GetCount();
	if (!count)
		return;

	std::vector<CPrefabObject*> selectedPrefabs(count);

	for (int i = 0; i < count; ++i)
		selectedPrefabs[i] = static_cast<CPrefabObject*>(pSelection->GetObject(i));

	CUndo undo("Close selected prefabs");
	ClosePrefabs(selectedPrefabs);
}

void CPrefabManager::AddSelectionToPrefab(CPrefabObject* pPrefab, const CSelectionGroup& selection)
{
	
	std::vector<CBaseObject*> objects;
	selection.GetObjects(objects);

	Private_PrefabManager::CObjectsRefOwner objectOwner(objects);

	CUndo undo("Add Objects To Prefab");

	pPrefab->AddMembers(objects);

	// If we have nested dependencies between these object send an modify event afterwards to resolve them properly (e.g. shape objects linked to area triggers)
	for (int i = 0; i < objects.size(); i++)
	{
		objects[i]->UpdatePrefab();
	}
}

void CPrefabManager::ExtractObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects)
{
	if (childObjects.empty())
		return;

	CUndo undo("Extract object(s) from prefab");

	CloneObjectsFromPrefabs(childObjects);

	IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();
	for (CBaseObject* pChild : childObjects)
	{
		pObjectManager->DeleteObject(pChild);
	}
}

void CPrefabManager::ExtractAllFromPrefabs(std::vector<CPrefabObject*>& prefabs)
{
	if (prefabs.empty())
		return;

	CUndo undo("Extract all from prefab(s)");

	CloneAllFromPrefabs(prefabs);

	IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();
	for (CPrefabObject* pPrefab : prefabs)
		pObjectManager->DeleteObject(pPrefab);
}

void CPrefabManager::ExtractAllFromSelection()
{
	std::vector<CPrefabObject*> prefabs;
	Private_PrefabManager::GetSelectedPrefabs(prefabs);

	if (!prefabs.empty())
	{
		ExtractAllFromPrefabs(prefabs);
	}
}

void CPrefabManager::CloneObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects)
{
	if (childObjects.empty())
		return;

	std::unordered_map<CPrefabObject*, std::unordered_set<CBaseObject*>> objectsPerPrefabContainer;
	Private_PrefabManager::CollectAffectedPrefabsFor(childObjects, objectsPerPrefabContainer);
	Private_PrefabManager::CloneObjects(objectsPerPrefabContainer);
}

void CPrefabManager::CloneAllFromPrefabs(std::vector<CPrefabObject*>& prefabs)
{
	IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();

	CUndo undo("Clone all from prefab(s)");

	std::vector<CBaseObject*> objectsToSelect;
	for (CPrefabObject* pPrefab : prefabs)
	{
		const auto childCount = pPrefab->GetChildCount();
		std::unordered_set<CBaseObject*> children;
		std::vector<CBaseObject*> newChildren;
		children.reserve(childCount);
		newChildren.reserve(childCount);

		for (auto i = 0; i < childCount; ++i)
		{
			children.insert(pPrefab->GetChild(i));
		}

		pPrefab->ExtractChildrenClones(children, newChildren);

		objectsToSelect.insert(objectsToSelect.cend(), newChildren.begin(), newChildren.end());
	}

	pObjectManager->SelectObjects(objectsToSelect);
}

void CPrefabManager::CloneAllFromSelection()
{
	std::vector<CPrefabObject*> prefabs;
	Private_PrefabManager::GetSelectedPrefabs(prefabs);

	if (!prefabs.empty())
	{
		GetIEditor()->GetPrefabManager()->CloneAllFromPrefabs(prefabs);
	}
}

bool CPrefabManager::OpenPrefabs(std::vector<CPrefabObject*>& prefabObjects)
{
	if (prefabObjects.empty())
		return false;

	bool bOpenedAtLeastOne = false;

	CUndo undo("Open prefabs");
	for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
	{
		CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
		if (!pPrefabObj->IsOpen())
		{
			if (CUndo::IsRecording())
				CUndo::Record(new CUndoGroupObjectOpenClose(pPrefabObj));

			pPrefabObj->Open();
			bOpenedAtLeastOne = true;
		}
	}

	return bOpenedAtLeastOne;
}

bool CPrefabManager::ClosePrefabs(std::vector<CPrefabObject*>& prefabObjects)
{
	if (prefabObjects.empty())
		return false;

	bool bClosedAtLeastOne = false;

	CUndo undo("Close prefabs");
	for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
	{
		CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
		if (pPrefabObj->IsOpen())
		{
			if (CUndo::IsRecording())
				CUndo::Record(new CUndoGroupObjectOpenClose(pPrefabObj));
			pPrefabObj->Close();
			bClosedAtLeastOne = true;
		}
	}

	return bClosedAtLeastOne;
}

void CPrefabManager::GetPrefabObjects(std::vector<CPrefabObject*>& outPrefabObjects)
{
	std::vector<CBaseObject*> prefabObjects;
	GetIEditorImpl()->GetObjectManager()->FindObjectsOfType(RUNTIME_CLASS(CPrefabObject), prefabObjects);
	if (prefabObjects.empty())
		return;

	for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
	{
		CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
		if (pPrefabObj == NULL)
			continue;

		outPrefabObjects.push_back(pPrefabObj);
	}
}

int CPrefabManager::GetPrefabInstanceCount(IDataBaseItem* pPrefabItem)
{
	int instanceCount = 0;
	std::vector<CPrefabObject*> prefabObjects;
	GetPrefabObjects(prefabObjects);

	if (pPrefabItem)
	{
		for (int i = 0, prefabsFound(prefabObjects.size()); i < prefabsFound; ++i)
		{
			CPrefabObject* pPrefabObject = (CPrefabObject*)prefabObjects[i];
			if (pPrefabObject->GetPrefabGuid() == pPrefabItem->GetGUID())
			{
				++instanceCount;
			}
		}
	}

	return instanceCount;
}

IDataBaseLibrary* CPrefabManager::AddLibrary(const string& library, bool bSetFullFilename /*= false*/)
{
	// Check if library with same name already exist.
	IDataBaseLibrary* const pBaseLib = FindLibrary(library);
	if (pBaseLib)
	{
		return pBaseLib;
	}

	CBaseLibrary* const pLib = MakeNewLibrary();
	pLib->SetName(library);

	const string filename = MakeFilename(library);

	pLib->SetFilename(filename);
	pLib->SetModified(false);

	m_libs.push_back(pLib);
	return pLib;
}

string CPrefabManager::MakeFilename(const string& library)
{
	return string().Format("%s.%s", library.c_str(), GetFileExtension());
}

void CPrefabManager::ImportAssetsFromLevel(XmlNodeRef& levelRoot)
{
	using namespace Private_PrefabManager;

	XmlNodeRef libs = levelRoot->findChild("PrefabsLibrary");
	if (!libs)
	{
		return;
	}

	ICryPak* const pCryPak = GetIEditor()->GetSystem()->GetIPak();

	for (int i = 0; i < libs->getChildCount(); i++)
	{
		XmlNodeRef libNode = libs->getChild(i);
		string libName;
		if (!libNode->getAttr("Name", libName))
		{
			continue;
		}

		if (strcmp(libNode->getTag(), "LevelLibrary") != 0)
		{
			const char* szRootFolder = "Prefabs";
			const string filename(PathUtil::Make(szRootFolder, libName, "xml"));

			if (pCryPak->IsFileExist(PathUtil::ReplaceExtension(filename.c_str(), "bak"), ICryPak::eFileLocation_OnDisk))
			{
				// already imported
				continue;
			}

			if (!pCryPak->IsFileExist(filename.c_str()))
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Prefab library file not found: %s", filename.c_str());
				continue;
			}

			libNode = XmlHelpers::LoadXmlFromFile(filename);
			const string path = PathUtil::Make(szRootFolder, libName);

			ImportPrefabLibrary(libNode, path);

			FileUtils::BackupFile(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), path).c_str());
		}
		else
		{
			const char* szRootFolder = "Prefabs/LevelLibrary";
			const string path = PathUtil::Make(szRootFolder, PathUtil::ToGamePath(GetIEditorImpl()->GetLevelFolder()));
			ImportPrefabLibrary(libNode, path);
		}
	}
}

void CPrefabManager::UpdateAllPrefabsToLatestVersion()
{
	CAssetManager* const pAssetManager = GetIEditor()->GetAssetManager();
	const CAssetType* const pPrefabType = pAssetManager->FindAssetType("Prefab");
	pAssetManager->ForeachAssetOfType(pPrefabType, [this](CAsset* pAsset)
	{
		//get the asset guid and use it to load the prefab item (asset guid is the same as prefab guid)
		const CryGUID& guid = pAsset->GetGUID();
		CPrefabItem* pItem = static_cast<CPrefabItem*>(LoadItem(guid));
		//upgrade the object and remember to reload all the objects in the scene to apply the new changes
		pItem->CheckVersionAndUpgrade();
		//update all the instances in the level (if there are any)
		pItem->UpdateObjects();
		//serialize the changes to file
		pItem->GetLibrary()->Save();
	});
}

std::vector<CPrefabItem*> CPrefabManager::GetAllPrefabItems(const std::vector<CBaseObject*>& objects)
{
	std::vector<CPrefabItem*> items;

	for (const CBaseObject* pObject : objects)
	{
		if (pObject->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			const CPrefabObject* pPrefabObject = static_cast<const CPrefabObject*>(pObject);
			CPrefabItem* pItem = pPrefabObject->GetPrefabItem();

			if (std::find(items.begin(), items.end(), pItem) == items.end())
			{
				items.push_back(pItem);
			}
		}
	}

	return items;
}

std::vector<CBaseObject*> CPrefabManager::FindAllInstancesOfItem(const CPrefabItem* pPrefabItem)
{
	CBaseObjectsArray objects;
	GetIEditor()->GetObjectManager()->FindObjectsOfType(OBJTYPE_PREFAB, objects);

	std::vector<CBaseObject*> instances;
	for (CBaseObject* pObject : objects)
	{
		CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(pObject);

		if (pPrefabObject->GetPrefabItem()->GetGUID() == pPrefabItem->GetGUID())
		{
			instances.push_back(pPrefabObject);
		}
	}

	return instances;
}

void CPrefabManager::SelectAllInstancesOfItem(const CPrefabItem* pPrefabItem)
{
	std::vector<CBaseObject*> instances = FindAllInstancesOfItem(pPrefabItem);
	GetIEditor()->GetObjectManager()->ClearSelection();
	GetIEditor()->GetObjectManager()->AddObjectsToSelection(instances);
}

void CPrefabManager::DeleteItem(IDataBaseItem* pItem)
{
	assert(pItem);

	CPrefabItem* pPrefabItem = (CPrefabItem*)pItem;

	// Delete all objects from object manager that have this prefab item
	std::vector<CryGUID> guids;
	CBaseObjectsArray objects;
	IObjectManager* pObjMan = GetIEditorImpl()->GetObjectManager();
	pObjMan->GetObjects(objects);
	for (int i = 0; i < objects.size(); ++i)
	{
		CBaseObject* pObj = objects[i];
		if (pObj->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			CPrefabObject* pPrefab = (CPrefabObject*) pObj;
			if (pPrefab->GetPrefabItem() == pPrefabItem)
			{
				// Collect guids first to delete objects later
				guids.push_back(pPrefab->GetId());
			}
		}
	}
	for (int i = 0; i < guids.size(); ++i)
	{
		CBaseObject* pObj = pObjMan->FindObject(guids[i]);
		if (pObj)
			pObjMan->DeleteObject(pObj);
	}

	IDataBaseLibrary* pLibrary = pItem->GetLibrary();

	CBaseLibraryManager::DeleteItem(pItem);

	//cleanup the library when it's been emptied
	if (pLibrary && !pLibrary->GetItemCount())
	{
		DeleteLibrary(pLibrary->GetName());
	}
}

IDataBaseItem* CPrefabManager::LoadItem(const CryGUID& guid)
{
	IDataBaseItem* pItem = FindItem(guid);
	if (pItem)
	{
		return pItem;
	}

	const CAsset* const pAsset = GetIEditor()->GetAssetManager()->FindAssetById(guid);
	if (pAsset && pAsset->GetFilesCount() && LoadLibrary(pAsset->GetFile(0)))
	{
		return FindItem(guid);
	}

	return nullptr;
}

IDataBaseLibrary* CPrefabManager::LoadLibrary(const string& filename, bool bReload)
{
	IDataBaseLibrary* pLibrary = CBaseLibraryManager::LoadLibrary(filename, bReload);
	if (bReload && pLibrary)
	{
		CPrefabLibrary* pPrefabLibrary = (CPrefabLibrary*)pLibrary;
		pPrefabLibrary->UpdatePrefabObjects();
	}
	return pLibrary;
}