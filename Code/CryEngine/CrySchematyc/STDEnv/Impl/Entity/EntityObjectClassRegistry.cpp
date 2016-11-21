// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectClassRegistry.h"

#include <CryEntitySystem/IEntityAttributesProxy.h>
#include <GameObjects/GameObject.h>
#include <GameObjects/GameObjectSystem.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUserData.h>

#include "AutoRegister.h"
#include "STDEnv.h"
#include "STDModules.h"
#include "Entity/EntityClassProperties.h"
#include "Entity/EntityObjectAttribute.h"
#include "Entity/EntityObjectMap.h"
#include "Entity/EntityObjectPreviewer.h"
#include "Entity/GameObjectExtension.h"
#include "Entity/Components/EntityDebugComponent.h"
#include "Entity/Components/EntityMovementComponent.h"

namespace Schematyc
{
namespace
{
class CGameObjectExtensionCreator : public IGameObjectExtensionCreatorBase
{
public:

	// IGameObjectExtensionCreatorBase

	virtual IGameObjectExtensionPtr Create()
	{
		return ComponentCreate_DeleteWithRelease<CGameObjectExtension>();
	}

	virtual void GetGameObjectExtensionRMIData(void** ppRMI, size_t* pCount)
	{
		CGameObjectExtension::GetGameObjectExtensionRMIData(ppRMI, pCount);
	}

	// ~IGameObjectExtensionCreatorBase
};

IEntityProxyPtr CreateGameObjectProxyAndExtension(IEntity* pEntity, SEntitySpawnParams& spawnParams, void* pUserData)
{
	IGameObject* pGameObject = nullptr;
	IEntityProxyPtr pEntityProxy = gEnv->pGameFramework->GetIGameObjectSystem()->CreateGameObjectEntityProxy(*pEntity, &pGameObject);
	SCHEMATYC_ENV_ASSERT(pGameObject);
	if (pGameObject)
	{
		if (pGameObject->ActivateExtension(CGameObjectExtension::ms_szExtensionName))
		{
			pGameObject->SetUserData(spawnParams.pUserData);
			return pEntityProxy;
		}
		else
		{
			pEntity->RegisterComponent(pEntityProxy, false);
		}
	}
	return IEntityProxyPtr();
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
	static CGameObjectExtensionCreator gameObjectExtensionCreator;
	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(CGameObjectExtension::ms_szExtensionName, &gameObjectExtensionCreator, nullptr);

	gEnv->pSchematyc->GetCompiler().GetClassCompilationSignalSlots().Connect(Delegate::Make(*this, &CEntityObjectClassRegistry::OnClassCompilation), m_connectionScope);  // TODO : Can we filter by class guid?
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

			entityClass.desc.classAttributes.clear();
			entityClass.desc.entityAttributes.clear();

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
			entityClass.desc.pUserProxyCreateFunc = &CreateGameObjectProxyAndExtension;

			entityClass.desc.entityAttributes.push_back(std::make_shared<CEntityObjectAttribute>(runtimeClass.GetGUID(), runtimeClass.GetDefaultProperties()));

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
