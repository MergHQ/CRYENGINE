// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#include "CryFont.h"
#if defined(USE_NULLFONT)
	#include "NullFont.h"
#endif

///////////////////////////////////////////////
extern "C" ICryFont * CreateCryFontInterface(ISystem * pSystem)
{
	ModuleInitISystem(pSystem, "CryFont");

	if (gEnv->IsDedicated())
	{
#if defined(USE_NULLFONT)
		return new CCryNullFont();
#else
		// The NULL font implementation must be present for all platforms
		// supporting running as a pure dedicated server.
		pSystem->GetILog()->LogError("Missing NULL font implementation for dedicated server");
		return NULL;
#endif
	}
	else
	{
#if defined(USE_NULLFONT) && defined(USE_NULLFONT_ALWAYS)
		return new CCryNullFont();
#else
		return new CCryFont(pSystem);
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryFont : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryFont, "EngineModule_CryFont", 0x6758643f43214957, 0x9b920d898d31f434)

	virtual ~CEngineModule_CryFont() {}

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() const override { return "CryFont"; };
	virtual const char* GetCategory() const override { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		ISystem* pSystem = env.pSystem;
		env.pCryFont = CreateCryFontInterface(pSystem);
		return env.pCryFont != 0;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryFont)