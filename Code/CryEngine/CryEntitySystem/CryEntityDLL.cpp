// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryEntitySystem/IEntitySystem.h>
#include "EntitySystem.h"
// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

CEntitySystem* g_pIEntitySystem = NULL;

struct CSystemEventListner_Entity : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
			break;
		case ESYSTEM_EVENT_LEVEL_LOAD_START:
			if (g_pIEntitySystem)
				g_pIEntitySystem->OnLevelLoadStart();
			break;
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			{
				if (g_pIEntitySystem )
				{
					if (!gEnv->pSystem->IsSerializingFile())
					{
						// activate the default layers
						g_pIEntitySystem->EnableDefaultLayers();
					}
					{
						LOADING_TIME_PROFILE_SECTION_NAMED("ENTITY_EVENT_LEVEL_LOADED");
						SEntityEvent loadingCompleteEvent(ENTITY_EVENT_LEVEL_LOADED);
						g_pIEntitySystem->SendEventToAll( loadingCompleteEvent );
					}
				}
			}
			break;
		case ESYSTEM_EVENT_3D_POST_RENDERING_END:
			if (g_pIEntitySystem)
			{
				g_pIEntitySystem->Unload();
			}
			break;
		}
	}
};
static CSystemEventListner_Entity g_system_event_listener_entity;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_EntitySystem : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_EntitySystem, "EngineModule_CryEntitySystem", 0x885655072f014c03, 0x820c5a1a9b4d623b)

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() override { return "CryEntitySystem"; };
	virtual const char* GetCategory() override { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		ISystem* pSystem = env.pSystem;

		CEntitySystem* pEntitySystem = new CEntitySystem(pSystem);
		g_pIEntitySystem = pEntitySystem;
		if (!pEntitySystem->Init(pSystem))
		{
			pEntitySystem->Release();
			return false;
		}
		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_entity);

		env.pEntitySystem = pEntitySystem;
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_EntitySystem)

CEngineModule_EntitySystem::CEngineModule_EntitySystem()
{
};

CEngineModule_EntitySystem::~CEngineModule_EntitySystem()
{
};

#include <CryCore/CrtDebugStats.h>
