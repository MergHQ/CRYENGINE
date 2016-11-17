// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IPlugin.h>

class CSchematycPlugin : public IPlugin
{
public:

	CSchematycPlugin();

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
