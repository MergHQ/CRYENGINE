// CryEngine Source File.
// Copyright (C), Crytek, 1999-2016.


#include "StdAfx.h"
#include "Game/GameRules.h"
#include "Game/GameFactory.h"

#include "Entities/Helpers/NativeEntityPropertyHandling.h"

#include "FlowNodes/Helpers/FlowGameEntityNode.h"
#include "FlowNodes/Helpers/FlowBaseNode.h"

std::map<string, CGameEntityNodeFactory*> CGameFactory::s_flowNodeFactories;

IEntityRegistrator *IEntityRegistrator::g_pFirst = nullptr;
IEntityRegistrator *IEntityRegistrator::g_pLast = nullptr;

CAutoRegFlowNodeBaseZero *CAutoRegFlowNodeBaseZero::m_pFirst = nullptr;
CAutoRegFlowNodeBaseZero *CAutoRegFlowNodeBaseZero::m_pLast = nullptr;

void CGameFactory::Init()
{
	// Register entities
	if(IEntityRegistrator::g_pFirst != nullptr)
	{
		do
		{
			IEntityRegistrator::g_pFirst->Register();

			IEntityRegistrator::g_pFirst = IEntityRegistrator::g_pFirst->m_pNext;
		}
		while(IEntityRegistrator::g_pFirst != nullptr);
	}
}

void CGameFactory::RegisterFlowNodes()
{
	// Start with registering the entity nodes
	IFlowSystem *pFlowSystem = gEnv->pGame->GetIGameFramework()->GetIFlowSystem();

	for(auto it = s_flowNodeFactories.begin(); it != s_flowNodeFactories.end(); ++it)
	{
		pFlowSystem->RegisterType(it->first.c_str(), it->second);
	}

	stl::free_container(s_flowNodeFactories);

	// Now register the regular nodes
	CAutoRegFlowNodeBaseZero *pFactory = CAutoRegFlowNodeBaseZero::m_pFirst;
	while (pFactory)
	{
		pFlowSystem->RegisterType(pFactory->m_sClassName, pFactory);
		pFactory = pFactory->m_pNext;
	}
}

CGameEntityNodeFactory &CGameFactory::RegisterEntityFlowNode(const char *className)
{
	CGameEntityNodeFactory* pNodeFactory = new CGameEntityNodeFactory();
	
	string nodeName = "entity:";
	nodeName += className;

	s_flowNodeFactories[nodeName] = pNodeFactory;

	return *pNodeFactory;
}