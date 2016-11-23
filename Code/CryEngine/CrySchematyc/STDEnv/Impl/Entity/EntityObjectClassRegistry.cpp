// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectClassRegistry.h"

#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUserData.h>

#include "AutoRegister.h"
#include "STDEnv.h"
#include "STDModules.h"
#include "Entity/EntityClassProperties.h"
#include "Entity/EntityObjectMap.h"
#include "Entity/EntityObjectPreviewer.h"
#include "Entity/EntityComponent.h"
#include "Entity/Components/EntityDebugComponent.h"
#include "Entity/Components/EntityMovementComponent.h"

namespace Schematyc
{
namespace
{
IEntityComponent* CreateSchematycEntityComponent(IEntity* pEntity, SEntitySpawnParams& spawnParams, void* pUserData)
{
	if (auto* pEntityComponent = pEntity->CreateComponentClass<Schematyc::CEntityObjectComponent>())
	{
		auto *pEntityUserData = (SEntityUserData *)spawnParams.pUserData;

		const IRuntimeClass* pRuntimeClass = CSTDEnv::GetInstance().GetEntityObjectClassRegistry().GetRuntimeClassFromEntityClass(spawnParams.pClass->GetName());

		pEntityComponent->SetRuntimeClass(pRuntimeClass, pEntityUserData != nullptr ? pEntityUserData->bIsPreview : false);
		return pEntityComponent;
	}

	return nullptr;
}
} // Anonymous

CEntityObjectClassRegistry::SEntityClass::SEntityClass()
{
	// N.B. This is a quick work around for the problem of entities without script tables not triggering areas.
	desc.pScriptTable = gEnv->pScriptSystem->CreateTable();
	desc.pScriptTable->AddRef();
}

CEntityObjectClassRegistry::SEntityClass::SEntityClass(const SEntityClass& rhs)
	: editorCategory(rhs.editorCategory)
	, icon(rhs.icon)
	, desc(rhs.desc)
{
	desc.pScriptTable->AddRef();
}

void CEntityObjectClassRegistry::Init()
{
	gEnv->pSchematyc->GetCompiler().GetClassCompilationSignalSlots().Connect(Delegate::Make(*this, &CEntityObjectClassRegistry::OnClassCompilation), m_connectionScope);  // TODO : Can we filter by class guid?
}

const IRuntimeClass* CEntityObjectClassRegistry::GetRuntimeClassFromEntityClass(const char* entityClass) const
{
	auto entityClassIt = m_entityClasses.find(entityClass);
	if (entityClassIt != m_entityClasses.end())
	{
		return gEnv->pSchematyc->GetRuntimeRegistry().GetClass(entityClassIt->second.runtimeClassGUID).get();
	}

	return nullptr;
}

CEntityObjectClassRegistry::SEntityClass::~SEntityClass()
{
	desc.pScriptTable->Release();
}

void CEntityObjectClassRegistry::OnClassCompilation(const IRuntimeClass& runtimeClass)
{
	if (runtimeClass.GetEnvClassGUID() == g_entityClassGUID)
	{
		const char* szClassName = runtimeClass.GetName();
		EntityClasses::iterator itEntityClass = m_entityClasses.find(szClassName);
		const bool bNewEntityClass = itEntityClass == m_entityClasses.end();
		if (bNewEntityClass)
		{
			itEntityClass = m_entityClasses.insert(EntityClasses::value_type(szClassName, SEntityClass())).first;
		}

		IEntityClassRegistry& entityClassRegistry = *gEnv->pEntitySystem->GetClassRegistry();
		if (bNewEntityClass && entityClassRegistry.FindClass(szClassName))
		{
			SCHEMATYC_ENV_CRITICAL_ERROR("Duplicate entity class name '%s'!", szClassName);
			return;
		}

		CAnyConstPtr pEnvClassProperties = runtimeClass.GetEnvClassProperties();
		SCHEMATYC_ENV_ASSERT(pEnvClassProperties);
		if (pEnvClassProperties)
		{
			const SEntityClassProperties& classProperties = DynamicCast<SEntityClassProperties>(*pEnvClassProperties);
			SEntityClass& entityClass = itEntityClass->second;

			entityClass.runtimeClassGUID = runtimeClass.GetGUID();

			entityClass.editorCategory = "Schematyc";
			entityClass.editorCategory.append("/");
			entityClass.editorCategory.append(szClassName);

			string::size_type fileNamePos = entityClass.editorCategory.find_last_of("/");
			if (fileNamePos != string::npos)
			{
				entityClass.editorCategory.erase(fileNamePos);
			}
			entityClass.editorCategory.replace("::", "/");

			entityClass.icon = classProperties.icon;

			const string::size_type iconFileNamePos = entityClass.icon.find_last_of("/\\");
			if (iconFileNamePos != string::npos)
			{
				entityClass.icon.erase(0, iconFileNamePos + 1);
			}

			entityClass.desc.flags = ECLF_BBOX_SELECTION;

			if (classProperties.bHideInEditor)
			{
				entityClass.desc.flags |= ECLF_INVISIBLE;
			}
			if (!bNewEntityClass)
			{
				entityClass.desc.flags |= ECLF_MODIFY_EXISTING;
			}

			entityClass.desc.sName = szClassName;
			entityClass.desc.editorClassInfo.sCategory = entityClass.editorCategory.c_str();
			entityClass.desc.editorClassInfo.sIcon = entityClass.icon.c_str();

			entityClass.desc.pUserProxyCreateFunc = &CreateSchematycEntityComponent;

			entityClassRegistry.RegisterStdClass(entityClass.desc);
		}
	}
}

void RegisterEntityClasses(IEnvRegistrar& registrar) // #SchematycTODO : Move to separate file?
{
	CEnvRegistrationScope scope = registrar.Scope(g_stdModuleGUID);
	{
		auto pClass = SCHEMATYC_MAKE_ENV_CLASS(g_entityClassGUID, "Entity");
		pClass->AddComponent(GetTypeInfo<CEntityDebugComponent>().GetGUID());
		pClass->AddComponent(GetTypeInfo<CEntityMovementComponent>().GetGUID());
		pClass->SetProperties(SEntityClassProperties());
		pClass->SetPreviewer(CEntityObjectPreviewer());
		scope.Register(pClass);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterEntityClasses)
