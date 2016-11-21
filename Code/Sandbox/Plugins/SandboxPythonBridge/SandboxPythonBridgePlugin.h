// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <IPlugin.h>

class SandboxPythonBridgePlugin : public IPlugin
{
public:
	SandboxPythonBridgePlugin();

	virtual void Release() override;
	virtual void ShowAbout() override;
	virtual const char* GetPluginGUID() override;
	virtual DWORD GetPluginVersion() override;
	virtual const char* GetPluginName() override;
	virtual bool CanExitNow() override;
	virtual void OnEditorNotify(EEditorNotifyEvent aEventId) override;
};
