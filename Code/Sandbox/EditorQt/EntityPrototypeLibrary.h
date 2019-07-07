// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibrary.h"

/** Library of prototypes.
 */
class SANDBOX_API CEntityPrototypeLibrary : public CBaseLibrary
{
public:
	CEntityPrototypeLibrary(CBaseLibraryManager* pManager) : CBaseLibrary(pManager) {}
	virtual bool Save();
	virtual bool Load(const string& filename);
	virtual void Serialize(XmlNodeRef& node, bool bLoading);
};
