#include "StdAfx.h"
#include "PlayerInput.h"
#include "Player/Player.h"
#include "Player/Movement/PlayerMovement.h"

#include <CryNetwork/Rmi.h>
#include <CryGame/IGameFramework.h>

CRYREGISTER_CLASS(CPlayerInput);

CPlayerInput::~CPlayerInput()
{
	gEnv->pGameFramework->GetIActionMapManager()->RemoveExtraActionListener(this);
}

void CPlayerInput::Initialize()
{
	m_pPlayer = GetEntity()->GetComponent<CPlayer>();

	SRmi<RMI_WRAP(&CPlayerInput::SvInput)>::Register(this, eRAT_NoAttach, true, eNRT_ReliableOrdered);
}

void CPlayerInput::InitLocalPlayer()
{
	// NOTE: Since CRYENGINE 5.3, the game is responsible to initialize the action maps
	IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
	pActionMapManager->InitActionMaps("Libs/config/defaultprofile.xml");
	pActionMapManager->Enable(true);
	pActionMapManager->EnableActionMap("player", true);

	if (IActionMap *pActionMap = pActionMapManager->GetActionMap("player"))
	{
		pActionMap->SetActionListener(GetEntityId());
	}

	pActionMapManager->AddExtraActionListener(this);

	// Populate the action handler callbacks so that we get action map events
	InitializeActionHandler();
}

uint64 CPlayerInput::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_UPDATE)
		| BIT64(ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER)
	;
};

void CPlayerInput::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_UPDATE:
		{
			SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];
			Update(*pCtx);
		}
		break;
		case ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER:
			InitLocalPlayer();
		break;
	}
}

void CPlayerInput::Update(SEntityUpdateContext &ctx)
{
	if (m_mouseDeltaRotation.IsZero())
		return;

	// Start by updating look dir
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	ypr.x += m_mouseDeltaRotation.x * m_pPlayer->GetCVars().m_rotationSpeedYaw * ctx.fFrameTime;

	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * m_pPlayer->GetCVars().m_rotationSpeedPitch * ctx.fFrameTime, m_pPlayer->GetCVars().m_rotationLimitsMinPitch, m_pPlayer->GetCVars().m_rotationLimitsMaxPitch);

	ypr.z = 0;

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	m_mouseDeltaRotation = ZERO;

	// Look direction needs to be synced to server to calculate the movement in
	// the right direction.
	NetMarkAspectsDirty(kInputAspect);
}

void CPlayerInput::OnPlayerRespawn()
{
	m_inputFlags = 0;
	m_mouseDeltaRotation = ZERO;
	m_lookOrientation = IDENTITY;
}

void CPlayerInput::HandleInputFlagChange(TInputFlags flags, int activationMode, EInputFlagType type)
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

	NetMarkAspectsDirty(kInputAspect);
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

bool CPlayerInput::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (aspect == kInputAspect)
	{
		ser.BeginGroup("PlayerInput");

		auto inputs = m_inputFlags;
		auto prevState = m_inputFlags;

		ser.Value("m_inputFlags", m_inputFlags, 'ui8');

		if (ser.IsReading())
		{
			auto changedKeys = inputs ^ m_inputFlags;

			auto pressedKeys = changedKeys & inputs;
			if (pressedKeys != 0)
			{
				HandleInputFlagChange(pressedKeys, eIS_Pressed);
			}

			auto releasedKeys = changedKeys & prevState;
			if (releasedKeys != 0)
			{
				HandleInputFlagChange(pressedKeys, eIS_Released);
			}
		}

		// Serialize the player look orientation
		ser.Value("m_lookOrientation", m_lookOrientation, 'ori3');

		ser.EndGroup();
	}

	return true;
}

struct SvInputParams
{
	Vec3 pos;
	void SerializeWith(TSerialize ser)
	{
		ser.Value("pos", pos, 'wrld');
	}
};

bool CPlayerInput::OnActionShoot(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode != eIS_Pressed)
		return false;

	static bool toServer = true;
	if (toServer)
	{
		SRmi<RMI_WRAP(&CPlayerInput::SvInput)>::Invoke(this,
			SvInputParams{ GetEntity()->GetPos() }, eRMI_ToServer);
	}
	else
	{
		SRmi<RMI_WRAP(&CPlayerMovement::SvMovement)>::Invoke(
			GetEntity()->GetComponent<CPlayerMovement>(),
			MovementParams{ GetEntity()->GetPos() }, eRMI_ToServer);
	}
	toServer = !toServer;

	return true;
}

bool CPlayerInput::SvInput(SvInputParams&& p, INetChannel *)
{
	m_pPlayer->DisplayText(p.pos, stack_string().Format("%s -> Server",
		GetEntity()->GetName()));
	return true;
}
