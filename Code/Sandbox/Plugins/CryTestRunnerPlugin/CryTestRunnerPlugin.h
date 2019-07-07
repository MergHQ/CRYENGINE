// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IPlugin.h"

class CCryTestRunnerPlugin : public IPlugin
{
public:
	CCryTestRunnerPlugin();
	~CCryTestRunnerPlugin() { /* exit point of the plugin, perform cleanup */ }

	int32       GetPluginVersion() { return 1; }
	const char* GetPluginName() { return "CryTest Runner Plugin"; }
	const char* GetPluginDescription() { return "Manage and execute CRYENGINE feature tests"; }

private:
};
