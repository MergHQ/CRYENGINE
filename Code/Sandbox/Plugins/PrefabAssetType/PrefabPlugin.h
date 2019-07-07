// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IPlugin.h"

class CPrefabPlugin : public IPlugin
{
public:
	CPrefabPlugin() { /* entry point of the plugin, perform initializations */ }
	~CPrefabPlugin() { /* exit point of the plugin, perform cleanup */ }

	int32       GetPluginVersion() { return 1; }
	const char* GetPluginName() { return "Prefab Plugin"; }
	const char* GetPluginDescription() { return "Prefabs are groups of objects that can be placed in the level as instances"; }
};
