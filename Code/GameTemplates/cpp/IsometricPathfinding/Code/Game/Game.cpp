// CryEngine Source File
// Copyright (C), Crytek, 1999-2016


#include "StdAfx.h"
#include "Game.h"
#include "GameFactory.h"

#include <IPlayerProfiles.h>

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

	// We hafve to initialize the player profile system
	// Otherwise level load will fail in action map initialization
	InitializePlayerProfile();

	return true;
}

void CGame::InitializePlayerProfile()
{
	auto &profileManager = *m_pGameFramework->GetIPlayerProfileManager();

	profileManager.Initialize();

	bool bFirstLogin = true;
	if (!profileManager.LoginUser("Player", bFirstLogin))
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to login player profile user.");
		return;
	}

	profileManager.ActivateProfile("Player", "default");
}

void CGame::RegisterGameFlowNodes()
{
	CGameFactory::RegisterFlowNodes();
}

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	const bool bRun = m_pGameFramework->PreUpdate(haveFocus, updateFlags);

	// Perform update of game-specific systems here

	m_pGameFramework->PostUpdate(haveFocus, updateFlags);
	
	return bRun ? 1 : 0;
}

const char* CGame::GetName()
{
	ICVar *pGameName = gEnv->pConsole->GetCVar("sys_game_name");

	return pGameName->GetName();
}