// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GameRules.h"
#include <GameObjects/GameObject.h>

CGameRules::CGameRules()
{
}

CGameRules::~CGameRules()
{
	gEnv->pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(nullptr);
}

bool CGameRules::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(this);

	return true;
}

bool InitializeGameObjectCallback(IEntity* pEntity, IGameObject* pGameObject, void* pUserData)
{
	pGameObject->SetChannelId(*static_cast<uint16*>(pUserData));
	return true;
}

bool CGameRules::OnClientConnect(int channelId_old, bool isReset)
{
	uint16 channelId = static_cast<uint16>(channelId_old);
	const float fTerrainSize = static_cast<float>(gEnv->p3DEngine->GetTerrainSize());
	const float fTerrainElevation = gEnv->p3DEngine->GetTerrainElevation(fTerrainSize * 0.5f, fTerrainSize * 0.5f);
	const Vec3 vSpawnLocation(fTerrainSize * 0.5f, fTerrainSize * 0.5f, fTerrainElevation + 15.0f);

	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
	IEntityClass* pPlayerClass = pEntitySystem->GetClassRegistry()->FindClass("Player");

	if (!pPlayerClass)
		return false;

	IGameObjectSystem::SEntitySpawnParamsForGameObjectWithPreactivatedExtension userData;
	userData.hookFunction = InitializeGameObjectCallback;
	userData.pUserData = &channelId;

	SEntitySpawnParams params;
	params.sName = "Player";
	params.vPosition = vSpawnLocation;
	params.pClass = pPlayerClass;
	params.pUserData = (void*)&userData;

	if (channelId)
	{
		params.nFlags |= ENTITY_FLAG_NEVER_NETWORK_STATIC;
		if (INetChannel* pNetChannel = gEnv->pGameFramework->GetNetChannel(channelId))
		{
			if (pNetChannel->IsLocal())
			{
				params.id = LOCAL_PLAYER_ENTITY_ID;
			}
		}
	}

	return (pEntitySystem->SpawnEntity(params) != nullptr);
}

void CGameRules::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
}
