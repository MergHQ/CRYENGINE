#include "StdAfx.h"
#include "PlayerMovement.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"
#include "Player/View/PlayerView.h"

CPlayerMovement::CPlayerMovement()
{
}

void CPlayerMovement::PostInit(IGameObject *pGameObject)
{
	m_pPlayer = static_cast<CPlayer *>(pGameObject->QueryExtension("Player"));

	// Make sure that this extension is updated regularly via the Update function below
	pGameObject->EnableUpdateSlot(this, 0);
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

void CPlayerMovement::Update(SEntityUpdateContext &ctx, int updateSlot)
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