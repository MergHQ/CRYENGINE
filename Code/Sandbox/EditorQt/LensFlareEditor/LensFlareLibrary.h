// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibrary.h"

class SANDBOX_API CLensFlareLibrary : public CBaseLibrary
{
public:
	CLensFlareLibrary(CBaseLibraryManager* pManager) : CBaseLibrary(pManager) {}
	virtual bool          Save();
	virtual bool          Load(const string& filename);
	virtual void          Serialize(XmlNodeRef& node, bool bLoading);

	IOpticsElementBasePtr GetOpticsOfItem(const char* szflareName);
};
