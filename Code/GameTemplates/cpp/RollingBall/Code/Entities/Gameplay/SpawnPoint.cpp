#include "StdAfx.h"
#include "SpawnPoint.h"

#include "GamePlugin.h"

#include <CryEntitySystem/IEntitySystem.h>

CRYREGISTER_CLASS(CSpawnPoint);

class CSpawnPointRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		RegisterEntityWithDefaultComponent<CSpawnPoint>("SpawnPoint");
	}

	virtual void Unregister() override {}
};

CSpawnPointRegistrator g_spawnerRegistrator;

void CSpawnPoint::SpawnEntity(IEntity &otherEntity)
{
	otherEntity.SetWorldTM(GetEntity()->GetWorldTM());
}