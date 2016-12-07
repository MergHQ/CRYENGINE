#include "StdAfx.h"
#include "PlayerInput.h"

#include "Player/Player.h"

#include <CryAnimation/ICryAnimation.h>

#include "GamePlugin.h"

class CPlayerInputRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityComponent<CPlayerInput>("PlayerInput");
	}

	virtual void Unregister() override {}
};

CPlayerInputRegistrator g_playerInputRegistrator;

void CPlayerInput::PostInit(IGameObject *pGameObject)
{
	m_pPlayer = static_cast<CPlayer *>(pGameObject->QueryExtension("Player"));

	// NOTE: Since CRYENGINE 5.3, the game is responsible to initialize the action maps
	IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
	pActionMapManager->InitActionMaps("Libs/config/defaultprofile.xml");
	pActionMapManager->Enable(true);
	pActionMapManager->EnableActionMap("player", true);

	if (IActionMap *pActionMap = pActionMapManager->GetActionMap("player"))
	{
		pActionMap->SetActionListener(GetEntityId());
	}

	GetGameObject()->CaptureActions(this);

	m_mouseDeltaRotation = ZERO;

	// Populate the action handler callbacks so that we get action map events
	InitializeActionHandler();
}

void CPlayerInput::OnPlayerRespawn()
{
	m_moveDirection = ZERO;
	m_mouseDeltaRotation = ZERO;
}

void CPlayerInput::InitializeActionHandler()
{
	m_actionHandler.AddHandler(ActionId("moveleft"), &CPlayerInput::OnActionMoveLeft);
	m_actionHandler.AddHandler(ActionId("moveright"), &CPlayerInput::OnActionMoveRight);
	m_actionHandler.AddHandler(ActionId("moveforward"), &CPlayerInput::OnActionMoveForward);
	m_actionHandler.AddHandler(ActionId("moveback"), &CPlayerInput::OnActionMoveBack);

	m_actionHandler.AddHandler(ActionId("mouse_rotateyaw"), &CPlayerInput::OnActionMouseRotateYaw);
	m_actionHandler.AddHandler(ActionId("mouse_rotatepitch"), &CPlayerInput::OnActionMouseRotatePitch);
}

void CPlayerInput::OnAction(const ActionId &action, int activationMode, float value)
{
	// This function is called when inputs trigger action map events
	// The handler below maps the event (see 'action') to a callback, further below in this file.
	m_actionHandler.Dispatch(this, GetEntityId(), action, activationMode, value);
}

bool CPlayerInput::OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_moveDirection.x = -value;
	return true;
}

bool CPlayerInput::OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_moveDirection.x = value;
	return true;
}

bool CPlayerInput::OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_moveDirection.y = value;
	return true;
}

bool CPlayerInput::OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_moveDirection.y = -value;
	return true;
}

bool CPlayerInput::OnActionMouseRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_mouseDeltaRotation.x -= value;
	return true;
}

bool CPlayerInput::OnActionMouseRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_mouseDeltaRotation.y -= value;
	return true;
}