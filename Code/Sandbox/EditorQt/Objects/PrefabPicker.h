// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
class CPrefabObject;

class CPrefabPicker
{
public:
	//!Spawns a resource selector dialog to allow selection of another Prefab Asset to assign to this object
	void SwapPrefab(CPrefabObject* pPrefabObject);
	//!Set the prefab pointed by this asset on the object
	//! \return true if the new prefab is set
	static bool SetPrefabFromAsset(CPrefabObject* pPrefabObject, const CAsset* pNewPrefabAsset);
	//!Set the prefab pointed by this asset on the object
	//! \return true if the new prefab is set and the asset exists
	static bool SetPrefabFromAssetFilename(CPrefabObject* pPrefabObject, const string& assetFilename);
	//!If this asset can be assigned to this prefab, compares all the prefabs GUIDs in the parent hierarchy of pPrefabObject with all the children of pNewPrefabAsset
	static bool IsValidAssetForPrefab(const CPrefabObject* pPrefabObject, const CAsset& pNewPrefabAsset);
	//!Get all the prefab objects in pObject hierarchy
	//! \param pObject search for prefab objects in this hierarchy
	//! \param outAllChildren all the children in pObject hierarchy that are of type CPrefabObject
	static void GetAllPrefabObjectDescendants(const CBaseObject* pObject, std::vector<CPrefabObject*>& outAllChildren);
};