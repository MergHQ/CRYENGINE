// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#ifdef CRY_TESTING
#include "Entity.h"
#include "EntitySystem.h"

#include <CrySystem/Testing/CryTest.h>
#include <CrySystem/Testing/CryTestCommands.h>
#include <CryMath/LCGRandom.h>

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>

#include "SubstitutionProxy.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryMath/Random.h>

CRY_TEST_SUITE(EntityTestsSuit)
{
	struct IUnifiedEntityComponent : public IEntityComponent
	{
		static void ReflectType(Schematyc::CTypeDesc<IUnifiedEntityComponent>& desc)
		{
			desc.SetGUID("{C89DD0DD-9850-43BA-BF52-86EFB7C9D0A5}"_cry_guid);
		}
	};

	class CUnifiedEntityComponent : public IUnifiedEntityComponent
	{
	public:
		static void ReflectType(Schematyc::CTypeDesc<CUnifiedEntityComponent>& desc)
		{
			desc.AddBase<IUnifiedEntityComponent>();
			desc.SetGUID("{C89DD0DD-9850-43BA-BF52-86EFB7C9D0A5}"_cry_guid);
			desc.SetEditorCategory("Unit Tests");
			desc.SetLabel("Unified Entity Component");
			desc.SetDescription("Does stuff");
			desc.SetIcon("icons:ObjectTypes/light.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform,
				IEntityComponent::EFlags::Socket,
				IEntityComponent::EFlags::Attach,
				IEntityComponent::EFlags::HiddenFromUser,
				IEntityComponent::EFlags::HideFromInspector });

			desc.AddMember(&CUnifiedEntityComponent::m_bMyBool, 'bool', "Bool", "Bool", "Bool desc", false);
			desc.AddMember(&CUnifiedEntityComponent::m_myFloat, 'floa', "Float", "Float", "Float desc", 0.f);
		}

		bool  m_bMyBool = false;
		float m_myFloat = 0.f;
	};

	struct SScopedSpawnEntity
	{
		SScopedSpawnEntity(const char* szName)
		{
			SEntitySpawnParams params;
			params.guid = CryGUID::Create();
			params.sName = szName;
			params.nFlags = ENTITY_FLAG_CLIENT_ONLY;

			pEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(params));
			CRY_TEST_ASSERT(pEntity != nullptr);
		}

		~SScopedSpawnEntity()
		{
			g_pIEntitySystem->RemoveEntity(pEntity->GetId());
		}

		CEntity* pEntity;
	};

	CRY_TEST(SpawnTest)
	{
		SScopedSpawnEntity entity("TestEntity");
		EntityId id = entity.pEntity->GetId();

		CRY_TEST_ASSERT(entity.pEntity->GetGuid() != CryGUID::Null());

		CRY_TEST_CHECK_EQUAL(id, g_pIEntitySystem->FindEntityByGuid(entity.pEntity->GetGuid()));
		CRY_TEST_CHECK_EQUAL(entity.pEntity, g_pIEntitySystem->FindEntityByName("TestEntity"));

		// Test Entity components
		IEntitySubstitutionComponent* pComponent = entity.pEntity->GetOrCreateComponent<IEntitySubstitutionComponent>();
		CRY_TEST_ASSERT(nullptr != pComponent);
		CRY_TEST_ASSERT(1 == entity.pEntity->GetComponentsCount());
	}

	class CLegacyEntityComponent : public IEntityComponent
	{
	public:
		CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS_GUID(CLegacyEntityComponent, "LegacyEntityComponent", "b1febcee-be12-46a9-b69a-cc66d6c0d3e0"_cry_guid);
	};

	CRYREGISTER_CLASS(CLegacyEntityComponent);

	CRY_TEST(CreateLegacyComponent)
	{
		SScopedSpawnEntity entity("LegacyComponentTestEntity");

		CLegacyEntityComponent* pComponent = entity.pEntity->GetOrCreateComponent<CLegacyEntityComponent>();
		CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 1);

		entity.pEntity->RemoveComponent(pComponent);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 0);
	}

	struct ILegacyComponentInterface : public IEntityComponent
	{
		CRY_ENTITY_COMPONENT_INTERFACE_GUID(ILegacyComponentInterface, "d9a71012-8af1-4426-8835-99c4f4eb987d"_cry_guid)

		virtual bool IsValid() const { return false; }
	};

	class CLegacyEntityComponentWithInterface final : public ILegacyComponentInterface
	{
	public:
		CRY_ENTITY_COMPONENT_CLASS_GUID(CLegacyEntityComponentWithInterface, ILegacyComponentInterface, "LegacyEntityComponentWithInterface", "34ee1b92-2154-4bad-b69a-cc66d6c0d3e0"_cry_guid);

		virtual bool IsValid() const final { return true; }
	};

	CRYREGISTER_CLASS(CLegacyEntityComponentWithInterface);

	CRY_TEST(CreateLegacyComponentWithInterface)
	{
		SScopedSpawnEntity entity("LegacyComponentTestInterfaceEntity");

		ILegacyComponentInterface* pComponent = entity.pEntity->GetOrCreateComponent<ILegacyComponentInterface>();
		CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 1);
		CRY_TEST_ASSERT(pComponent->IsValid());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<CLegacyEntityComponentWithInterface>());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<ILegacyComponentInterface>());

		entity.pEntity->RemoveComponent(pComponent);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 0);

		pComponent = entity.pEntity->GetOrCreateComponent<CLegacyEntityComponentWithInterface>();
		CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 1);
		CRY_TEST_ASSERT(pComponent->IsValid());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<CLegacyEntityComponentWithInterface>());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<ILegacyComponentInterface>());

		DynArray<CLegacyEntityComponentWithInterface*> components;
		entity.pEntity->GetAllComponents<CLegacyEntityComponentWithInterface>(components);
		CRY_TEST_CHECK_EQUAL(components.size(), 1);
		CRY_TEST_CHECK_EQUAL(components.at(0), static_cast<CLegacyEntityComponentWithInterface*>(pComponent));

		DynArray<ILegacyComponentInterface*> componentsbyInterface;
		entity.pEntity->GetAllComponents<ILegacyComponentInterface>(componentsbyInterface);
		CRY_TEST_CHECK_EQUAL(componentsbyInterface.size(), 1);
		CRY_TEST_CHECK_EQUAL(componentsbyInterface.at(0), static_cast<CLegacyEntityComponentWithInterface*>(pComponent));
	}

	CRY_TEST(CreateUnifiedComponent)
	{
		SScopedSpawnEntity entity("UnifiedComponentTestEntity");

		CUnifiedEntityComponent* pComponent = entity.pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
		CRY_TEST_ASSERT(pComponent);
		if (!pComponent)
			return;
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 1);
		CRY_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
		CRY_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<CUnifiedEntityComponent>());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<IUnifiedEntityComponent>());

		entity.pEntity->RemoveComponent(pComponent);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 0);

		pComponent = static_cast<CUnifiedEntityComponent*>(entity.pEntity->GetOrCreateComponent<IUnifiedEntityComponent>());
		CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 1);
		CRY_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
		CRY_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<CUnifiedEntityComponent>());
		CRY_TEST_CHECK_EQUAL(pComponent, entity.pEntity->GetComponent<IUnifiedEntityComponent>());

		DynArray<CUnifiedEntityComponent*> components;
		entity.pEntity->GetAllComponents<CUnifiedEntityComponent>(components);
		CRY_TEST_CHECK_EQUAL(components.size(), 1);
		CRY_TEST_CHECK_EQUAL(components.at(0), static_cast<CUnifiedEntityComponent*>(pComponent));

		DynArray<IUnifiedEntityComponent*> componentsbyInterface;
		entity.pEntity->GetAllComponents<IUnifiedEntityComponent>(componentsbyInterface);
		CRY_TEST_CHECK_EQUAL(componentsbyInterface.size(), 1);
		CRY_TEST_CHECK_EQUAL(componentsbyInterface.at(0), static_cast<CUnifiedEntityComponent*>(pComponent));
	}

	CRY_TEST(UnifiedComponentSerialization)
	{
		XmlNodeRef node = gEnv->pSystem->CreateXmlNode();
		CryGUID instanceGUID;

		{
			SScopedSpawnEntity entity("UnifiedComponentSerializationTestEntity");

			CUnifiedEntityComponent* pComponent = entity.pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
			CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
			if (!pComponent)
				return;
			// Check default values
			CRY_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
			CRY_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);

			pComponent->m_bMyBool = true;
			pComponent->m_myFloat = 1337.f;

			instanceGUID = pComponent->GetGUID();

			// Save to XML
			entity.pEntity->SerializeXML(node, false);
		}

		// Create another entity for deserialization
		SScopedSpawnEntity entity("UnifiedComponentDeserializationTestEntity");
		// Deserialize
		entity.pEntity->SerializeXML(node, true);

		CUnifiedEntityComponent* pComponent = entity.pEntity->GetComponent<CUnifiedEntityComponent>();
		CRY_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		// Check deserialized values
		CRY_TEST_CHECK_EQUAL(pComponent->m_bMyBool, true);
		CRY_TEST_CHECK_EQUAL(pComponent->m_myFloat, 1337.f);
		CRY_TEST_CHECK_EQUAL(pComponent->GetGUID(), instanceGUID);
	}

	CRY_TEST(QueryInvalidGUID)
	{
		SScopedSpawnEntity entity("UnifiedComponentTestEntity");
		entity.pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
		entity.pEntity->GetOrCreateComponent<CLegacyEntityComponentWithInterface>();
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponentsCount(), 2);
		// Querying the lowest level GUIDs is disallowed
		CRY_TEST_CHECK_EQUAL(entity.pEntity->GetComponent<IEntityComponent>(), nullptr);
	}

	class CComponent2 : public IEntityComponent
	{
	public:
		static void ReflectType(Schematyc::CTypeDesc<CComponent2>& desc)
		{
			desc.SetGUID("{8B80B6EA-3A85-48F0-89DC-EDE69E72BC08}"_cry_guid);
			desc.SetLabel("CComponent2");
			desc.SetComponentFlags({ IEntityComponent::EFlags::HiddenFromUser, IEntityComponent::EFlags::HideFromInspector});
		}

		virtual void Initialize() override
		{
			m_bInitialized = true;
		}

		bool m_bInitialized = false;
	};

	class CComponent1 : public IEntityComponent
	{
	public:
		static void ReflectType(Schematyc::CTypeDesc<CComponent1>& desc)
		{
			desc.SetGUID("{00235E09-55EC-46AD-A6C7-606EA4A40A6B}"_cry_guid);
			desc.SetLabel("CComponent1");
			desc.AddMember(&CComponent1::m_bLoadingFromDisk, 'load', "Loading", "Loading", "", false);
			desc.SetComponentFlags({ IEntityComponent::EFlags::HiddenFromUser, IEntityComponent::EFlags::HideFromInspector});
		}

		virtual void Initialize() override
		{
			if (m_bLoadingFromDisk)
			{
				// Make sure that CComponent2 is already available (but uninitialized)
				CComponent2* pOtherComponent = m_pEntity->GetComponent<CComponent2>();
				CRY_TEST_CHECK_DIFFERENT(pOtherComponent, nullptr);
				if (pOtherComponent != nullptr)
				{
					CRY_TEST_CHECK_EQUAL(pOtherComponent->m_bInitialized, false);
				}
			}
		}

		bool m_bLoadingFromDisk = false;
	};

	// Test whether two components will be deserialized into CEntity::m_components before being Initialized
	// Initialize must never be called during loading from disk if another component (in the same entity) has yet to be loaded
	CRY_TEST(LoadMultipleComponents)
	{
		XmlNodeRef xmlNode;

		{
			SScopedSpawnEntity entity("SaveMultipleComponents");
			CComponent1* pComponent1 = entity.pEntity->GetOrCreateComponent<CComponent1>();
			CComponent2* pComponent2 = entity.pEntity->GetOrCreateComponent<CComponent2>();

			CRY_TEST_ASSERT(pComponent1 != nullptr);
			CRY_TEST_ASSERT(pComponent2 != nullptr);

			if (!pComponent1 || !pComponent2)
				return;

			// Set boolean to true so we can assert existence of CComponent2 in CComponent1::Initialize
			pComponent1->m_bLoadingFromDisk = true;

			// Save
			xmlNode = gEnv->pSystem->CreateXmlNode("Entity");
			entity.pEntity->SerializeXML(xmlNode, false);
		}

		// Load
		SScopedSpawnEntity entity("LoadMultipleComponents");
		entity.pEntity->SerializeXML(xmlNode, true);

		CRY_TEST_CHECK_DIFFERENT(entity.pEntity->GetComponent<CComponent1>(), nullptr);
		CRY_TEST_CHECK_DIFFERENT(entity.pEntity->GetComponent<CComponent2>(), nullptr);
	}

	CRY_TEST(TestComponentEvent)
	{
		class CComponentWithEvent : public IEntityComponent
		{
		public:
			virtual void ProcessEvent(const SEntityEvent& event) override
			{
				CRY_TEST_CHECK_EQUAL(event.event, ENTITY_EVENT_PHYSICS_CHANGE_STATE);
				m_wasHit = true;
			}

			virtual Cry::Entity::EventFlags GetEventMask() const override { return ENTITY_EVENT_PHYSICS_CHANGE_STATE; }

			bool m_wasHit = false;
		};

		class CComponentWithoutEvent : public IEntityComponent
		{
		public:
			virtual void ProcessEvent(const SEntityEvent& event) override
			{
				CRY_TEST_ASSERT(false);
			}
		};

		SScopedSpawnEntity entity("TestComponentEvent");
		CComponentWithEvent* pComponent = entity.pEntity->CreateComponentClass<CComponentWithEvent>();
		entity.pEntity->CreateComponentClass<CComponentWithoutEvent>();
		pComponent->m_wasHit = false;
		pComponent->SendEvent(SEntityEvent(ENTITY_EVENT_PHYSICS_CHANGE_STATE));
		CRY_TEST_CHECK_EQUAL(pComponent->m_wasHit, true);

		pComponent->m_wasHit = false;
		entity.pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_PHYSICS_CHANGE_STATE));
		CRY_TEST_CHECK_EQUAL(pComponent->m_wasHit, true);

		// Ensure that we can send the events without any issues
		entity.pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_PHYSICS_CHANGE_STATE));
	}

	CRY_TEST(TestNestedComponentEvent)
	{
		class CComponentWithEvent : public IEntityComponent
		{
		public:
			virtual void ProcessEvent(const SEntityEvent& event) override
			{
				CRY_TEST_ASSERT(event.event == ENTITY_EVENT_PHYSICS_CHANGE_STATE || event.event == ENTITY_EVENT_SCRIPT_EVENT);

				m_receivedEvents++;

				if (event.event == ENTITY_EVENT_PHYSICS_CHANGE_STATE)
				{
					CRY_TEST_CHECK_EQUAL(m_receivedEvents, 1);
					m_pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_SCRIPT_EVENT));
					CRY_TEST_CHECK_EQUAL(m_receivedEvents, 2);
				}
				else if (event.event == ENTITY_EVENT_SCRIPT_EVENT)
				{
					CRY_TEST_CHECK_EQUAL(m_receivedEvents, 2);
				}
			}

			virtual Cry::Entity::EventFlags GetEventMask() const override { return ENTITY_EVENT_PHYSICS_CHANGE_STATE | ENTITY_EVENT_SCRIPT_EVENT; }

			uint8 m_receivedEvents = 0;
		};

		SScopedSpawnEntity entity("TestComponentEvent");
		CComponentWithEvent* pComponent = entity.pEntity->CreateComponentClass<CComponentWithEvent>();
		entity.pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_PHYSICS_CHANGE_STATE));
		entity.pEntity->RemoveComponent(pComponent);

		// Ensure that we can send the events without any issues
		entity.pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_PHYSICS_CHANGE_STATE));
		entity.pEntity->SendEvent(SEntityEvent(ENTITY_EVENT_SCRIPT_EVENT));
	}

	CRY_TEST(TestSpawnedTransformation)
	{
		SEntitySpawnParams params;
		params.guid = CryGUID::Create();
		params.sName = __FUNCTION__;
		params.nFlags = ENTITY_FLAG_CLIENT_ONLY;

		params.vPosition = Vec3(1, 2, 3);
		const Ang3 angles = Ang3(gf_PI, 0, gf_PI * 0.5f);
		params.qRotation = Quat::CreateRotationXYZ(angles);
		params.vScale = Vec3(1.5f, 1.f, 0.8f);

		CEntity* pEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(params));
		CRY_TEST_ASSERT(pEntity != nullptr);
		CRY_TEST_ASSERT(pEntity->GetParent() == nullptr);

		// Check world-space position
		CRY_TEST_CHECK_EQUAL(pEntity->GetWorldPos(), params.vPosition);
		CRY_TEST_CHECK_EQUAL(pEntity->GetWorldTM().GetTranslation(), params.vPosition);

		// Check world-space rotation
		CRY_TEST_CHECK_CLOSE(pEntity->GetWorldRotation(), params.qRotation, VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(Quat(pEntity->GetWorldTM()), params.qRotation, VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetRightDir(), params.qRotation.GetColumn0(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetForwardDir(), params.qRotation.GetColumn1(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetUpDir(), params.qRotation.GetColumn2(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetWorldAngles(), Ang3(params.qRotation), VEC_EPSILON);

		// Check world-space scale
		CRY_TEST_CHECK_CLOSE(pEntity->GetWorldScale(), params.vScale, VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetWorldTM().GetScale(), params.vScale, VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pEntity->GetWorldTM(), Matrix34::Create(params.vScale, params.qRotation, params.vPosition), VEC_EPSILON);

		// Entities with parents currently return the world space position for the local Get functions
		// Ensure that this still works, while it remains intended
		CRY_TEST_CHECK_EQUAL(pEntity->GetPos(), params.vPosition);
		CRY_TEST_CHECK_EQUAL(pEntity->GetRotation(), params.qRotation);
		CRY_TEST_CHECK_EQUAL(pEntity->GetScale(), params.vScale);
		CRY_TEST_CHECK_EQUAL(pEntity->GetLocalTM(), Matrix34::Create(params.vScale, params.qRotation, params.vPosition));

		g_pIEntitySystem->RemoveEntity(pEntity->GetId());
	}

	CRY_TEST(TestSpawnedChildTransformation)
	{
		SEntitySpawnParams parentParams;
		parentParams.guid = CryGUID::Create();
		parentParams.sName = __FUNCTION__;
		parentParams.nFlags = ENTITY_FLAG_CLIENT_ONLY;

		parentParams.vPosition = Vec3(1, 2, 3);
		const Ang3 angles = Ang3(gf_PI, 0, gf_PI * 0.5f);
		parentParams.qRotation = Quat::CreateRotationXYZ(angles);
		parentParams.vScale = Vec3(1.5f, 1.f, 0.8f);

		CEntity* const pParentEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(parentParams));
		CRY_TEST_ASSERT(pParentEntity != nullptr);
		CRY_TEST_ASSERT(pParentEntity->GetParent() == nullptr);

		SEntitySpawnParams childParams = parentParams;
		childParams.id = INVALID_ENTITYID;
		childParams.guid = CryGUID::Create();
		childParams.vPosition = Vec3(2, 2, 2);
		childParams.qRotation = Quat::CreateRotationZ(0.2f);
		childParams.vScale = Vec3(1.f, 1.f, 1.2f);
		childParams.pParent = pParentEntity;

		const CEntity* const pChildEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(childParams));
		CRY_TEST_ASSERT(pChildEntity != nullptr);
		CRY_TEST_ASSERT(pChildEntity->GetParent() == pParentEntity);

		// Ensure that parent world transform is correct
		CRY_TEST_CHECK_CLOSE(pParentEntity->GetWorldTM(), Matrix34::Create(parentParams.vScale, parentParams.qRotation, parentParams.vPosition), VEC_EPSILON);

		// Check world-space position
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetWorldPos(), pParentEntity->GetWorldTM().TransformPoint(childParams.vPosition));
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetWorldTM().GetTranslation(), pParentEntity->GetWorldTM().TransformPoint(childParams.vPosition));

		// Check world-space rotation
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetWorldRotation(), pParentEntity->GetWorldRotation() * childParams.qRotation, VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(Quat(pChildEntity->GetWorldTM()), pParentEntity->GetWorldRotation() * childParams.qRotation, VEC_EPSILON);

		Matrix34 childWorldTransform = pParentEntity->GetWorldTM() * Matrix34::Create(childParams.vScale, childParams.qRotation, childParams.vPosition);
		childWorldTransform.OrthonormalizeFast();

		CRY_TEST_CHECK_CLOSE(pChildEntity->GetRightDir(), childWorldTransform.GetColumn0(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetForwardDir(), childWorldTransform.GetColumn1(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetUpDir(), childWorldTransform.GetColumn2(), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetWorldAngles(), Ang3(pParentEntity->GetWorldRotation() * childParams.qRotation), VEC_EPSILON);

		// Check world-space scale
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetWorldScale(), pParentEntity->GetWorldScale().CompMul(childParams.vScale), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetWorldTM().GetScale(), pParentEntity->GetWorldScale().CompMul(childParams.vScale), VEC_EPSILON);
		CRY_TEST_CHECK_CLOSE(pChildEntity->GetWorldTM(), pParentEntity->GetWorldTM() * Matrix34::Create(childParams.vScale, childParams.qRotation, childParams.vPosition), VEC_EPSILON);

		// Check local-space
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetPos(), childParams.vPosition);
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetRotation(), childParams.qRotation);
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetScale(), childParams.vScale);
		CRY_TEST_CHECK_EQUAL(pChildEntity->GetLocalTM(), Matrix34::Create(childParams.vScale, childParams.qRotation, childParams.vPosition));

		g_pIEntitySystem->RemoveEntity(pParentEntity->GetId());
		g_pIEntitySystem->RemoveEntity(pChildEntity->GetId());
	}

	CRY_TEST(TestDefaultEntitySpawnClass)
	{
		SEntitySpawnParams spawnParams;
		spawnParams.guid = CryGUID::Create();
		spawnParams.sName = __FUNCTION__;
		spawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY;

		CEntity* const pEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(spawnParams));
		CRY_TEST_ASSERT(pEntity != nullptr);
		CRY_TEST_ASSERT(pEntity->GetClass() != nullptr);
		CRY_TEST_ASSERT(pEntity->GetClass() == g_pIEntitySystem->GetClassRegistry()->GetDefaultClass());
	}

	CRY_TEST(TestComponentShutdownOrder)
	{
		bool firstComponentDestroyed = false;
		bool secondComponentDestroyed = false;

		class CFirstComponent : public IEntityComponent
		{
			bool&       wasDestroyed;
			const bool& wasOtherDestroyed;

		public:
			CFirstComponent(bool& destroyed, bool& otherDestroyed) : wasDestroyed(destroyed), wasOtherDestroyed(otherDestroyed) {}
			virtual ~CFirstComponent()
			{
				CRY_TEST_ASSERT(!wasDestroyed);
				CRY_TEST_ASSERT(wasOtherDestroyed);
				wasDestroyed = true;
			}
		};

		class CSecondComponent : public IEntityComponent
		{
			bool&       wasDestroyed;
			const bool& wasOtherDestroyed;

		public:
			CSecondComponent(bool& destroyed, bool& otherDestroyed) : wasDestroyed(destroyed), wasOtherDestroyed(otherDestroyed) {}
			virtual ~CSecondComponent()
			{
				CRY_TEST_ASSERT(!wasDestroyed);
				CRY_TEST_ASSERT(!wasOtherDestroyed);
				wasDestroyed = true;
			}
		};

		SEntitySpawnParams spawnParams;
		spawnParams.guid = CryGUID::Create();
		spawnParams.sName = __FUNCTION__;
		spawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY;

		CEntity* const pEntity = static_cast<CEntity*>(g_pIEntitySystem->SpawnEntity(spawnParams));
		CRY_TEST_ASSERT(pEntity != nullptr);

		pEntity->CreateComponentClass<CFirstComponent>(firstComponentDestroyed, secondComponentDestroyed);
		CRY_TEST_ASSERT(!firstComponentDestroyed && !secondComponentDestroyed);
		pEntity->CreateComponentClass<CSecondComponent>(secondComponentDestroyed, firstComponentDestroyed);
		CRY_TEST_ASSERT(!firstComponentDestroyed && !secondComponentDestroyed);

		g_pIEntitySystem->RemoveEntity(pEntity->GetId(), true);
		CRY_TEST_ASSERT(firstComponentDestroyed && secondComponentDestroyed);
	}

	CRndGen MathRand;

	class CEntityTimerAccuracyTestCommand : public ISimpleEntityEventListener
	{
	public:

		explicit CEntityTimerAccuracyTestCommand()
		{
			m_expectedDuration.SetMilliSeconds(MathRand.GetRandom(50, 1500));
		}

		virtual void ProcessEvent(const SEntityEvent& event) override
		{
			if (event.event == ENTITY_EVENT_TIMER)
				m_isCalled = true;
		}

		bool Update()
		{
			if (!m_pTimerEntity)
				Start();

			if (m_isCalled)
			{
				const CTimeValue duration = gEnv->pTimer->GetFrameStartTime() - m_startTime;

				//To be replaced with direct CTimeValue multiplication after it is supported
				CRY_TEST_ASSERT(duration.GetSeconds() > m_expectedDuration.GetSeconds() * 0.8f && duration.GetSeconds() < m_expectedDuration.GetSeconds() * 1.2f,
					"duration: %f, expected: %f", duration.GetSeconds(), m_expectedDuration.GetSeconds());
			}
			return m_isCalled;
		}

		IEntity* m_pTimerEntity = nullptr;
		bool m_isCalled = false;
		CTimeValue m_startTime;
		CTimeValue m_expectedDuration;

		void Start()
		{
			SEntitySpawnParams spawnParams;
			m_pTimerEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);
			m_pTimerEntity->SetTimer(this, m_pTimerEntity->GetId(), CryGUID(), 0, static_cast<int>(m_expectedDuration.GetMilliSeconds()));
			m_startTime = gEnv->pTimer->GetFrameStartTime();
		}
	};

	CRY_TEST(EntityTimerTest, timeout = 60, editor=false)
	{
		commands = {
			CryTest::CCommandLoadLevel("woodland"),
			CryTest::CCommandRepeat(10, CEntityTimerAccuracyTestCommand()),
		};
	}

	//////////////////////////////////////////////////////////////////////
	class CTestLinkComponent : public IEntityComponent
	{
	public:
		void ProcessEvent(const SEntityEvent& event) override
		{
			if (event.event == Cry::Entity::EEvent::LinkAdded)
			{
				IEntityLink* pLink = (IEntityLink*)event.nParam[0];

				CRY_TEST_CHECK_DIFFERENT(pLink, nullptr);
				CRY_TEST_CHECK_EQUAL(m_pLinkedEntity->GetId(), pLink->entityId);

				m_isCalled = true;
			}
		}

		virtual Cry::Entity::EventFlags GetEventMask() const override
		{
			return Cry::Entity::EEvent::LinkAdded;
		}

		void SetLinkedEntity(IEntity* pEntity)
		{
			m_pLinkedEntity = pEntity;
		}

	public:
		bool m_isCalled = false;
		IEntity* m_pLinkedEntity = nullptr;
	};

	void TestEntityLink()
	{
		IEntity* pFirstEntity = nullptr;
		IEntity* pSecondEntity = nullptr;
		CTestLinkComponent* pComponent = nullptr;
		string linkString = "Test Link";

		SEntitySpawnParams spawnParams;
		pFirstEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);
		pComponent = pFirstEntity->CreateComponentClass<CTestLinkComponent>();

		SEntitySpawnParams spawnParams2;
		pSecondEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams2);
		pComponent->SetLinkedEntity(pSecondEntity);

		pFirstEntity->AddEntityLink(linkString.c_str(), pSecondEntity->GetId(), pSecondEntity->GetGuid());

		IEntityLink* pLink = pFirstEntity->GetEntityLinks();

		CRY_TEST_CHECK_DIFFERENT(pLink, nullptr);
		CRY_TEST_CHECK_EQUAL(pLink->name, linkString);
		CRY_TEST_CHECK_EQUAL(pLink->entityId, pSecondEntity->GetId());

		CRY_TEST_ASSERT(pComponent->m_isCalled);

		// CleanUp
		g_pIEntitySystem->RemoveEntity(pFirstEntity->GetId());
		g_pIEntitySystem->RemoveEntity(pSecondEntity->GetId());
	}


	CRY_TEST(EntityLinkTest, timeout = 60)
	{
		commands = {
			TestEntityLink,
		};
	}
	//////////////////////////////////////////////////////////////////////

	namespace TestEventPriority
	{
		int previousPriority = 0;
		int componentCounter = 0;
		const int numberOfComponents = 20;
	}

	class TestPrioComponent1 : public IEntityComponent
	{
	public:
		int m_priority = 1;

		virtual void Initialize() override {}

		virtual ComponentEventPriority GetEventPriority() const override { return m_priority; }
		virtual Cry::Entity::EventFlags GetEventMask() const override { return Cry::Entity::EEvent::Update; }

		virtual void ProcessEvent(const SEntityEvent& event) override
		{
			CRY_TEST_ASSERT(TestEventPriority::previousPriority <= m_priority);
			TestEventPriority::previousPriority = m_priority;

			TestEventPriority::componentCounter++;
		}
	};

	class TestPrioComponent2 : public IEntityComponent
	{
	public:
		int m_priority = 13;

		virtual ComponentEventPriority GetEventPriority() const override { return m_priority; }
		virtual Cry::Entity::EventFlags GetEventMask() const override { return Cry::Entity::EEvent::Update; }

		virtual void ProcessEvent(const SEntityEvent& event) override
		{
			CRY_TEST_ASSERT(TestEventPriority::previousPriority <= m_priority);
			TestEventPriority::previousPriority = m_priority;

			TestEventPriority::componentCounter++;
		}
	};

	class TestPrioComponent3 : public IEntityComponent
	{
	public:
		int m_priority = 59;

		virtual ComponentEventPriority GetEventPriority() const override { return m_priority; }
		virtual Cry::Entity::EventFlags GetEventMask() const override { return Cry::Entity::EEvent::Update; }

		virtual void ProcessEvent(const SEntityEvent& event) override
		{
			CRY_TEST_ASSERT(TestEventPriority::previousPriority <= m_priority);
			TestEventPriority::previousPriority = m_priority;

			TestEventPriority::componentCounter++;
			if (TestEventPriority::numberOfComponents == TestEventPriority::componentCounter)
			{
				TestEventPriority::componentCounter = 0;
				TestEventPriority::previousPriority = 0;
			}
		}
	};

	CRY_TEST_FIXTURE(CTestComponentEventPriority)
	{
		virtual void Done() override
		{
			g_pIEntitySystem->RemoveEntity(m_pEntity->GetId());
		}

		void Start()
		{
			SEntitySpawnParams params;
			m_pEntity = g_pIEntitySystem->SpawnEntity(params);
			AddRandomComponents(m_pEntity, TestEventPriority::numberOfComponents);
		}

		void RemoveAndReAddComponents()
		{
			m_pEntity->RemoveAllComponents();

			AddRandomComponents(m_pEntity, TestEventPriority::numberOfComponents);
			TestEventPriority::componentCounter = 0;
		}

		void AddRandomComponents(IEntity* pEntity, int count)
		{
			for (int i = 0; i < count; ++i)
			{
				int rnd = cry_random(0, 2);
				if (rnd == 0)
					pEntity->CreateComponentClass<TestPrioComponent1>();
				else if (rnd == 1)
					pEntity->CreateComponentClass<TestPrioComponent2>();
				else
					pEntity->CreateComponentClass<TestPrioComponent3>();
			}
		}

		IEntity* m_pEntity = nullptr;
	};

	CRY_TEST_WITH_FIXTURE(EventPriority, CTestComponentEventPriority, editor = false, timeout = 20)
	{
		commands =
		{
			CryTest::CCommandLoadLevel("woodland"),
			CryTest::CCommandFunction(this, &CTestComponentEventPriority::Start),
			CryTest::CCommandFunction(this, &CTestComponentEventPriority::RemoveAndReAddComponents),
			CryTest::CCommandWait(4.f),
			CryTest::CCommandFunction(this, &CTestComponentEventPriority::RemoveAndReAddComponents),
			CryTest::CCommandWait(4.f),
		};
	}

	void CheckLoadedEntities()
	{
		// Find the entity in the "NotLoadedInGame" layer
		// This is exported but should not be visible by default
		{
			IEntity* pNotLoadedEntity = gEnv->pEntitySystem->FindEntityByName("NotLoadedEntity");
			CRY_TEST_ASSERT(pNotLoadedEntity != nullptr && pNotLoadedEntity->IsHidden());
		}

		// Find the entity in the "LoadedInGameFromLayerActivation" layer
		// This is not loaded by default, but should be made visible from FG thus visible
		{
			IEntity* pLoadedFromFGEntity = gEnv->pEntitySystem->FindEntityByName("LoadedInGameFromFG");
			CRY_TEST_ASSERT(pLoadedFromFGEntity != nullptr && !pLoadedFromFGEntity->IsHidden());
		}

		// Finally, find the entity in the "LoadedInGame" layer
		// This is loaded by default
		{
			IEntity* pLoadedFromLevelEntity = gEnv->pEntitySystem->FindEntityByName("LoadedInGameByDefault");
			CRY_TEST_ASSERT(pLoadedFromLevelEntity != nullptr && !pLoadedFromLevelEntity->IsHidden());
		}
	}

	CRY_TEST(EntityLayerActivation, editor = false, timeout = 20)
	{
		commands =
		{
			CryTest::CCommandLoadLevel("EntityFeatureTests"),
			// Give time for flowgraph controlled layer activation to be invoked
			CryTest::CCommandWait(4.f),
			// Now check if the correct layers (and the entities inside them) were activated
			CryTest::CCommand(CheckLoadedEntities),
		};
	}

	//////////////////////////////////////////////////////////////////////////

	class CChildTestComponent : public IEntityComponent
	{
	public:
		CChildTestComponent() {}
		~CChildTestComponent() {}

		virtual Cry::Entity::EventFlags GetEventMask() const override { return Cry::Entity::EEvent::TransformChanged; }

		virtual void ProcessEvent(const SEntityEvent& event) override
		{
			if (event.event == Cry::Entity::EEvent::TransformChanged)
			{
				m_TransformChangeCount++;
			}
		}

		int m_TransformChangeCount = 0;
	};

	class CTestEntityAttachment
	{
	public:
		CTestEntityAttachment() {}
		~CTestEntityAttachment() {}

		bool Update()
		{
			if (!pParentEntity)
				Start();

			// Generate a random position, rotation and scale
			Vec3 pos(cry_random(-100.f, 100.f), cry_random(-100.f, 100.f), cry_random(-100.f, 100.f));
			Quat rot = Quat::CreateRotationAA(cry_random(-100.f, 100.f), Vec3(1.f,0.f,0.f));
			Vec3 scale(cry_random(0.1f, 10.f), cry_random(0.1f, 10.f), cry_random(0.1f, 10.f));
			
			pParentEntity->SetPosRotScale(pos, rot, scale);
			
			CheckTransform();
			if (steps != 20)
			{
				steps++;
				return false;
			}
			else
			{
				gEnv->pEntitySystem->RemoveEntity(pParentEntity->GetId());

				return true;
			}
		}

		void CheckTransform()
		{
			// The transformation of the parent and child should be equal, because they were spawn with the same transformation
			CRY_TEST_CHECK_EQUAL(pParentEntity->GetWorldTM(), pChildEntity->GetWorldTM());
		}

		void Start()
		{
			SEntitySpawnParams spawnParams;
			pParentEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);

			SEntitySpawnParams spawnParams2;
			pChildEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams2);
			pChildComponent = pChildEntity->CreateComponentClass<CChildTestComponent>();

			pParentEntity->AttachChild(pChildEntity);

			CRY_TEST_CHECK_EQUAL(pParentEntity->GetChildCount(), 1);
			CRY_TEST_CHECK_DIFFERENT(pParentEntity->GetChild(0), nullptr);
			CRY_TEST_CHECK_DIFFERENT(pChildEntity->GetParent(), nullptr);

			pParentEntity->SetPos(Vec3(0, 0, 0));

			// We should already got multiple transform events at this point
			CRY_TEST_CHECK_DIFFERENT(pChildComponent->m_TransformChangeCount, 0);
		}

		IEntity* pParentEntity = nullptr;
		IEntity* pChildEntity = nullptr;
		CChildTestComponent* pChildComponent = nullptr;
		int steps = 0;
	};

	CRY_TEST(TestEntityAttachment, timeout = 60)
	{
		commands = {
			CryTest::CCommand(CTestEntityAttachment()),
		};
	}

}

void RegisterUnitTestComponents(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(EntityTestsSuit::CUnifiedEntityComponent));
		scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(EntityTestsSuit::CComponent1));
		scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(EntityTestsSuit::CComponent2));
	}
}

#endif // #ifdef CRY_TESTING