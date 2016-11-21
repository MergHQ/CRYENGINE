#include "StdAfx.h"
#ifdef INCLUDE_SCALEFORM_SDK
	#include "ScaleformHelper.h"

// Engine module wrapper for Scaleform Helper.
// Note: This can either be loaded from inside CrySystem (if compiled in directly) or loaded from a DLL (if pre-compiled version is shipped).
// The only responsibility of the module is to set gEnv->pScaleformHelper, through which all CRYENGINE specific Scaleform interaction runs.
	#include <CrySystem/IEngineModule.h>
	#include <CryExtension/ClassWeaver.h>

class CEngineModule_ScaleformHelper : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_ScaleformHelper, "EngineModule_ScaleformHelper", 0x3d38f12a521d43cf, 0xca18fd1fa7ea5020)

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() override { return "CryScaleformHelper"; };

	#if CRY_IS_SCALEFORM_HELPER
	virtual const char* GetCategory() override { return "CryExtensions"; };
	#else
	virtual const char* GetCategory() override { return "CryEngine"; };
	#endif

	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		if (env.pRenderer)
		{
			ISystem* pSystem = env.pSystem;

			ModuleInitISystem(pSystem, "ScaleformHelper");
			env.pScaleformHelper = new CScaleformHelper();	
		}

		return true;
	}
};
CRYREGISTER_SINGLETON_CLASS(CEngineModule_ScaleformHelper)

// Declared through macro's above
CEngineModule_ScaleformHelper::CEngineModule_ScaleformHelper() {}
CEngineModule_ScaleformHelper::~CEngineModule_ScaleformHelper() {}
#endif

// If we are a stand-alone module, this will implement the module entry-points
#ifdef CRY_IS_SCALEFORM_HELPER
	#include <CryCore/Platform/platform_impl.inl>
#endif
