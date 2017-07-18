// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Entity.h"
#include "EntitySystem.h"

#include <CrySystem/CryUnitTest.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>

#include "SubstitutionProxy.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>

CRY_UNIT_TEST_SUITE(EntityTestsSuit)
{
	struct IUnifiedEntityComponent : public IEntityComponent
	{
		static CryGUID& IID()
		{
			static CryGUID id = "{C89DD0DD-9850-43BA-BF52-86EFB7C9D0A5}"_cry_guid;
			return id;
		}

		static void ReflectType(Schematyc::CTypeDesc<IUnifiedEntityComponent>& desc)
		{
			desc.SetGUID(cryiidof<IUnifiedEntityComponent>());
		}
	};

	class CUnifiedEntityComponent : public IUnifiedEntityComponent
	{
	public:
		static CryGUID& IID()
		{
			static CryGUID id = "{C89DD0DD-9850-43BA-BF52-86EFB7C9D0A5}"_cry_guid;
			return id;
		}

		static void Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
		}

		static void ReflectType(Schematyc::CTypeDesc<CUnifiedEntityComponent>& desc)
		{
			desc.AddBase<IUnifiedEntityComponent>();
			desc.SetGUID(CUnifiedEntityComponent::IID());
			desc.SetEditorCategory("Unit Tests");
			desc.SetLabel("Unified Entity Component");
			desc.SetDescription("Does stuff");
			desc.SetIcon("icons:ObjectTypes/light.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CUnifiedEntityComponent::m_bMyBool, 'bool', "Bool", "Bool", "Bool desc", false);
			desc.AddMember(&CUnifiedEntityComponent::m_myFloat, 'floa', "Float", "Float", "Float desc", 0.f);
		}

		bool m_bMyBool = false;
		float m_myFloat = 0.f;
	};

	IEntity* SpawnTestEntity(const char* szName)
	{
		SEntitySpawnParams params;
		params.guid = CryGUID::Create();
		params.sName = szName;
		params.nFlags = ENTITY_FLAG_CLIENT_ONLY;
		params.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

		IEntity *pEntity = gEnv->pEntitySystem->SpawnEntity(params);
		CRY_UNIT_TEST_ASSERT(pEntity != NULL);
		return pEntity;
	}

	CRY_UNIT_TEST(SpawnTest)
	{
		IEntity *pEntity = SpawnTestEntity("TestEntity");
		EntityId id = pEntity->GetId();

		CRY_UNIT_TEST_ASSERT( pEntity->GetGuid() != CryGUID::Null() );

		CRY_UNIT_TEST_CHECK_EQUAL(id, gEnv->pEntitySystem->FindEntityByGuid(pEntity->GetGuid()));
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity, gEnv->pEntitySystem->FindEntityByName("TestEntity"));

		// Test Entity components
		IEntitySubstitutionComponent *pComponent = pEntity->GetOrCreateComponent<IEntitySubstitutionComponent>();
		CRY_UNIT_TEST_ASSERT( nullptr != pComponent );
		CRY_UNIT_TEST_ASSERT( 1 == pEntity->GetComponentsCount() );
	}

	class CLegacyEntityComponent : public IEntityComponent
	{
	public:
		CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CLegacyEntityComponent, "LegacyEntityComponent", 0xB1FEBCEEBE1246A9, 0xB69ACC66D6C0D3E0);
	};

	CRYREGISTER_CLASS(CLegacyEntityComponent);

	CRY_UNIT_TEST(CreateLegacyComponent)
	{
		IEntity *pEntity = SpawnTestEntity("LegacyComponentTestEntity");

		CLegacyEntityComponent *pComponent = pEntity->GetOrCreateComponent<CLegacyEntityComponent>();
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 1);

		pEntity->RemoveComponent(pComponent);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 0);
	}

	struct ILegacyComponentInterface : public IEntityComponent
	{
		CRY_ENTITY_COMPONENT_INTERFACE(ILegacyComponentInterface, 0xD9A710128AF14426, 0x883599C4F4EB987D)

		virtual bool IsValid() const { return false; }
	};

	class CLegacyEntityComponentWithInterface final : public ILegacyComponentInterface
	{
	public:
		CRY_ENTITY_COMPONENT_CLASS(CLegacyEntityComponentWithInterface, ILegacyComponentInterface, "LegacyEntityComponentWithInterface", 0x34EE1B9221544BAD, 0xB69ACC66D6C0D3E0);

		virtual bool IsValid() const final { return true; }
	};

	CRYREGISTER_CLASS(CLegacyEntityComponentWithInterface);

	CRY_UNIT_TEST(CreateLegacyComponentWithInterface)
	{
		IEntity *pEntity = SpawnTestEntity("LegacyComponentTestInterfaceEntity");

		ILegacyComponentInterface *pComponent = pEntity->GetOrCreateComponent<ILegacyComponentInterface>();
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 1);
		CRY_UNIT_TEST_ASSERT(pComponent->IsValid());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<CLegacyEntityComponentWithInterface>());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<ILegacyComponentInterface>());

		pEntity->RemoveComponent(pComponent);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 0);

		pComponent = pEntity->GetOrCreateComponent<CLegacyEntityComponentWithInterface>();
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 1);
		CRY_UNIT_TEST_ASSERT(pComponent->IsValid());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<CLegacyEntityComponentWithInterface>());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<ILegacyComponentInterface>());

		DynArray<CLegacyEntityComponentWithInterface*> components;
		pEntity->GetAllComponents<CLegacyEntityComponentWithInterface>(components);
		CRY_UNIT_TEST_CHECK_EQUAL(components.size(), 1);
		CRY_UNIT_TEST_CHECK_EQUAL(components.at(0), static_cast<CLegacyEntityComponentWithInterface*>(pComponent));

		DynArray<ILegacyComponentInterface*> componentsbyInterface;
		pEntity->GetAllComponents<ILegacyComponentInterface>(componentsbyInterface);
		CRY_UNIT_TEST_CHECK_EQUAL(componentsbyInterface.size(),  1);
		CRY_UNIT_TEST_CHECK_EQUAL(componentsbyInterface.at(0), static_cast<CLegacyEntityComponentWithInterface*>(pComponent));
	}

	CRY_UNIT_TEST(CreateUnifiedComponent)
	{
		IEntity *pEntity = SpawnTestEntity("UnifiedComponentTestEntity");

		CUnifiedEntityComponent *pComponent = pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 1);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<CUnifiedEntityComponent>());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<IUnifiedEntityComponent>());

		pEntity->RemoveComponent(pComponent);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 0);

		pComponent = static_cast<CUnifiedEntityComponent*>(pEntity->GetOrCreateComponent<IUnifiedEntityComponent>());
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 1);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<CUnifiedEntityComponent>());
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent, pEntity->GetComponent<IUnifiedEntityComponent>());

		DynArray<CUnifiedEntityComponent*> components;
		pEntity->GetAllComponents<CUnifiedEntityComponent>(components);
		CRY_UNIT_TEST_CHECK_EQUAL(components.size(), 1);
		CRY_UNIT_TEST_CHECK_EQUAL(components.at(0), static_cast<CUnifiedEntityComponent*>(pComponent));

		DynArray<IUnifiedEntityComponent*> componentsbyInterface;
		pEntity->GetAllComponents<IUnifiedEntityComponent>(componentsbyInterface);
		CRY_UNIT_TEST_CHECK_EQUAL(componentsbyInterface.size(), 1);
		CRY_UNIT_TEST_CHECK_EQUAL(componentsbyInterface.at(0), static_cast<CUnifiedEntityComponent*>(pComponent));
	}

	CRY_UNIT_TEST(UnifiedComponentSerialization)
	{
		XmlNodeRef node = gEnv->pSystem->CreateXmlNode();
		CryGUID instanceGUID;

		{
			IEntity *pEntity = SpawnTestEntity("UnifiedComponentSerializationTestEntity");

			CUnifiedEntityComponent *pComponent = pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
			CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
			// Check default values
			CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_bMyBool, false);
			CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_myFloat, 0.f);

			pComponent->m_bMyBool = true;
			pComponent->m_myFloat = 1337.f;

			instanceGUID = pComponent->GetGUID();

			// Save to XML
			pEntity->SerializeXML(node, false);
		}

		// Create another entity for deserialization
		IEntity *pDeserializedEntity = SpawnTestEntity("UnifiedComponentDeserializationTestEntity");
		// Deserialize
		pDeserializedEntity->SerializeXML(node, true);

		CUnifiedEntityComponent *pComponent = pDeserializedEntity->GetComponent<CUnifiedEntityComponent>();
		CRY_UNIT_TEST_CHECK_DIFFERENT(pComponent, nullptr);
		// Check deserialized values
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_bMyBool, true);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->m_myFloat, 1337.f);
		CRY_UNIT_TEST_CHECK_EQUAL(pComponent->GetGUID(), instanceGUID);
	}

	CRY_UNIT_TEST(QueryInvalidGUID)
	{
		IEntity *pEntity = SpawnTestEntity("UnifiedComponentTestEntity");
		CUnifiedEntityComponent *pComponent = pEntity->GetOrCreateComponent<CUnifiedEntityComponent>();
		CLegacyEntityComponentWithInterface* pLegacyComponent = pEntity->GetOrCreateComponent<CLegacyEntityComponentWithInterface>();
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponentsCount(), 2);
		// Querying the lowest level GUIDs is disallowed
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity->GetComponent<IEntityComponent>(), nullptr);
	}
}

void RegisterUnitTestComponents(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(EntityTestsSuit::CUnifiedEntityComponent));
		EntityTestsSuit::CUnifiedEntityComponent::Register(componentScope);
	}
}