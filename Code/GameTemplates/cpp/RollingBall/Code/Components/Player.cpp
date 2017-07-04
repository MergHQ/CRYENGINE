#include "StdAfx.h"
#include "Player.h"
#include "SpawnPoint.h"

#include <CryRenderer/IRenderAuxGeom.h>

#include <CryNetwork/Rmi.h>

namespace {
	static void RegisterCPlayerComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPlayerComponent));
		}
	}

	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterCPlayerComponent);
}

void CPlayerComponent::Initialize()
{
	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are m_pEntity
	// Network: input component is synced, so it has to present on both sides.
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	// CryNetwork/CryPhysics: the entity has to be physicalized on both sides
	// *prior* to binding to network, so the physical state is synced properly.
	m_pEntity->LoadGeometry(GetOrMakeEntitySlotId(), "Objects/Default/primitive_sphere.cgf");

	SEntityPhysicalizeParams physParams;
	physParams.type = PE_RIGID;
	physParams.mass = 90.f;
	m_pEntity->Physicalize(physParams);

	// By default, the server delegates authority to a single Player-entity on the client.
	// However, we want the player physics to be simulated server-side, so we need
	// to prevent physics aspect delegation.
	// This should be done on both sides.
	m_pEntity->GetNetEntity()->EnableDelegatableAspect(eEA_Physics, false);

	SRmi<RMI_WRAP(&CPlayerComponent::SvJump)>::Register(this, eRAT_NoAttach, true, eNRT_ReliableOrdered);
}

void CPlayerComponent::LocalPlayerInitialize()
{
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();

	// Register an action, and the callback that will be sent when it's m_pEntity
	m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveLeft, activationMode);  });
	// Bind the 'A' key the "moveleft" action
	m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse,	EKeyId::eKI_A);

	m_pInputComponent->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveRight, activationMode);  });
	m_pInputComponent->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

	m_pInputComponent->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveForward, activationMode);  });
	m_pInputComponent->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

	m_pInputComponent->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveBack, activationMode);  });
	m_pInputComponent->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

	m_pInputComponent->RegisterAction("player", "mouse_rotateyaw", [this](int activationMode, float value) { m_mouseDeltaRotation.x -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotateyaw", eAID_KeyboardMouse, EKeyId::eKI_MouseX);

	m_pInputComponent->RegisterAction("player", "mouse_rotatepitch", [this](int activationMode, float value) { m_mouseDeltaRotation.y -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotatepitch", eAID_KeyboardMouse, EKeyId::eKI_MouseY);

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "jump", [this](int activationMode, float value)
	{
		if (activationMode == eIS_Pressed)
		{
			// RMI is used here only for demo purposes, the actual jump action
			// can be sent with input flags as well.
			SRmi<RMI_WRAP(&CPlayerComponent::SvJump)>::InvokeOnServer(this,
				MovementParams{ m_pEntity->GetPos() });
		}
	});

	// Bind the shoot action to left mouse click
	m_pInputComponent->BindAction("player", "jump", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);
}

uint64 CPlayerComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME)
		| BIT64(ENTITY_EVENT_UPDATE)
		| BIT64(ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER)
	;
}

void CPlayerComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_START_GAME:
	{
		// Revive the entity when gameplay starts
		Revive();
	}
	break;
	case ENTITY_EVENT_NET_BECOME_LOCAL_PLAYER:
	{
		LocalPlayerInitialize();
	}
	break;
	case ENTITY_EVENT_UPDATE:
	{
		SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];
		
		// Camera components exists only for the local player
		if (m_pCameraComponent)
		{
			// Update the camera component offset
			UpdateCamera(pCtx->fFrameTime);
		}
		
		if (gEnv->bServer) // Simulate physics only on the server for now.
		{
			// Start by updating the movement request we want to send to the character controller
			// This results in the physical representation of the character moving
			UpdateMovementRequest(pCtx->fFrameTime);
		}
	}
	break;
	}
}

void CPlayerComponent::UpdateMovementRequest(float frameTime)
{
	if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
	{
		Vec3 velocity = ZERO;

		const float moveImpulseStrength = 800.f;

		// Look orientation has been replicated from the client's camera
		auto cameraTransformation = m_lookOrientation;

		// Update movement
		pe_action_impulse impulseAction;
		impulseAction.impulse = ZERO;

		if (m_inputFlags & (TInputFlags)EInputFlag::MoveLeft)
		{
			impulseAction.impulse -= cameraTransformation.GetColumn0() * moveImpulseStrength;
		}
		if (m_inputFlags & (TInputFlags)EInputFlag::MoveRight)
		{
			impulseAction.impulse += cameraTransformation.GetColumn0() * moveImpulseStrength;
		}
		if (m_inputFlags & (TInputFlags)EInputFlag::MoveForward)
		{
			impulseAction.impulse += cameraTransformation.GetColumn1() * moveImpulseStrength;
		}
		if (m_inputFlags & (TInputFlags)EInputFlag::MoveBack)
		{
			impulseAction.impulse -= cameraTransformation.GetColumn1() * moveImpulseStrength;
		}

		// Only dispatch the impulse to physics if one was provided
		if (!impulseAction.impulse.IsZero())
		{
			// Multiply by frame time to keep consistent across machines
			impulseAction.impulse *= frameTime;

			pPhysicalEntity->Action(&impulseAction);
		}
	}
}

void CPlayerComponent::UpdateCamera(float frameTime)
{
	// Start with updating look orientation from the latest input
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	if (!m_mouseDeltaRotation.IsZero())
	{
		const float rotationSpeed = 0.002f;

		ypr.x += m_mouseDeltaRotation.x * rotationSpeed;

		const float rotationLimitsMinPitch = -0.84f;
		const float rotationLimitsMaxPitch = 1.5f;

		// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
		ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);

		// Look direction needs to be synced to server to calculate the movement in
		// the right direction.
		m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));
		NetMarkAspectsDirty(kInputAspect);

		// Reset every frame
		m_mouseDeltaRotation = ZERO;
	}

	// Start with changing view rotation to the requested mouse look orientation
	Matrix34 localTransform = IDENTITY;
	localTransform.SetRotation33(Matrix33(m_pEntity->GetWorldRotation().GetInverted()) * CCamera::CreateOrientationYPR(ypr));

	const float viewDistance = 10.f;

	// Offset the player along the forward axis (normally back)
	// Also offset upwards
	localTransform.SetTranslation(-localTransform.GetColumn1() * viewDistance);

	m_pCameraComponent->SetTransformMatrix(localTransform);
}

void CPlayerComponent::Revive()
{
	// Unhide the entity in case hidden by the Editor
	m_pEntity->Hide(false);
}

void CPlayerComponent::HandleInputFlagChange(TInputFlags flags, int activationMode, EInputFlagType type)
{
	switch (type)
	{
	case EInputFlagType::Hold:
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
	case EInputFlagType::Toggle:
	{
		if (activationMode == eIS_Released)
		{
			// Toggle the bit(s)
			m_inputFlags ^= flags;
		}
	}
	break;
	}

	// Input is replicated from the client to the server.
	if (!gEnv->bServer)
	{
		NetMarkAspectsDirty(kInputAspect);
	}
}

bool CPlayerComponent::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
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


bool CPlayerComponent::SvJump(MovementParams&& p, INetChannel *pChannel)
{
	if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
	{
		pe_action_impulse impulse;
		impulse.impulse = Vec3(0, 0, 800);
		pPhysicalEntity->Action(&impulse);
	}
	return true;
}
