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
	//!If this asset can be assigned to this prefab
	static bool IsValidAssetForPrefab(const CPrefabObject* pPrefabObject, const CAsset& pNewPrefabAsset);
};