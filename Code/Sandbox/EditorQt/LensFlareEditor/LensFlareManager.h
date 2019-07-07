// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibraryManager.h"
#include <CryRenderer/IFlares.h>

class IOpticsElementBase;
class CLensFlareEditor;

class SANDBOX_API CLensFlareManager : public CBaseLibraryManager
{
public:
	CLensFlareManager();
	virtual ~CLensFlareManager();

	void              ClearAll();

	virtual bool      LoadFlareItemByName(const string& fullItemName, IOpticsElementBasePtr pDestOptics);
	void              Modified();
	//! Path to libraries in this manager.
	string            GetLibsPath();
	IDataBaseLibrary* LoadLibrary(const string& filename, bool bReload = false);

private:
	CBaseLibraryItem* MakeNewItem();
	CBaseLibrary*     MakeNewLibrary();

	bool              OnPickFlare(const string& oldValue, string& newValue);

	//! Root node where this library will be saved.
	string GetRootNodeName();
	string m_libsPath;
};
