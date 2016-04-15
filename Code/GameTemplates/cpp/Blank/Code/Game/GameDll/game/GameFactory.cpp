// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "game/GameRules.h"
#include "game/GameFactory.h"
#include "player/Player.h"
#include "player/extensions/ViewExtension.h"
#include "player/extensions/InputExtension.h"
#include "player/extensions/MovementExtension.h"
#include "entities/GeomEntity.h"
#include "flownodes/FlowGameEntityNode.h"

std::map<string, CGameEntityNodeFactory*> CGameFactory::s_flowNodeFactories;


void CGameFactory::Init()
{
	RegisterGameObject<CPlayer>("Player", "", eGORF_HiddenInEditor);
	RegisterGameObject<CInputExtension>("InputExtension", "", eGORF_NoEntityClass);
	RegisterGameObject<CMovementExtension>("MovementExtension", "", eGORF_NoEntityClass);
	RegisterGameObject<CViewExtension>("ViewExtension", "", eGORF_NoEntityClass);
	RegisterGameObject<CGameRules>("GameRules", "", eGORF_HiddenInEditor);
	RegisterNoScriptGameObject<CGeomEntity>("GameGeomEntity", "Game/GeomEntity", eGORF_None);

	IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();
	pGameFramework->GetIGameRulesSystem()->RegisterGameRules("SinglePlayer", "GameRules");
	pGameFramework->GetIGameRulesSystem()->AddGameRulesAlias("SinglePlayer", "sp");
}

void CGameFactory::RegisterEntityFlowNodes()
{
	IFlowSystem* pFlowSystem = gEnv->pGame->GetIGameFramework()->GetIFlowSystem();
	std::map<string, CGameEntityNodeFactory*>::iterator it = s_flowNodeFactories.begin(), end = s_flowNodeFactories.end();
	for(; it != end; ++it)
	{
		pFlowSystem->RegisterType(it->first.c_str(), it->second);
	}

	stl::free_container(s_flowNodeFactories);
}

void CGameFactory::CreateScriptTables(SEntityScriptProperties& out, uint32 flags)
{
	out.pEntityTable = gEnv->pScriptSystem->CreateTable();
	out.pEntityTable->AddRef();

	out.pEditorTable = gEnv->pScriptSystem->CreateTable();
	out.pEditorTable->AddRef();

	out.pPropertiesTable = gEnv->pScriptSystem->CreateTable();
	out.pPropertiesTable->AddRef();

	if (flags & eGORF_InstanceProperties)
	{
		out.pInstancePropertiesTable = gEnv->pScriptSystem->CreateTable();
		out.pInstancePropertiesTable->AddRef();
	}
}

template<class T>
struct CObjectCreator : public IGameObjectExtensionCreatorBase
{
	IGameObjectExtensionPtr Create()
	{
		return ComponentCreate_DeleteWithRelease<T>();
	}

	void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
	{
		T::GetGameObjectExtensionRMIData(ppRMI, nCount);
	}
};

template<class T>
void CGameFactory::RegisterGameObject(const string& name, const string& script, uint32 flags)
{
	bool registerClass = ((flags & eGORF_NoEntityClass) == 0);
	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = name.c_str();
	clsDesc.sScriptFile = script.c_str();
	
	static CObjectCreator<T> _creator;

	gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(name.c_str(), &_creator, registerClass ? &clsDesc : nullptr);
	T::SetExtensionId(gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->GetID(name.c_str()));

	if ((flags & eGORF_HiddenInEditor) != 0)
	{ 
		IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name.c_str());
		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE); 
	}
}

template<class T>
void CGameFactory::RegisterNoScriptGameObject(const string& name, const string& editorPath, uint32 flags)
{
	SEntityScriptProperties props;
	CGameFactory::CreateScriptTables(props, flags);
	gEnv->pScriptSystem->SetGlobalValue(name.c_str(), props.pEntityTable);

	CGameEntityNodeFactory* pNodeFactory = new CGameEntityNodeFactory();
	T::RegisterProperties(props, pNodeFactory);

	if (!editorPath.empty())
	{
		props.pEditorTable->SetValue("EditorPath", editorPath.c_str());
	}

	props.pEntityTable->SetValue("Editor", props.pEditorTable);
	props.pEntityTable->SetValue("Properties", props.pPropertiesTable);

	if (flags & eGORF_InstanceProperties)
	{
		props.pEntityTable->SetValue("PropertiesInstance", props.pInstancePropertiesTable);
	}

	string nodeName = "entity:";
	nodeName += name.c_str();
	pNodeFactory->Close();
	s_flowNodeFactories[nodeName] = pNodeFactory;

	const bool registerClass = ((flags & eGORF_NoEntityClass) == 0);
	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = name.c_str();
	clsDesc.pScriptTable = props.pEntityTable;

	static CObjectCreator<T> _creator;

	gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(name.c_str(), &_creator, registerClass ? &clsDesc : nullptr);
	T::SetExtensionId(gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->GetID(name.c_str()));

	if ((flags & eGORF_HiddenInEditor) != 0)
	{ 
		IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name.c_str());
		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE); 
	}
}
