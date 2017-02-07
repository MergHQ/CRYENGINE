#include "StdAfx.h"
#include "Entity.h"

#include "MonoRuntime.h"

#include "NativeComponents/ManagedEntityComponent.h"

#include "Wrappers/MonoDomain.h"
#include "Wrappers/MonoClass.h"

#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

static std::map<MonoReflectionType*, CManagedEntityComponentFactory> s_entityComponentFactoryMap;

static void RegisterComponent(MonoReflectionType* pType, uint64 guidHipart, uint64 guidLopart)
{
	MonoClass* pMonoClass = mono_type_get_class(mono_reflection_type_get_type(pType));
	MonoImage* pImage = mono_class_get_image(pMonoClass);

	auto* pRuntime = static_cast<CMonoRuntime*>(gEnv->pMonoRuntime);
	auto* pDomain = pRuntime->FindDomainByHandle(mono_object_get_domain((MonoObject*)pType));

	auto* pLibrary = pDomain->GetLibraryFromMonoAssembly(mono_image_get_assembly(pImage));

	auto id = CryGUID::Construct(guidHipart, guidLopart);
	s_entityComponentFactoryMap.emplace(pType, CManagedEntityComponentFactory(pLibrary->GetClassFromMonoClass(pMonoClass), id));
}

static IEntityComponent* CreateManagedComponent(IEntity *pEntity, SEntitySpawnParams& params, void* pUserData)
{
	for (auto it = s_entityComponentFactoryMap.begin(); it != s_entityComponentFactoryMap.end(); ++it)
	{
		if (it->second.GetEntityClass() == params.pClass)
		{
			return pEntity->AddComponent(it->second.GetId(), std::make_shared<CManagedEntityComponent>(it->second), false);
		}
	}

	return nullptr;
}

static void RegisterManagedEntityWithDefaultComponent(MonoString* pName, MonoString* pEditorCategory, MonoString* pEditorHelper, MonoString* pEditorIcon, bool bHide, MonoReflectionType* pComponentType)
{
	auto it = s_entityComponentFactoryMap.find(pComponentType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = mono_string_to_utf8(pName);

	clsDesc.editorClassInfo.sCategory = mono_string_to_utf8(pEditorCategory);
	clsDesc.editorClassInfo.sHelper = mono_string_to_utf8(pEditorHelper);
	clsDesc.editorClassInfo.sIcon = mono_string_to_utf8(pEditorIcon);

	clsDesc.pUserProxyCreateFunc = &CreateManagedComponent;

	if (IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(clsDesc))
	{
		it->second.SetEntityClass(pEntityClass);
	}
}

static MonoObject* GetComponent(IEntity* pEntity, MonoReflectionType* pType)
{
	auto it = s_entityComponentFactoryMap.find(pType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	if (auto* pComponent = static_cast<CManagedEntityComponent*>(pEntity->GetComponentByTypeId(it->second.GetId())))
	{
		return (MonoObject*)pComponent->GetObject()->GetHandle();
	}

	return nullptr;
}

static MonoObject* GetOrCreateComponent(IEntity* pEntity, MonoReflectionType* pType)
{
	auto it = s_entityComponentFactoryMap.find(pType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	CManagedEntityComponent* pComponent = static_cast<CManagedEntityComponent*>(pEntity->GetComponentByTypeId(it->second.GetId()));
	if (pComponent == nullptr)
	{
		pComponent = static_cast<CManagedEntityComponent*>(pEntity->AddComponent(it->second.GetId(), std::make_shared<CManagedEntityComponent>(it->second), false));
	}

	return static_cast<MonoObject*>(pComponent->GetObject()->GetHandle());
}

static void RegisterComponentProperty(MonoReflectionType* pComponentType, MonoReflectionProperty* pProperty, MonoString* pPropertyName, MonoString* pPropertyLabel, MonoString* pPropertyDescription, EEntityPropertyType type)
{
	auto it = s_entityComponentFactoryMap.find(pComponentType);
	CRY_ASSERT(it != s_entityComponentFactoryMap.end());

	it->second.AddProperty((MonoReflectionPropertyInternal*)pProperty, mono_string_to_utf8(pPropertyName), mono_string_to_utf8(pPropertyLabel), mono_string_to_utf8(pPropertyDescription), type);
}

void CManagedEntityInterface::RegisterFunctions(std::function<void(const void* pMethod, const char* methodName)> func)
{
	func(RegisterComponent, "RegisterComponent");
	func(RegisterManagedEntityWithDefaultComponent, "RegisterEntityWithDefaultComponent");
	func(GetOrCreateComponent, "GetComponent");
	func(GetOrCreateComponent, "GetOrCreateComponent");
	func(RegisterComponentProperty, "RegisterComponentProperty");
}