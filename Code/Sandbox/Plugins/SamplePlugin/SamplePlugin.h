// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IPlugin.h"

#if defined(SamplePlugin_EXPORTS)
	#define SAMPLE_PLUGIN_API DLL_EXPORT
#else
	#define SAMPLE_PLUGIN_API DLL_IMPORT
#endif

class SAMPLE_PLUGIN_API CSamplePlugin : public IPlugin
{
public:
	CSamplePlugin() { /* entry point of the plugin, perform initializations */ }
	~CSamplePlugin() { /* exit point of the plugin, perform cleanup */ }

	int32       GetPluginVersion() { return 1; }
	const char* GetPluginName() { return "Sample Plugin"; }
	const char* GetPluginDescription() { return "Plugin used as a code sample to demonstrate Sandbox's plugin system"; }

private:
};
