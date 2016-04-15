// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "Game.h"
#include "GameFactory.h"
#include "flownodes/FlowBaseNode.h"


CAutoRegFlowNodeBaseZero* CAutoRegFlowNodeBaseZero::m_pFirst = nullptr;
CAutoRegFlowNodeBaseZero* CAutoRegFlowNodeBaseZero::m_pLast = nullptr;


CGame::CGame()
	: m_pGameFramework(nullptr)
{
	GetISystem()->SetIGame(this);
}

CGame::~CGame()
{
	if (m_pGameFramework->StartedGameContext())
	{
		m_pGameFramework->EndGameContext();
	}
	
	GetISystem()->SetIGame(nullptr);
}

bool CGame::Init(IGameFramework* pFramework)
{
	m_pGameFramework = pFramework;
	CGameFactory::Init();
	m_pGameFramework->SetGameGUID(GAME_GUID);

	return true;
}

void CGame::RegisterGameFlowNodes()
{
	IFlowSystem* pFlowSystem = m_pGameFramework->GetIFlowSystem();
	if (pFlowSystem)
	{
		CAutoRegFlowNodeBaseZero* pFactory = CAutoRegFlowNodeBaseZero::m_pFirst;

		while (pFactory)
		{
			pFlowSystem->RegisterType(pFactory->m_sClassName, pFactory);
			pFactory = pFactory->m_pNext;
		}

		CGameFactory::RegisterEntityFlowNodes();
	}
}

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	const bool bRun = m_pGameFramework->PreUpdate(haveFocus, updateFlags);
	m_pGameFramework->PostUpdate(haveFocus, updateFlags);
	return bRun ? 1 : 0;
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
