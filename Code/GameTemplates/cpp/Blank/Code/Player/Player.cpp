#include "StdAfx.h"
#include "Player.h"

#include "Movement/PlayerMovement.h"
#include "Input/PlayerInput.h"
#include "View/PlayerView.h"

#include "GamePlugin.h"
#include "Game/GameRules.h"

#include <CryRenderer/IRenderAuxGeom.h>

class CPlayerRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityWithDefaultComponent<CPlayer>("Player");
	}

	virtual void Unregister() override {}
};

CPlayerRegistrator g_playerRegistrator;

CPlayer::CPlayer()
	: m_pInput(nullptr)
	, m_pMovement(nullptr)
	, m_pView(nullptr)
{
}

CPlayer::~CPlayer()
{
	gEnv->pGameFramework->GetIActorSystem()->RemoveActor(GetEntityId());
}

bool CPlayer::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	return pGameObject->BindToNetwork();
}

void CPlayer::PostInit(IGameObject *pGameObject)
{
	m_pMovement = static_cast<CPlayerMovement *>(GetGameObject()->AcquireExtension("PlayerMovement"));
	m_pInput = static_cast<CPlayerInput *>(GetGameObject()->AcquireExtension("PlayerInput"));

	m_pView = static_cast<CPlayerView *>(GetGameObject()->AcquireExtension("PlayerView"));

	// Register with the actor system
	gEnv->pGameFramework->GetIActorSystem()->AddActor(GetEntityId(), this);
}

void CPlayer::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_RESET:
		{
			if (event.nParam[0] == 1)
			{
				// Make sure to revive player when respawning in Editor
				SetHealth(GetMaxHealth());
			}
		}
		break;
	}
}

void CPlayer::SetHealth(float health)
{
	if (health > 0)
	{
		m_pInput->OnPlayerRespawn();
	}
}