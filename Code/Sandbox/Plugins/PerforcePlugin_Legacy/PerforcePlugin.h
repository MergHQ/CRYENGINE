// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IPlugin.h>

class CPerforcePlugin : public IPlugin, public IAutoEditorNotifyListener
{
public:
	CPerforcePlugin();
	~CPerforcePlugin();

	int32       GetPluginVersion() override;
	const char* GetPluginName() override;
	const char* GetPluginDescription() override { return "Perforce source control integration"; }
	void        OnEditorNotifyEvent(EEditorNotifyEvent aEventId) override;
};
