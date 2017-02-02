// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

USE_CRYPLUGIN_FLOWNODES

class CCryPerceptionSystemPlugin : public ICryPerceptionSystemPlugin
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(ICryPerceptionSystemPlugin)
		CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CCryPerceptionSystemPlugin, "Plugin_CryPerceptionSystem", 0xA4EE250934684BAD, 0x8472DC66D91186C6)

	PLUGIN_FLOWNODE_REGISTER
	PLUGIN_FLOWNODE_UNREGISTER

	CCryPerceptionSystemPlugin() : m_pPerceptionManager(nullptr) {}
	virtual ~CCryPerceptionSystemPlugin() {}

	// ICryPlugin
	virtual const char* GetName() const override { return "CryPerceptionSystem"; }
	virtual const char* GetCategory() const override { return "Plugin"; }
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	// ~ICryPlugin

public:

	// ICryPerceptionSystemPlugin
	virtual IPerceptionManager& GetPerceptionManager() const override;
	// ~ICryPerceptionSystemPlugin

protected:

	// IPluginUpdateListener
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}
	// ~IPluginUpdateListener

private:
	std::unique_ptr<CPerceptionManager> m_pPerceptionManager;
};

//-----------------------------------------------------------------------------------------------------------
bool CCryPerceptionSystemPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	m_pPerceptionManager.reset(new CPerceptionManager);
	return true;
}

//-----------------------------------------------------------------------------------------------------------
IPerceptionManager& CCryPerceptionSystemPlugin::GetPerceptionManager() const
{
	CRY_ASSERT(m_pPerceptionManager);
	return *m_pPerceptionManager.get();
}

//-----------------------------------------------------------------------------------------------------------
CRYREGISTER_SINGLETON_CLASS(CCryPerceptionSystemPlugin)
