#include "StdAfx.h"
#include "SpawnPoint.h"

#include "GamePlugin.h"

#include "Player/Player.h"

class CSpawnPointRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityWithDefaultComponent<CSpawnPoint>("SpawnPoint");
	}
};

CSpawnPointRegistrator g_spawnerRegistrator;

void CSpawnPoint::SpawnEntity(IEntity &otherEntity)
{
	otherEntity.SetWorldTM(GetEntity()->GetWorldTM());
}