// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

class CPlugin_CryDefaultEntities
	: public ICryPlugin
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CPlugin_CryDefaultEntities, "Plugin_CryDefaultEntities", 0x2C51634796014B70, 0xBB74CE14DD711EE6)

	PLUGIN_FLOWNODE_REGISTER
	PLUGIN_FLOWNODE_UNREGISTER

public:
	//! Retrieve name of plugin.
	virtual const char* GetName() const override { return "CryDefaultEntities"; }

	//! Retrieve category for the plugin.
	virtual const char* GetCategory() const override { return "Default"; }

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;

	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}
};
