#include "StdAfx.h"
#include "PlayerMovement.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"
#include "Player/View/PlayerView.h"

CRYREGISTER_CLASS(CPlayerMovement);

CPlayerMovement::CPlayerMovement()
{
}

void CPlayerMovement::Initialize()
{
	m_pPlayer = GetEntity()->GetComponent<CPlayer>();
}

void CPlayerMovement::Physicalize()
{
	// Physicalize the player as type Living.
	// This physical entity type is specifically implemented for players
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_RIGID;

	physParams.mass = m_pPlayer->GetCVars().m_mass;

	GetEntity()->Physicalize(physParams);
}

uint64 CPlayerMovement::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_UPDATE);
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

	// Update movement
	pe_action_impulse impulseAction;
	impulseAction.impulse = ZERO;

	auto inputFlags = m_pPlayer->GetInput()->GetInputFlags();
	auto &viewRotation = m_pPlayer->GetView()->GetViewRotation();

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