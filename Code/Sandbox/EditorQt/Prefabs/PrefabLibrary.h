// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibrary.h"

/** Library of prototypes.
 */
class SANDBOX_API CPrefabLibrary : public CBaseLibrary
{
public:
	CPrefabLibrary(CBaseLibraryManager* pManager) : CBaseLibrary(pManager) {}
	virtual bool Save();
	virtual bool Load(const string& filename);
	virtual void Serialize(XmlNodeRef& node, bool bLoading);
	void         UpdatePrefabObjects();
};
