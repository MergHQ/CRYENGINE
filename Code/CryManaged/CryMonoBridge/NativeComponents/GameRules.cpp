#include "StdAfx.h"
#include "GameRules.h"

#include <../CryAction/IActorSystem.h>

string CManagedGameRules::s_actorClassName;

bool CManagedGameRules::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	if (!pGameObject->BindToNetwork())
		return false;

	gEnv->pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(this);

	return true;
}

bool CManagedGameRules::OnClientConnect(int channelId, bool isReset)
{
	return gEnv->pGameFramework->GetIActorSystem()->CreateActor(channelId, "DefaultPlayer", s_actorClassName, ZERO, IDENTITY, Vec3(1, 1, 1)) != nullptr;
}

void CManagedGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char* desc, bool keepClient)
{
	auto* pActorSystem = gEnv->pGameFramework->GetIActorSystem();
	auto* pActor = pActorSystem->GetActorByChannelId(channelId);

	if (pActor != nullptr)
	{
		pActorSystem->RemoveActor(pActor->GetEntityId());
	}
}