// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PluginDll.h"

#include <CryEntitySystem/IEntityClass.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CryCore/StaticInstanceList.h>

IEntityRegistrator* IEntityRegistrator::g_pFirst = nullptr;
IEntityRegistrator* IEntityRegistrator::g_pLast = nullptr;

class CSystemEventListener : public ISystemEventListener
{
public:
	~CSystemEventListener()
	{
		gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
	}

	void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
	{
		switch (event)
		{
		case ESYSTEM_EVENT_GAME_POST_INIT:
			{
				// Register entities
				if (IEntityRegistrator::g_pFirst != nullptr)
				{
					do
					{
						IEntityRegistrator::g_pFirst->Register();

						IEntityRegistrator::g_pFirst = IEntityRegistrator::g_pFirst->m_pNext;
					}
					while (IEntityRegistrator::g_pFirst != nullptr);
				}

				// Register dummy entities
				IEntityClassRegistry::SEntityClassDesc stdClass;
				stdClass.flags |= ECLF_INVISIBLE;

				stdClass.sName = "AreaBox";
				gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(stdClass);

				stdClass.sName = "AreaSphere";
				gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(stdClass);

				stdClass.sName = "AreaSolid";
				gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(stdClass);

				stdClass.sName = "ClipVolume";
				gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(stdClass);

				stdClass.sName = "AreaShape";
				gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(stdClass);
			}

			{
				auto staticAutoRegisterLambda = []( Schematyc::IEnvRegistrar& registrar )
				{
					// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
					Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
				};

				gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
					stl::make_unique<Schematyc::CEnvPackage>(
						"{CB9E7C85-3289-41B6-983A-6A076ABA6351}"_cry_guid,
						"EntityComponents",
						"Crytek GmbH",
						"CRYENGINE Default Entity Components",
						staticAutoRegisterLambda
						)
				);
			}
			break;
		}
	}
};

CSystemEventListener g_listener;

bool CPlugin_CryDefaultEntities::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	env.pSystem->GetISystemEventDispatcher()->RegisterListener(&g_listener,"CCryPluginManager::CSystemEventListener");

	return true;
}

CRYREGISTER_SINGLETON_CLASS(CPlugin_CryDefaultEntities)

#include <CryCore/CrtDebugStats.h>
