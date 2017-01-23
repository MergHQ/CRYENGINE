// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IPlugin.h"

class CMFCToolsPlugin : public IPlugin
{
public:
	CMFCToolsPlugin() {}
	~CMFCToolsPlugin() {}

	int32       GetPluginVersion() { return 1; };
	const char* GetPluginName() { return "MFC Tools"; };
	const char* GetPluginDescription() { return "Legacy tools that use MFC as their UI toolkit. Windows only."; };

private:
};
