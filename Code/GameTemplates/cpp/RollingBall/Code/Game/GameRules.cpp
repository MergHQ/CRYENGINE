// CryEngine Source File.
// Copyright (C), Crytek, 1999-2016.


#include "StdAfx.h"
#include "GameRules.h"

#include "GamePlugin.h"

#include "Player/Player.h"
#include "Entities/Gameplay/SpawnPoint.h"

class CRulesRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityWithDefaultComponent<CGameRules>("GameRules");

		// Get the default game rules name
		// Note that this is not necessary, feel free to replace your game rules name with a custom string here
		// It is possible to have multiple game rules implementations registered.
		ICVar *pDefaultGameRulesVar = gEnv->pConsole->GetCVar("sv_gamerulesdefault");

		gEnv->pGameFramework->GetIGameRulesSystem()->RegisterGameRules(pDefaultGameRulesVar->GetString(), "GameRules", false);
		gEnv->pGameFramework->GetIGameRulesSystem()->AddGameRulesAlias(pDefaultGameRulesVar->GetString(), pDefaultGameRulesVar->GetString());
	}

	virtual void Unregister() override {}
};

CRulesRegistrator g_gameRulesRegistrator;

CGameRules::~CGameRules()
{
	gEnv->pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(nullptr);
}

bool CGameRules::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(this);

	return true;
}

bool CGameRules::OnClientConnect(int channelId, bool isReset)
{
	// Called when a new client connects to the server
	// Occurs during level load for the local player
	// In this case we create a player called "DefaultPlayer", and use the "Player" entity class registered in Player.cpp

	SEntitySpawnParams params;
	params.id = LOCAL_PLAYER_ENTITY_ID;
	params.sName = "DefaultPlayer";
	params.nFlags = ENTITY_FLAG_LOCAL_PLAYER;
	params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	CRY_ASSERT(params.pClass);

	IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params);
	if (pEntity)
	{
		m_players[channelId] = pEntity->GetId();
	}
	return pEntity ? true : false;
}

void CGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient)
{
	m_players.erase(channelId);
}

bool CGameRules::OnClientEnteredGame(int channelId, bool isReset)
{
	// Trigger actor revive, but never do this outside of game mode in the Editor
	if (!gEnv->IsEditing())
	{
		// This is a quick hack. Previously there has been pActorSystem->GetActorByChannelId(channelId)
		// It makes sense to call this in ENTITY_EVENT_SET_AUTHORITY for the player,
		// but it's not network-bound yet, so that will fail.
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_players[channelId]);
		pEntity->GetComponent<CPlayer>()->Respawn();
	}

	return true;
}