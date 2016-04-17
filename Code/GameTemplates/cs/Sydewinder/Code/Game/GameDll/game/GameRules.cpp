// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "GameRules.h"
#include <GameObjects/GameObject.h>


CGameRules::CGameRules()
{
}

CGameRules::~CGameRules()
{
	gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(nullptr);
}

bool CGameRules::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(this);

	return true;
}

bool CGameRules::OnClientConnect(int channelId, bool isReset)
{
	const float fTerrainSize = static_cast<float>(gEnv->p3DEngine->GetTerrainSize());
	const float fTerrainElevation = gEnv->p3DEngine->GetTerrainElevation(fTerrainSize * 0.5f, fTerrainSize * 0.5f);
	const Vec3 vSpawnLocation(fTerrainSize * 0.5f, fTerrainSize * 0.5f, fTerrainElevation + 15.0f);

	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
	IEntityClass* pPlayerClass = pEntitySystem->GetClassRegistry()->FindClass("Player");

	if (!pPlayerClass)
		return false;

	SEntitySpawnParams params;
	params.sName = "Player";
	params.vPosition = vSpawnLocation;
	params.pClass = pPlayerClass;

	if (channelId)
	{
		params.nFlags |= ENTITY_FLAG_NEVER_NETWORK_STATIC;
		if (INetChannel* pNetChannel = gEnv->pGame->GetIGameFramework()->GetNetChannel(channelId))
		{
			if (pNetChannel->IsLocal())
			{
				params.id = LOCAL_PLAYER_ENTITY_ID;
			}
		}
	}

	IEntity* pPlayerEntity = pEntitySystem->SpawnEntity(params, false);

	CGameObject* pGameObject = static_cast<CGameObject*>(pPlayerEntity->GetProxy(ENTITY_PROXY_USER));
	// always set the channel id before initializing the entity
	pGameObject->SetChannelId(channelId);

	return pEntitySystem->InitEntity(pPlayerEntity, params);
}

void CGameRules::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
}
