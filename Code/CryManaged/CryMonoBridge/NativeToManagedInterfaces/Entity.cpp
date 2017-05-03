#include "StdAfx.h"
#include "Entity.h"

#include "MonoRuntime.h"

#include "NativeComponents/ManagedEntityComponent.h"

#include "Wrappers/MonoDomain.h"
#include "Wrappers/MonoClass.h"

static std::unordered_map<MonoInternals::MonoReflectionType*, std::unique_ptr<SManagedEntityComponentFactory>> s_entityComponentFactoryMap;

static void RegisterComponent(MonoInternals::MonoReflectionType* pType, uint64 guidHipart, uint64 guidLopart)
{
	MonoInternals::MonoClass* pMonoClass = MonoInternals::mono_type_get_class(MonoInternals::mono_reflection_type_get_type(pType));
	MonoInternals::MonoImage* pImage = MonoInternals::mono_class_get_image(pMonoClass);

	CMonoDomain* pDomain = GetMonoRuntime()->FindDomainByHandle(MonoInternals::mono_object_get_domain((MonoInternals::MonoObject*)pType));

	CMonoLibrary* pLibrary = pDomain->GetLibraryFromMonoAssembly(MonoInternals::mono_image_get_assembly(pImage));

	CryGUID id = CryGUID::Construct(guidHipart, guidLopart);

	s_entityComponentFactoryMap.emplace(pType, stl::make_unique<SManagedEntityComponentFactory>(pLibrary->GetClassFromMonoClass(pMonoClass), id));
	
	ICryFactoryRegistryImpl* pFactoryRegistry = static_cast<ICryFactoryRegistryImpl*>(gEnv->pSystem->GetCryFactoryRegistry());

	SRegFactoryNode node;
	node.m_pFactory = s_entityComponentFactoryMap[pType].get();
	node.m_pNext = nullptr;
	pFactoryRegistry->RegisterFactories(&node);
}

static IEntityComponent* CreateManagedComponent(IEntity *pEntity, SEntitySpawnParams& params, void* pUserData)
{
	return pEntity->AddComponent(*(const CryClassID*)pUserData, nullptr, true);
}

static void RegisterManagedEntityWithDefaultComponent(MonoInternals::MonoString* pName, MonoInternals::MonoString* pEditorCategory, MonoInternals::MonoString* pEditorHelper, MonoInternals::MonoString* pEditorIcon, bool bHide, MonoInternals::MonoReflectionType* pComponentType)
{
	auto it = s_entityComponentFactoryMap.find(pComponentType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	std::shared_ptr<CMonoString> pClassName = CMonoDomain::CreateString(pName);
	std::shared_ptr<CMonoString> pCategory = CMonoDomain::CreateString(pEditorCategory);
	std::shared_ptr<CMonoString> pHelper = CMonoDomain::CreateString(pEditorHelper);
	std::shared_ptr<CMonoString> pIcon = CMonoDomain::CreateString(pEditorIcon);

	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = pClassName->GetString();

	clsDesc.editorClassInfo.sCategory = pCategory->GetString();
	clsDesc.editorClassInfo.sHelper = pHelper->GetString();
	clsDesc.editorClassInfo.sIcon = pIcon->GetString();

	clsDesc.pUserProxyCreateFunc = &CreateManagedComponent;
	clsDesc.pUserProxyData = (void*)&it->second->GetClassID();

	it->second->pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(clsDesc);
}

static MonoInternals::MonoObject* GetComponent(IEntity* pEntity, MonoInternals::MonoReflectionType* pType)
{
	auto it = s_entityComponentFactoryMap.find(pType);

	if (it == s_entityComponentFactoryMap.end())
	{
		return nullptr;
	}

	if (CManagedEntityComponent* pComponent = static_cast<CManagedEntityComponent*>(pEntity->GetComponentByTypeId(it->second->GetClassID())))
	{
		return pComponent->GetObject()->GetManagedObject();
	}

	return nullptr;
}

static MonoInternals::MonoObject* AddComponent(IEntity* pEntity, MonoInternals::MonoReflectionType* pType)
{
	auto it = s_entityComponentFactoryMap.find(pType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	if (CManagedEntityComponent* pComponent = static_cast<CManagedEntityComponent*>(pEntity->AddComponent(it->second->GetClassID(), nullptr, true)))
	{
		return pComponent->GetObject()->GetManagedObject();
	}

	return nullptr;
}

static MonoInternals::MonoObject* GetOrCreateComponent(IEntity* pEntity, MonoInternals::MonoReflectionType* pType)
{
	auto it = s_entityComponentFactoryMap.find(pType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	CManagedEntityComponent* pComponent = static_cast<CManagedEntityComponent*>(pEntity->GetComponentByTypeId(it->second->GetClassID()));
	if (pComponent == nullptr)
	{
		pComponent = static_cast<CManagedEntityComponent*>(pEntity->AddComponent(it->second->GetClassID(), nullptr, true));
		if (pComponent == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to create component %s!", it->second->GetName());

			return nullptr;
		}
	}

	return pComponent->GetObject()->GetManagedObject();
}

static void RegisterComponentProperty(MonoInternals::MonoReflectionType* pComponentType, MonoInternals::MonoReflectionProperty* pProperty, MonoInternals::MonoString* pPropertyName, MonoInternals::MonoString* pPropertyLabel, MonoInternals::MonoString* pPropertyDescription, EEntityPropertyType type)
{
	auto it = s_entityComponentFactoryMap.find(pComponentType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	std::shared_ptr<CMonoString> pName = CMonoDomain::CreateString(pPropertyName);
	std::shared_ptr<CMonoString> pLabel = CMonoDomain::CreateString(pPropertyLabel);
	std::shared_ptr<CMonoString> pDescription = CMonoDomain::CreateString(pPropertyDescription);

	it->second->AddProperty((MonoReflectionPropertyInternal*)pProperty, pName->GetString(), pLabel->GetString(), pDescription->GetString(), type);
}

void CManagedEntityInterface::RegisterFunctions(std::function<void(const void* pMethod, const char* methodName)> func)
{
	func(RegisterComponent, "RegisterComponent");
	func(RegisterManagedEntityWithDefaultComponent, "RegisterEntityWithDefaultComponent");
	func(GetComponent, "GetComponent");
	func(GetOrCreateComponent, "GetOrCreateComponent");
	func(AddComponent, "AddComponent");
	func(RegisterComponentProperty, "RegisterComponentProperty");
}