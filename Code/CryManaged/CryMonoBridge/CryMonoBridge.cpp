// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CryMonoBridge.h"

// Must be included only once in DLL module.
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#include "MonoRuntime.h"

#if defined(CRY_PLATFORM_WINDOWS)
	#ifndef _LIB
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
	#endif
#endif // WIN32

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryMonoBridge : public IMonoEngineModule
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(Cry::IDefaultModule)
		CRYINTERFACE_ADD(IMonoEngineModule)
	CRYINTERFACE_END()
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryMonoBridge, "EngineModule_CryMonoBridge", 0x2b4615a571524d67, 0x920dc857f8503b3a)

	virtual ~CEngineModule_CryMonoBridge() {}
	virtual const char* GetName() const override { return "CryMonoBridge"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }


	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		env.pMonoRuntime = new CMonoRuntime();

		return env.pMonoRuntime->Initialize();
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryMonoBridge)