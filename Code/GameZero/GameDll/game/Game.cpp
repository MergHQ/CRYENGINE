// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Game.h"
#include "GameFactory.h"

#include <CryFlowGraph/IFlowBaseNode.h>

#ifndef _LIB
CAutoRegFlowNodeBase* CAutoRegFlowNodeBase::m_pFirst = nullptr;
CAutoRegFlowNodeBase* CAutoRegFlowNodeBase::m_pLast = nullptr;
#endif

CGame::CGame()
{
}

CGame::~CGame()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (gEnv->pGameFramework->StartedGameContext())
	{
		gEnv->pGameFramework->EndGameContext();
	}

	UnregisterGameFlowNodes();
}

bool CGame::Init()
{
	CGameFactory::Init();
	gEnv->pGameFramework->SetGameGUID(GAME_GUID);

	// Request loading of the example map next frame
	gEnv->pConsole->ExecuteString("map example", false, true);
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	return true;
}

void CGame::RegisterGameFlowNodes()
{
#ifndef _LIB
	IFlowSystem* pFlowSystem = gEnv->pGameFramework->GetIFlowSystem();
	if (pFlowSystem)
	{
		CAutoRegFlowNodeBase* pFactory = CAutoRegFlowNodeBase::m_pFirst;
		while (pFactory)
		{
			pFlowSystem->RegisterType(pFactory->m_sClassName, pFactory);
			pFactory = pFactory->m_pNext;
		}

		CGameFactory::RegisterEntityFlowNodes();
	}
#endif
}

void CGame::UnregisterGameFlowNodes()
{
#ifndef _LIB
	IFlowSystem* pFlowSystem = gEnv->pGameFramework->GetIFlowSystem();
	if (pFlowSystem)
	{
		CAutoRegFlowNodeBase* pFactory = CAutoRegFlowNodeBase::m_pFirst;
		while (pFactory)
		{
			pFlowSystem->UnregisterType(pFactory->m_sClassName);
			pFactory = pFactory->m_pNext;
		}

		CGameFactory::UnregisterEntityFlowNodes();
	}
#endif
}

void CGame::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_REGISTER_FLOWNODES:
		{
			RegisterGameFlowNodes();
		}
		break;
	}
}

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	return 1;
}

void CGame::Shutdown()
{
	this->~CGame();
}

void CGame::GetMemoryStatistics(ICrySizer* s)
{
	s->Add(*this);
}

const char* CGame::GetLongName()
{
	return GAME_LONGNAME;
}

const char* CGame::GetName()
{
	return GAME_NAME;
}
