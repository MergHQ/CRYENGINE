// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryEntitySystem/IEntitySystem.h>
#include "EntitySystem.h"
// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#include  <CrySchematyc/Env/IEnvRegistry.h>
#include  <CrySchematyc/Env/Elements/EnvComponent.h>
#include  <CrySchematyc/Env/EnvPackage.h>
#include <CryCore/StaticInstanceList.h>

CEntitySystem* g_pIEntitySystem = NULL;

static void RegisterToSchematyc(Schematyc::IEnvRegistrar& registrar)
{
	// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
	Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
}

struct CSystemEventListner_Entity : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		case ESYSTEM_EVENT_GAME_POST_INIT:
			{
				gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
					stl::make_unique<Schematyc::CEnvPackage>(
						"A37D36D5-2AB1-4B48-9353-3DEC93A4236A"_cry_guid,
						"EntityComponents",
						"Crytek GmbH",
						"CRYENGINE Default Entity Components",
						&RegisterToSchematyc
						)
				);
			}
			break;

		case ESYSTEM_EVENT_LEVEL_LOAD_START:
			if (g_pIEntitySystem)
				g_pIEntitySystem->OnLevelLoadStart();
			break;
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			{
				if (g_pIEntitySystem)
				{
					if (!gEnv->pSystem->IsSerializingFile())
					{
						// activate the default layers
						g_pIEntitySystem->EnableDefaultLayers();
					}
					{
						LOADING_TIME_PROFILE_SECTION_NAMED("ENTITY_EVENT_LEVEL_LOADED");
						SEntityEvent loadingCompleteEvent(ENTITY_EVENT_LEVEL_LOADED);
						g_pIEntitySystem->SendEventToAll(loadingCompleteEvent);
					}
					g_pIEntitySystem->OnLevelLoadEnd();
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
class CEngineModule_EntitySystem : public IEntitySystemEngineModule
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(IEntitySystemEngineModule)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CEngineModule_EntitySystem, "EngineModule_CryEntitySystem", 0x885655072f014c03, 0x820c5a1a9b4d623b)

	virtual ~CEngineModule_EntitySystem()
	{
		GetISystem()->GetISystemEventDispatcher()->RemoveListener(&g_system_event_listener_entity);
		SAFE_RELEASE(gEnv->pEntitySystem);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() const override     { return "CryEntitySystem"; };
	virtual const char* GetCategory() const override { return "CryEngine"; };

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

		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_entity, "CSystemEventListner_Entity");

		env.pEntitySystem = pEntitySystem;
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_EntitySystem)

#include <CryCore/CrtDebugStats.h>
