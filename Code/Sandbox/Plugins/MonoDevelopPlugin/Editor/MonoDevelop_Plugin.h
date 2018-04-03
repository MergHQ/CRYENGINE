// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Include/IPlugin.h>

class CMonoDevelopPlugin : public IPlugin
{
public:

	CMonoDevelopPlugin();

	// IPlugin
	void        Release();
	void        ShowAbout();
	const char* GetPluginGUID();
	DWORD       GetPluginVersion();
	const char* GetPluginName();
	bool        CanExitNow();
	void        Serialize(FILE* hFile, bool bIsStoring);
	void        ResetContent();
	bool        CreateUIElements();
	void        OnEditorNotify(EEditorNotifyEvent eventId);
	// ~IPlugin
};

