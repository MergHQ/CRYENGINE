#include "StdAfx.h"
#include "PlayerMovement.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"

CPlayerMovement::CPlayerMovement()
	: m_bOnGround(false)
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
	physParams.type = PE_LIVING;

	physParams.mass = m_pPlayer->GetCVars().m_mass;

	pe_player_dimensions playerDimensions;

	// Prefer usage of a cylinder instead of capsule
	playerDimensions.bUseCapsule = 0;

	// Specify the size of our cylinder
	playerDimensions.sizeCollider = Vec3(0.45f, 0.45f, m_pPlayer->GetCVars().m_playerEyeHeight * 0.5f);

	// Keep pivot at the player's feet (defined in player geometry) 
	playerDimensions.heightPivot = 0.f;
	// Offset collider upwards
	playerDimensions.heightCollider = 1.f;
	playerDimensions.groundContactEps = 0.004f;

	physParams.pPlayerDimensions = &playerDimensions;
	
	pe_player_dynamics playerDynamics;
	playerDynamics.kAirControl = 0.f;
	playerDynamics.mass = physParams.mass;

	physParams.pPlayerDynamics = &playerDynamics;

	GetEntity()->Physicalize(physParams);
}

void CPlayerMovement::RequestMove(const Vec3 &direction)
{
	if (auto *pPhysicalEntity = GetEntity()->GetPhysics())
	{
		pe_action_move moveAction;
		// Add dir to velocity
		moveAction.iJump = 2;
		moveAction.dir = direction;

		// Dispatch the movement request
		pPhysicalEntity->Action(&moveAction);
	}
}

void CPlayerMovement::SetVelocity(const Vec3 &velocity)
{
	if (auto *pPhysicalEntity = GetEntity()->GetPhysics())
	{
		pe_action_move moveAction;
		// Change velocity instantaneously
		moveAction.iJump = 1;

		moveAction.dir = velocity;

		// Dispatch the movement request
		pPhysicalEntity->Action(&moveAction);
	}
}

Vec3 CPlayerMovement::GetVelocity()
{
	if (auto *pPhysicalEntity = GetEntity()->GetPhysics())
	{
		pe_status_living status;
		if (pPhysicalEntity->GetStatus(&status))
			return status.vel;
	}

	return ZERO;
}

void CPlayerMovement::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	IEntity &entity = *GetEntity();
	IPhysicalEntity *pPhysicalEntity = entity.GetPhysics();
	if(pPhysicalEntity == nullptr)
		return;

	// Obtain stats from the living entity implementation
	GetLatestPhysicsStats(*pPhysicalEntity);
}

void CPlayerMovement::GetLatestPhysicsStats(IPhysicalEntity &physicalEntity)
{
	pe_status_living livingStatus;
	if(physicalEntity.GetStatus(&livingStatus) != 0)
	{
		m_bOnGround = !livingStatus.bFlying;

		// Store the ground normal in case it is needed
		// Note that users have to check if we're on ground before using, is considered invalid in air.
		m_groundNormal = livingStatus.groundSlope;
	}
}