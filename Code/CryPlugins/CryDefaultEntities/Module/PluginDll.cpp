// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PluginDll.h"

#include <CryEntitySystem/IEntityClass.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CryCore/StaticInstanceList.h>

IEntityRegistrator* IEntityRegistrator::g_pFirst = nullptr;
IEntityRegistrator* IEntityRegistrator::g_pLast = nullptr;



CPlugin_CryDefaultEntities::~CPlugin_CryDefaultEntities()
{
	if (gEnv->pSchematyc != nullptr)
	{
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(GetSchematycPackageGUID());
	}

	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

bool CPlugin_CryDefaultEntities::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	env.pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CCryPluginManager");

	return true;
}

void CPlugin_CryDefaultEntities::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
	{
		// Register legacy components introduced with 5.3 and below
		if (IEntityRegistrator::g_pFirst != nullptr)
		{
			do
			{
				IEntityRegistrator::g_pFirst->Register();

				IEntityRegistrator::g_pFirst = IEntityRegistrator::g_pFirst->m_pNext;
			} while (IEntityRegistrator::g_pFirst != nullptr);
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

		// Now register the new Schematyc components
		auto registerDefaultComponentsLambda = [](Schematyc::IEnvRegistrar& registrar)
		{
			Cry::DefaultComponents::RegisterAudioSpotComponent(registrar);
			Cry::DefaultComponents::RegisterPathfindingComponent(registrar);
			Cry::DefaultComponents::RegisterCameraComponent(registrar);
			Cry::DefaultComponents::RegisterLineConstraintComponent(registrar);
			Cry::DefaultComponents::RegisterPlaneConstraintComponent(registrar);
			Cry::DefaultComponents::RegisterPointConstraintComponent(registrar);
			Cry::DefaultComponents::RegisterDebugDrawComponent(registrar);
			Cry::DefaultComponents::RegisterDecalComponent(registrar);
			Cry::DefaultComponents::RegisterFogComponent(registrar);
			Cry::DefaultComponents::RegisterParticleComponent(registrar);
			Cry::DefaultComponents::RegisterAdvancedAnimationComponent(registrar);
			Cry::DefaultComponents::RegisterAlembicComponent(registrar);
			Cry::DefaultComponents::RegisterAnimatedMeshComponent(registrar);
			Cry::DefaultComponents::RegisterStaticMeshComponent(registrar);
			Cry::DefaultComponents::RegisterInputComponent(registrar);
			Cry::DefaultComponents::RegisterEnvironmentProbeComponent(registrar);
			Cry::DefaultComponents::RegisterPointLightComponent(registrar);
			Cry::DefaultComponents::RegisterProjectorLightComponent(registrar);
			Cry::DefaultComponents::RegisterBoxPrimitiveComponent(registrar);
			Cry::DefaultComponents::RegisterCapsulePrimitiveComponent(registrar);
			Cry::DefaultComponents::RegisterCharacterControllerComponent(registrar);
			Cry::DefaultComponents::RegisterCylinderPrimitiveComponent(registrar);
			Cry::DefaultComponents::RegisterStaticPhysicsComponent(registrar);
			Cry::DefaultComponents::RegisterSpherePrimitiveComponent(registrar);
		};

		gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
			stl::make_unique<Schematyc::CEnvPackage>(
				GetSchematycPackageGUID(),
				"EntityComponents",
				"Crytek GmbH",
				"CRYENGINE Default Entity Components",
				registerDefaultComponentsLambda
				)
		);
	}
	break;
	}
}

CRYREGISTER_SINGLETON_CLASS(CPlugin_CryDefaultEntities)

#include <CryCore/CrtDebugStats.h>
