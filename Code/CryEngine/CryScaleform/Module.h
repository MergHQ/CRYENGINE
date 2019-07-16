// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CrySystem/Scaleform/IScaleformHelper.h>

namespace Cry {
namespace ScaleformModule {

class CModule : public IModule
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(Cry::ScaleformModule::IModule)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS_GUID(CModule, "EngineModule_Scaleform", "{6B62A15E-8822-4A6A-981A-67962A840086}"_cry_guid);

	static CModule&     GetInstance();

	virtual const char* GetName() const override     { return "CryScaleform"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;

private:
	CModule();
	static CModule* s_pInstance;
};

} // ~ScaleformModule namespace
} // ~Cry namespace
