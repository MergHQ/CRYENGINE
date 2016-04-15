// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// CryAISystem.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CryAISystem.h"
#include <CryCore/Platform/platform_impl.inl>
#include "CAISystem.h"
#include "AILog.h"
#include <CrySystem/ISystem.h>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

CAISystem* g_pAISystem;

/*
   //////////////////////////////////////////////////////////////////////////
   // Pointer to Global ISystem.
   static ISystem* gISystem = 0;
   ISystem* GetISystem()
   {
   return gISystem;
   }
 */

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_AI : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
			break;
		}
	}
};
static CSystemEventListner_AI g_system_event_listener_ai;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAISystem : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAISystem, "EngineModule_CryAISystem", 0x6b8e79a784004f44, 0x97db7614428ad251)

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() override { return "CryAISystem"; };
	virtual const char* GetCategory() override { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		ISystem* pSystem = env.pSystem;

		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_ai);

		AIInitLog(pSystem);

		g_pAISystem = new CAISystem(pSystem);
		env.pAISystem = g_pAISystem;

		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAISystem)

CEngineModule_CryAISystem::CEngineModule_CryAISystem()
{
};

CEngineModule_CryAISystem::~CEngineModule_CryAISystem()
{
};

//////////////////////////////////////////////////////////////////////////
#include <CryCore/CrtDebugStats.h>

#ifndef _LIB
	#include <CryCore/Common_TypeInfo.h>
#endif

#include <CryCore/TypeInfo_impl.h>
#include "AutoTypeStructs_info.h"
