// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibrary.h"

//!Stores and serializes prefab items
//!Only one prefab item per library is allowed
class SANDBOX_API CPrefabLibrary : public CBaseLibrary
{
public:
	CPrefabLibrary(CBaseLibraryManager* pManager) : CBaseLibrary(pManager) {}
	virtual bool Save();
	virtual bool Load(const string& filename);
	virtual void Serialize(XmlNodeRef& node, bool bLoading);

	//!Add an item to this library if it's empty (only one item is allowed)
	virtual void AddItem(IDataBaseItem* item, bool bRegister /* = true */) override;

	//!Update all prefab objects instantiated from the first (and only item in the library)
	void         UpdatePrefabObjects();
};
