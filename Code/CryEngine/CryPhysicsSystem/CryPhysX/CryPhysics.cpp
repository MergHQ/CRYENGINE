// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include "world.h"

#ifndef STANDALONE_PHYSICS
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>
#endif
/* */

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Physics : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		//case ESYSTEM_EVENT_RANDOM_SEED:
		//	cry_random_seed(gEnv->bNoRandomSeed ? 0 : wparam);
		//	break;
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			break;
		case ESYSTEM_EVENT_3D_POST_RENDERING_END:
			break;
		}
	}
};

static CSystemEventListner_Physics g_system_event_listener_physics;

//////////////////////////////////////////////////////////////////////////

CRYPHYSICS_API IPhysicalWorld *CreatePhysicalWorld(ISystem *pSystem)
{
	ModuleInitISystem(pSystem, "CryPhysics");
	
	if (pSystem)
	{
		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_physics);
		return new PhysXWorld(pSystem->GetILog());
	}

	return new PhysXWorld(0);
}

//////////////////////////////////////////////////////////////////////////

#ifndef STANDALONE_PHYSICS
//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryPhysics : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
		CRYGENERATE_SINGLETONCLASS(CEngineModule_CryPhysics, "EngineModule_CryPhysics", 0x526cabf3d776407f, 0xaa2338545bb6ae7f)

		//////////////////////////////////////////////////////////////////////////
		virtual const char *GetName() const override { return "CryPhysics"; };
	virtual const char *GetCategory() const override { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment &env, const SSystemInitParams &initParams) override
	{
		ISystem* pSystem = env.pSystem;

		if (pSystem)
			pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_physics);

		env.pPhysicalWorld = new PhysXWorld(pSystem ? pSystem->GetILog() : 0);

		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryPhysics)

#endif
