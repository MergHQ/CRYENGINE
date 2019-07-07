// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <vector>

struct ISystem;

//////////////////////////////////////////////////////////////////////////
// Custom Actions Editor Manager
//////////////////////////////////////////////////////////////////////////
class CCustomActionsEditorManager
{
public:
	CCustomActionsEditorManager();
	~CCustomActionsEditorManager();
	void Init(ISystem* system);

	void ReloadCustomActionGraphs();
	void SaveCustomActionGraphs();
	void SaveAndReloadCustomActionGraphs();
	bool NewCustomAction(CString& filename);
	void GetCustomActions(std::vector<CString>& values) const;

private:
	void LoadCustomActionGraphs();
	void FreeCustomActionGraphs();
};
