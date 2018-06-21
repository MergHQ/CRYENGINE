// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CoreRegistry.h"

#include <CryReflection/Framework.h>
#include <CryReflection/IModule.h>
#include <CrySystem/ICryPlugin.h>


namespace Cry {
namespace Reflection {

class CCoreRegistry;

class CModule : public IModule
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(Cry::Reflection::IModule)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS_GUID(CModule, "EngineModule_Reflection", "{4E615AA3-5E2D-4E34-883F-4119C9286FB5}"_cry_guid);

	// TODO: Move this singleton constructor back to private. The only reason for it being public is the current UnitTest system.
	CModule();
	// ~ TODO

	static CModule& GetInstance();

	// Cry::IDefaultModule
	virtual const char* GetName() const override     { return "CryReflection"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	// ~Cry::IDefaultModule

	// IModule
	virtual ICoreRegistry* GetCoreRegistry() final { return &m_coreRegistry; };
	//~IModule

private:
	static CModule* s_pInstance;

	CCoreRegistry   m_coreRegistry;
};

} // ~Reflection namespace
} // ~Cry namespace
