#include "StdAfx.h"
#include "PlayerMovement.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"
#include "Player/View/PlayerView.h"

#include <CryNetwork/Rmi.h>

CRYREGISTER_CLASS(CPlayerMovement);

CPlayerMovement::CPlayerMovement()
{
}

void CPlayerMovement::Initialize()
{
	m_pPlayer = GetEntity()->GetComponent<CPlayer>();

	MovementParams p{ Vec3{0} };

	SRmi<RMI_WRAP(&CPlayerMovement::SvMovement)>::Register(this, eRAT_NoAttach, true, eNRT_ReliableOrdered);
	SRmi<RMI_WRAP(&CPlayerMovement::ClMovementFinal)>::Register(this, eRAT_NoAttach, true, eNRT_ReliableOrdered);
}

void CPlayerMovement::Physicalize()
{
	// Physicalize the player as type Living.
	// This physical entity type is specifically implemented for players
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_RIGID;

	physParams.mass = m_pPlayer->GetCVars().m_mass;

	GetEntity()->Physicalize(physParams);

	// By default, the server delegates authority to a single Player-entity on the client.
	// However, we want the player physics to be simulated server-side, so we need
	// to prevent physics aspect delegation.
	GetEntity()->GetNetEntity()->EnableDelegatableAspect(eEA_Physics, false);
}

uint64 CPlayerMovement::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_UPDATE)
	;
};

void CPlayerMovement::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_UPDATE:
		{
			SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];
			Update(*pCtx);
		}
		break;
	}
}

void CPlayerMovement::Update(SEntityUpdateContext &ctx)
{
	IEntity &entity = *GetEntity();
	IPhysicalEntity *pPhysicalEntity = entity.GetPhysics();
	if(pPhysicalEntity == nullptr)
		return;

	// Don't update movement when view isn't there: player spawned, but hasn't become local player.
	if (!m_pPlayer->GetView())
		return;

	// Simulate physics only on the server for now.
	if (!gEnv->bServer)
		return;

	// Update movement
	pe_action_impulse impulseAction;
	impulseAction.impulse = ZERO;

	const auto inputFlags = m_pPlayer->GetInput()->GetInputFlags();
	const auto &viewRotation = m_pPlayer->GetInput()->GetLookOrientation();

	// Go through the inputs and apply an impulse corresponding to view rotation
	if (inputFlags & CPlayerInput::eInputFlag_MoveForward)
	{
		impulseAction.impulse += viewRotation.GetColumn1() * m_pPlayer->GetCVars().m_moveImpulseStrength;
	}
	if (inputFlags & CPlayerInput::eInputFlag_MoveBack)
	{
		impulseAction.impulse -= viewRotation.GetColumn1() * m_pPlayer->GetCVars().m_moveImpulseStrength;
	}
	if (inputFlags & CPlayerInput::eInputFlag_MoveLeft)
	{
		impulseAction.impulse -= viewRotation.GetColumn0() * m_pPlayer->GetCVars().m_moveImpulseStrength;
	}
	if (inputFlags & CPlayerInput::eInputFlag_MoveRight)
	{
		impulseAction.impulse += viewRotation.GetColumn0() * m_pPlayer->GetCVars().m_moveImpulseStrength;
	}

	// Only dispatch the impulse to physics if one was provided
	if (!impulseAction.impulse.IsZero())
	{
		// Multiply by frame time to keep consistent across machines
		impulseAction.impulse *= ctx.fFrameTime;

		pPhysicalEntity->Action(&impulseAction);
	}
}

bool CPlayerMovement::SvMovement(MovementParams&& p, INetChannel *pChannel)
{
	SRmi<RMI_WRAP(&CPlayerMovement::ClMovementFinal)>::Invoke(this,
		std::move(p), eRMI_ToAllClients);
	return true;
}

bool CPlayerMovement::ClMovementFinal(MovementParams&& p, INetChannel *)
{
	m_pPlayer->DisplayText(p.pos, stack_string().Format("%s -> Everyone",
		GetEntity()->GetName()));
	return true;
}
