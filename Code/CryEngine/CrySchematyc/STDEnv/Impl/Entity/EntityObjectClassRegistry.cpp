// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectClassRegistry.h"

#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUserData.h>

#include "AutoRegister.h"
#include "STDEnv.h"
#include "STDModules.h"
#include "Entity/EntityClassProperties.h"
#include "Entity/EntityObjectComponent.h"
#include "Entity/EntityObjectMap.h"
#include "Entity/EntityObjectPreviewer.h"
#include "Entity/Components/EntityDebugComponent.h"
#include "Entity/Components/EntityMovementComponent.h"
#include "Entity/Components/EntityUtilsComponent.h"

namespace Schematyc
{
namespace
{
IEntityComponent* CreateSchematycEntityComponent(IEntity* pEntity, SEntitySpawnParams& spawnParams, void* pUserData)
{
	const SEntityObjectClass* pEntityObjectClass = static_cast<const SEntityObjectClass*>(pUserData);
	IRuntimeClassConstPtr pRuntimeClass = gEnv->pSchematyc->GetRuntimeRegistry().GetClass(pEntityObjectClass->guid);
	SCHEMATYC_CORE_ASSERT(pRuntimeClass);
	if (pRuntimeClass)
	{
		CEntityObjectComponent* pEntityObjectComponent = pEntity->CreateComponentClass<CEntityObjectComponent>();
		if (pEntityObjectComponent)
		{
			const SEntityUserData* pEntityUserData = (const SEntityUserData*)spawnParams.pUserData;
			pEntityObjectComponent->SetRuntimeClass(*pRuntimeClass, pEntityUserData ? pEntityUserData->bIsPreview : false);
			return pEntityObjectComponent;
		}
	}
	return nullptr;
}
}   // Anonymous

SEntityObjectClass::SEntityObjectClass()
{
	// N.B. This is a quick work around for the problem of entities without script tables not triggering areas.
	entityClassDesc.pScriptTable = gEnv->pScriptSystem->CreateTable();
	entityClassDesc.pScriptTable->AddRef();
}

SEntityObjectClass::SEntityObjectClass(const SEntityObjectClass& rhs)
	: name(rhs.name)
	, icon(rhs.icon)
	, entityClassDesc(rhs.entityClassDesc)
	, pEntityClass(rhs.pEntityClass)
{
	entityClassDesc.pScriptTable->AddRef();
}

SEntityObjectClass::~SEntityObjectClass()
{
	entityClassDesc.pScriptTable->Release();
}

void CEntityObjectClassRegistry::Init()
{
	gEnv->pSchematyc->GetCompiler().GetClassCompilationSignalSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(&CEntityObjectClassRegistry::OnClassCompilation, *this), m_connectionScope); // TODO : Can we filter by class guid?
}

const SEntityObjectClass* CEntityObjectClassRegistry::GetEntityObjectClass(const SGUID& guid) const
{
	EntityObjectClassesByGUID::const_iterator itEntityObjectClass = m_entityObjectClassesByGUID.find(guid);
	return itEntityObjectClass != m_entityObjectClassesByGUID.end() ? itEntityObjectClass->second : nullptr;
}

void CEntityObjectClassRegistry::OnClassCompilation(const IRuntimeClass& runtimeClass)
{
	if (runtimeClass.GetEnvClassGUID() == g_entityClassGUID)
	{
		string className = "Schematyc";
		className.append("::");
		className.append(runtimeClass.GetName());

		EntityObjectClasses::iterator itEntityObjectClass = m_entityObjectClasses.find(className.c_str());
		if (itEntityObjectClass == m_entityObjectClasses.end())
		{
			if (gEnv->pEntitySystem->GetClassRegistry()->FindClass(className))
			{
				SCHEMATYC_ENV_CRITICAL_ERROR("Duplicate entity class name '%s'!", className.c_str());
				return;
			}

			itEntityObjectClass = m_entityObjectClasses.insert(EntityObjectClasses::value_type(className, SEntityObjectClass())).first;
		}

		const SGUID classGUID = runtimeClass.GetGUID();
		SEntityObjectClass& entityObjectClass = itEntityObjectClass->second;

		m_entityObjectClassesByGUID[classGUID] = &entityObjectClass;

		CAnyConstPtr pEnvClassProperties = runtimeClass.GetEnvClassProperties();
		SCHEMATYC_ENV_ASSERT(pEnvClassProperties);
		if (pEnvClassProperties)
		{
			const SEntityClassProperties& classProperties = DynamicCast<SEntityClassProperties>(*pEnvClassProperties);

			entityObjectClass.name = className;
			entityObjectClass.guid = runtimeClass.GetGUID();
			entityObjectClass.icon = classProperties.icon;

			const string::size_type iconFileNamePos = entityObjectClass.icon.find_last_of("/\\");
			if (iconFileNamePos != string::npos)
			{
				entityObjectClass.icon.erase(0, iconFileNamePos + 1);
			}

			entityObjectClass.entityClassDesc.flags = ECLF_BBOX_SELECTION;

			if (classProperties.bHideInEditor)
			{
				entityObjectClass.entityClassDesc.flags |= ECLF_INVISIBLE;
			}

			if (entityObjectClass.pEntityClass)
			{
				entityObjectClass.entityClassDesc.flags |= ECLF_MODIFY_EXISTING;
			}

			entityObjectClass.entityClassDesc.sName = entityObjectClass.name.c_str();
			entityObjectClass.entityClassDesc.editorClassInfo.sCategory = "Schematyc";
			entityObjectClass.entityClassDesc.editorClassInfo.sIcon = entityObjectClass.icon.c_str();
			entityObjectClass.entityClassDesc.pUserProxyCreateFunc = &CreateSchematycEntityComponent;
			entityObjectClass.entityClassDesc.pUserProxyData = &entityObjectClass;
		}

		entityObjectClass.pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(entityObjectClass.entityClassDesc);

		SCHEMATYC_ENV_ASSERT(entityObjectClass.pEntityClass);
	}
}

void RegisterEntityClasses(IEnvRegistrar& registrar)   // #SchematycTODO : Move to separate file?
{
	CEnvRegistrationScope scope = registrar.Scope(g_stdModuleGUID);
	{
		auto pClass = SCHEMATYC_MAKE_ENV_CLASS(g_entityClassGUID, "Entity");
		pClass->AddComponent(GetTypeDesc<CEntityDebugComponent>().GetGUID());
		//pClass->AddComponent(GetTypeDesc<CEntityMovementComponent>().GetGUID());
		pClass->AddComponent(GetTypeDesc<CEntityUtilsComponent>().GetGUID());
		pClass->SetProperties(SEntityClassProperties());
		pClass->SetPreviewer(CEntityObjectPreviewer());
		scope.Register(pClass);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterEntityClasses)
