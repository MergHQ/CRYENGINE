#include "StdAfx.h"
#include "PlayerInput.h"

#include "Player/Player.h"

#include "Entities/Gameplay/Weapons/ISimpleWeapon.h"

#include <CryAnimation/ICryAnimation.h>

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

	// Make sure that this extension is updated regularly via the Update function below
	GetGameObject()->EnableUpdateSlot(this, 0);

	// Populate the action handler callbacks so that we get action map events
	InitializeActionHandler();
}

void CPlayerInput::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	// Start by updating look dir
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));
	
	ypr.x += m_mouseDeltaRotation.x * m_pPlayer->GetCVars().m_rotationSpeedYaw * ctx.fFrameTime;

	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * m_pPlayer->GetCVars().m_rotationSpeedPitch * ctx.fFrameTime, m_pPlayer->GetCVars().m_rotationLimitsMinPitch, m_pPlayer->GetCVars().m_rotationLimitsMaxPitch);

	ypr.z = 0;

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	// Reset every frame
	m_mouseDeltaRotation = ZERO;
}

void CPlayerInput::OnPlayerRespawn()
{
	m_inputFlags = 0;
	m_mouseDeltaRotation = ZERO;
	m_lookOrientation = IDENTITY;
}

void CPlayerInput::HandleInputFlagChange(EInputFlags flags, int activationMode, EInputFlagType type)
{
	switch (type)
	{
		case eInputFlagType_Hold:
		{
			if (activationMode == eIS_Released)
			{
				m_inputFlags &= ~flags;
			}
			else
			{
				m_inputFlags |= flags;
			}
		}
		break;
		case eInputFlagType_Toggle:
		{
			if (activationMode == eIS_Released)
			{
				// Toggle the bit(s)
				m_inputFlags ^= flags;
			}
		}
		break;
	}
}

void CPlayerInput::InitializeActionHandler()
{
	m_actionHandler.AddHandler(ActionId("moveleft"), &CPlayerInput::OnActionMoveLeft);
	m_actionHandler.AddHandler(ActionId("moveright"), &CPlayerInput::OnActionMoveRight);
	m_actionHandler.AddHandler(ActionId("moveforward"), &CPlayerInput::OnActionMoveForward);
	m_actionHandler.AddHandler(ActionId("moveback"), &CPlayerInput::OnActionMoveBack);

	m_actionHandler.AddHandler(ActionId("mouse_rotateyaw"), &CPlayerInput::OnActionMouseRotateYaw);
	m_actionHandler.AddHandler(ActionId("mouse_rotatepitch"), &CPlayerInput::OnActionMouseRotatePitch);

	m_actionHandler.AddHandler(ActionId("shoot"), &CPlayerInput::OnActionShoot);
}

void CPlayerInput::OnAction(const ActionId &action, int activationMode, float value)
{
	// This function is called when inputs trigger action map events
	// The handler below maps the event (see 'action') to a callback, further below in this file.
	m_actionHandler.Dispatch(this, GetEntityId(), action, activationMode, value);
}

bool CPlayerInput::OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	HandleInputFlagChange(eInputFlag_MoveLeft, activationMode);
	return true;
}

bool CPlayerInput::OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	HandleInputFlagChange(eInputFlag_MoveRight, activationMode);
	return true;
}

bool CPlayerInput::OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	HandleInputFlagChange(eInputFlag_MoveForward, activationMode);
	return true;
}

bool CPlayerInput::OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	HandleInputFlagChange(eInputFlag_MoveBack, activationMode);
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

bool CPlayerInput::OnActionShoot(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	// Only fire on press, not release
	if (activationMode == eIS_Pressed)
	{
		auto *pWeapon = m_pPlayer->GetCurrentWeapon();
		auto *pCharacter = GetEntity()->GetCharacter(CPlayer::eGeometry_ThirdPerson);

		if (pWeapon != nullptr && pCharacter != nullptr)
		{
			auto *pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("barrel_out");

			if (pBarrelOutAttachment != nullptr)
			{
				QuatTS bulletOrigin = pBarrelOutAttachment->GetAttWorldAbsolute();

				pWeapon->RequestFire(bulletOrigin.t, bulletOrigin.q);
			}
		}
	}

	return true;
}