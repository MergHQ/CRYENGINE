// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/ICryFramesPerSecondPlugin.h"

class CFramesPerSecond;

class CPlugin_CryFramesPerSecond : public ICryFramesPerSecondPlugin
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(ICryFramesPerSecondPlugin)
	CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CPlugin_CryFramesPerSecond, "Plugin_CryFramesPerSecond", 0x342a829a2bbe4a5c, 0x6c64bccd93b94a00)

	PLUGIN_FLOWNODE_REGISTER
	PLUGIN_FLOWNODE_UNREGISTER

	//! Retrieve name of plugin.
	virtual const char* GetName() const override { return "CryFramesPerSecond"; }

	//! Retrieve category for the plugin.
	virtual const char* GetCategory() const override { return "Plugin"; }

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;

public:
	virtual IFramesPerSecond* GetIFramesPerSecond() const override;

protected:
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override;

private:
	CFramesPerSecond* m_pFramesPerSecond;
};
