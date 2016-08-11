#include "StdAfx.h"
#include "SpawnPoint.h"

#include "Game/GameFactory.h"

#include "Player/Player.h"

class CSpawnPointRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGameFactory::RegisterNativeEntity<CSpawnPoint>("SpawnPoint", "Gameplay");
	}
};

CSpawnPointRegistrator g_spawnerRegistrator;

void CSpawnPoint::SpawnEntity(IEntity &otherEntity)
{
	otherEntity.SetWorldTM(GetEntity()->GetWorldTM());
}