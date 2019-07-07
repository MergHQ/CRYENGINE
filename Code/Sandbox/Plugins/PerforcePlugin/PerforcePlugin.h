// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IPlugin.h"

class CPerforcePlugin : public IPlugin
{
public:
	CPerforcePlugin();

	int32       GetPluginVersion();
	const char* GetPluginName();
	const char* GetPluginDescription() { return "Perforce source control integration"; }
};
