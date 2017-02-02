// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/ICryPerceptionSystemPlugin.h"

class CPerceptionManager;

class CCryPerceptionSystemPlugin : public ICryPerceptionSystemPlugin
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(ICryPerceptionSystemPlugin)
		CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CCryPerceptionSystemPlugin, "Plugin_CryPerceptionSystem", 0xA4EE250934684BAD, 0x8472DC66D91186C6)

	virtual ~CCryPerceptionSystemPlugin() {}

	// ICryPlugin
	virtual const char* GetName() const override;
	virtual const char* GetCategory() const override;
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	// ~ICryPlugin

public:

	// ICrySensorSystemPlugin
	virtual IPerceptionManager& GetPerceptionManager() const override;
	// ~ICrySensorSystemPlugin

protected:

	// IPluginUpdateListener
	virtual void OnPluginUpdate(EPluginUpdateType updateType) {}
	// ~IPluginUpdateListener

private:

	std::unique_ptr<CPerceptionManager> m_pPerceptionManager;
};
