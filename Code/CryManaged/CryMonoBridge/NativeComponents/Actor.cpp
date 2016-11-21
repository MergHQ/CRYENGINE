#include "StdAfx.h"
#include "Actor.h"

CManagedActor::~CManagedActor()
{
	gEnv->pGameFramework->GetIActorSystem()->RemoveActor(GetEntityId());
}

bool CManagedActor::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGameFramework->GetIActorSystem()->AddActor(GetEntityId(), this);
	return true;
}