// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryEntitySystem/IEntitySystem.h>
#include "EntitySystem.h"
#include "EntityUnitTests.h"
#include "EntityClassRegistry.h"
#include "EntityCVars.h"
#include "FlowGraphComponent.h"

#include "Schematyc/EntitySchematycActions.h"
#include "Schematyc/EntitySchematycUtilFunctions.h"
#include "Schematyc/EntityUtilsComponent.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CryCore/StaticInstanceList.h>

CEntitySystem* g_pIEntitySystem = nullptr;
constexpr CryGUID SchematyEntityComponentsPackageGUID = "A37D36D5-2AB1-4B48-9353-3DEC93A4236A"_cry_guid;

struct CSystemEventListener_Entity : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		case ESYSTEM_EVENT_GAME_POST_INIT:
			{
				static_cast<CEntityClassRegistry*>(g_pIEntitySystem->GetClassRegistry())->OnGameFrameworkInitialized();
			}
			break;
		case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
		{

			auto entitySchematycRegistration = [](Schematyc::IEnvRegistrar& registrar)
			{
				Schematyc::CEntityTimerAction::Register(registrar);
				Schematyc::CEntityDebugTextAction::Register(registrar);
				Schematyc::Entity::RegisterUtilFunctions(registrar);
				Schematyc::CEntityUtilsComponent::Register(registrar);
				
#ifdef CRY_TESTING
				RegisterUnitTestComponents(registrar);
#endif
			};

			gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
				stl::make_unique<Schematyc::CEnvPackage>(
					SchematyEntityComponentsPackageGUID,
					"EntityComponents",
					"Crytek GmbH",
					"CRYENGINE Default Entity Components",
					entitySchematycRegistration
					)
			);

			// Check if flowgraph components are enabled
			if (CVar::pFlowgraphComponents->GetIVal() != 0)
			{
				// Create signal flow nodes
				gEnv->pSchematyc->GetEnvRegistry().VisitSignals([](const Schematyc::IEnvSignal& signal) -> Schematyc::EVisitStatus
				{
					const Schematyc::IEnvElement* pEnvElement = signal.GetParent();

					if (Schematyc::EEnvElementType::Component == pEnvElement->GetType())
					{
						const Schematyc::IEnvComponent* pEnvComponent = static_cast<const Schematyc::IEnvComponent*>(pEnvElement);

						stack_string  nodeName = stack_string("EntityComponents:") + pEnvComponent->GetName() + ":" + "Signals:" + signal.GetName();

						_smart_ptr<CEntityComponentFlowNodeFactorySignal> pSignalFactory = new CEntityComponentFlowNodeFactorySignal(*pEnvComponent, signal);
						gEnv->pFlowSystem->RegisterType(nodeName.c_str(), pSignalFactory);
					}

					return Schematyc::EVisitStatus::Continue;
				});

				gEnv->pSchematyc->GetEnvRegistry().VisitComponents([](const Schematyc::IEnvComponent& component) -> Schematyc::EVisitStatus
				{
					stack_string  typeName = stack_string("EntityComponents:") + component.GetName();
					stack_string  nodeName;

					// Create getter flow node
					nodeName = typeName + ':' + "Get:" + "GetParameter";
					_smart_ptr<CEntityComponentFlowNodeFactoryGetter> pGetterFactory = new CEntityComponentFlowNodeFactoryGetter(component, component.GetDesc().GetMembers());
					gEnv->pFlowSystem->RegisterType(nodeName.c_str(), pGetterFactory);

					// Create setter flow node
					nodeName = typeName + ':' + "Set:" + "SetParameter";
					_smart_ptr<CEntityComponentFlowNodeFactorySetter> pSetterFactory = new CEntityComponentFlowNodeFactorySetter(component, component.GetDesc().GetMembers());
					gEnv->pFlowSystem->RegisterType(nodeName.c_str(), pSetterFactory);

					// Create function flow node
					component.VisitChildren([typeName, &component](const Schematyc::IEnvElement& element) -> Schematyc::EVisitStatus
					{
						switch (element.GetType())
						{
						case Schematyc::EEnvElementType::Function:
						{
							const Schematyc::CEnvFunction& function = static_cast<const Schematyc::CEnvFunction&>(element);

							stack_string nodeName = typeName + ':' + "Functions:" + function.GetName();

							_smart_ptr<CEntityComponentFlowNodeFactory> pFactory = new CEntityComponentFlowNodeFactory(component, function);
							gEnv->pFlowSystem->RegisterType(nodeName.c_str(), pFactory);
						}
						break;
						}

						return Schematyc::EVisitStatus::Continue;
					});

					return Schematyc::EVisitStatus::Continue;
				});
			}
		}
		break;
		case ESYSTEM_EVENT_FULL_SHUTDOWN:
		case ESYSTEM_EVENT_FAST_SHUTDOWN:
			{
				// Deregister the Schematyc packages which were registered in the entity system.
				if (gEnv->pSchematyc)
				{
					gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(SchematyEntityComponentsPackageGUID);

					g_pIEntitySystem->GetClassRegistry()->UnregisterSchematycEntityClass();
				}
			}
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

					g_pIEntitySystem->OnLevelLoaded();
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
static CSystemEventListener_Entity g_system_event_listener_entity;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_EntitySystem : public IEntitySystemEngineModule
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(IEntitySystemEngineModule)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS_GUID(CEngineModule_EntitySystem, "EngineModule_CryEntitySystem", "88565507-2f01-4c03-820c-5a1a9b4d623b"_cry_guid)

	virtual ~CEngineModule_EntitySystem()
	{
		GetISystem()->GetISystemEventDispatcher()->RemoveListener(&g_system_event_listener_entity);
		SAFE_RELEASE(g_pIEntitySystem);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() const override     { return "CryEntitySystem"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }

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

		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_entity, "CSystemEventListener_Entity");

		env.pEntitySystem = pEntitySystem;
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_EntitySystem)

#include <CryCore/CrtDebugStats.h>
