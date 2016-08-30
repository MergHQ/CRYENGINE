// CryEngine Source File.
// Copyright (C), Crytek, 1999-2016.


#include "StdAfx.h"
#include "GameRules.h"

#include "Game/GameFactory.h"

#include "Player/Player.h"
#include "Entities/Gameplay/SpawnPoint.h"

#include "FlowNodes/Helpers/FlowGameEntityNode.h"

#include <IActorSystem.h>

class CRulesRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGameFactory::RegisterGameObject<CGameRules>("GameRules");

		IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();

		// Get the default game rules name
		// Note that this is not necessary, feel free to replace your game rules name with a custom string here
		// It is possible to have multiple game rules implementations registered.
		ICVar *pDefaultGameRulesVar = gEnv->pConsole->GetCVar("sv_gamerulesdefault");

		pGameFramework->GetIGameRulesSystem()->RegisterGameRules(pDefaultGameRulesVar->GetString(), "GameRules");
		pGameFramework->GetIGameRulesSystem()->AddGameRulesAlias(pDefaultGameRulesVar->GetString(), pDefaultGameRulesVar->GetString());
	}
};

CRulesRegistrator g_gameRulesRegistrator;

CGameRules::~CGameRules()
{
	gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(nullptr);
}

bool CGameRules::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(this);

	return true;
}

bool CGameRules::OnClientConnect(int channelId, bool isReset)
{
	auto *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	
	// Called when a new client connects to the server
	// Occurs during level load for the local player
	// In this case we create a player called "DefaultPlayer", and use the "Player" entity class registered in Player.cpp
	return pActorSystem->CreateActor(channelId, "DefaultPlayer", "Player", ZERO, IDENTITY, Vec3(1, 1, 1)) != nullptr;
}

void CGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient)
{
	auto *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	if(IActor *pActor = pActorSystem->GetActorByChannelId(channelId))
	{
		pActorSystem->RemoveActor(pActor->GetEntityId());
	}
}

bool CGameRules::OnClientEnteredGame(int channelId, bool isReset)
{
	auto *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();

	if (auto *pActor = pActorSystem->GetActorByChannelId(channelId))
	{
		// Trigger actor revive
		pActor->SetHealth(pActor->GetMaxHealth());
	}

	return true;
}