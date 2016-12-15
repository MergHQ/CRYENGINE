// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/ISTDEnv.h>

namespace Schematyc
{
// Froward declare interfaces.
struct IEnvRegistrar;
// Forward declare classes.
class CEntityObjectClassRegistry;
class CEntityObjectDebugger;
class CEntityObjectMap;
class CSystemStateMonitor;

class CSTDEnv : public ISTDEnv, public ISystemEventListener
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(CSTDEnv)
	CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CSTDEnv, "Plugin_SchematycSTDEnv", 0x034c1d02501547ab, 0xb216c87cc5258a61)

public:
	CSTDEnv();
	virtual ~CSTDEnv();

	// ICryPlugin
	virtual const char* GetName() const override;
	virtual const char* GetCategory() const override;
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	// ~ICryPlugin

	//IPluginUpdateListener
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}
	// ~IPluginUpdateListener

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	const CSystemStateMonitor&  GetSystemStateMonitor() const;
	CEntityObjectClassRegistry& GetEntityObjectClassRegistry();
	CEntityObjectMap&           GetEntityObjectMap();
	CEntityObjectDebugger&      GetEntityObjectDebugger();

	static CSTDEnv&             GetInstance();

private:

	void RegisterPackage(IEnvRegistrar& registrar);

private:

	static CSTDEnv*                             s_pInstance;

	std::unique_ptr<CSystemStateMonitor>        m_pSystemStateMonitor;
	std::unique_ptr<CEntityObjectClassRegistry> m_pEntityObjectClassRegistry;
	std::unique_ptr<CEntityObjectMap>           m_pEntityObjectMap;
	std::unique_ptr<CEntityObjectDebugger>      m_pEntityObjectDebugger;
	CConnectionScope                            m_connectionScope;
};
} // Schematyc
