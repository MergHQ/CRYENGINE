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

	string player_name = string().Format("Player%d", m_players.size());
	params.sName = player_name;
	params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	CRY_ASSERT(params.pClass);

	IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params);
	if (pEntity)
	{
		// channelId: GameServerChannelId.
		m_players[channelId] = pEntity->GetId();

		pEntity->GetNetEntity()->SetChannelId(channelId);
		pEntity->GetNetEntity()->BindToNetwork();
	}
	return pEntity ? true : false;
}

void CGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient)
{
	m_players.erase(channelId);
}

bool CGameRules::OnClientEnteredGame(int channelId, bool isReset)
{
	// Ignore loading the map in the editor.
	if (gEnv->IsEditing())
		return true;

	// Respawn the player.
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_players[channelId]);
	pEntity->GetComponent<CPlayer>()->Respawn();

	return true;
}