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
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}
#endif
#endif // WIN32

//////////////////////////////////////////////////////////////////////////
class CSystemEventListener_CryMonoBridge : public ISystemEventListener
{
	
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch(event)
		{
		case ESYSTEM_EVENT_GAME_POST_INIT:
			if(gEnv->pMonoRuntime)
				gEnv->pMonoRuntime->LoadGame();
			break;
		case ESYSTEM_EVENT_FULL_SHUTDOWN:
		case ESYSTEM_EVENT_FAST_SHUTDOWN:
			//SAFE_DELETE(gEnv->pMonoRuntime); // Leads to crash on engine shutdown. Need to investigate...
			break;
		}
	}
};

static CSystemEventListener_CryMonoBridge g_system_event_listener_crymonobridge;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryMonoBridge : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryMonoBridge, "EngineModule_CryMonoBridge", 0x2b4615a571524d67, 0x920dc857f8503b3a)

	virtual const char *GetName() { return "CryMonoBridge"; }
	virtual const char *GetCategory() { return "CryEngine"; }

	virtual bool Initialize(SSystemGlobalEnvironment &env, const SSystemInitParams &initParams)
	{
		env.pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_crymonobridge);
		env.pMonoRuntime = new CMonoRuntime (initParams.szProjectDllDir);
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryMonoBridge)

CEngineModule_CryMonoBridge::CEngineModule_CryMonoBridge()
{
	// register cvars, commands and stuff like that
}

CEngineModule_CryMonoBridge::~CEngineModule_CryMonoBridge()
{
}