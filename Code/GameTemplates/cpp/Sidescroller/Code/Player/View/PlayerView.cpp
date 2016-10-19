#include "StdAfx.h"
#include "PlayerView.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"
#include "Player/Movement/PlayerMovement.h"

#include <IViewSystem.h>
#include <CryAnimation/ICryAnimation.h>

CPlayerView::CPlayerView()
{
}

CPlayerView::~CPlayerView()
{
	GetGameObject()->ReleaseView(this);
}

void CPlayerView::PostInit(IGameObject *pGameObject)
{
	m_pPlayer = static_cast<CPlayer *>(pGameObject->QueryExtension("Player"));

	// Register for UpdateView callbacks
	GetGameObject()->CaptureView(this);
}

void CPlayerView::UpdateView(SViewParams &viewParams)
{
	IEntity &entity = *GetEntity();

	// Start with matching view to the entity position
	viewParams.position = entity.GetWorldPos();

	// Offset camera to the side
	viewParams.position += entity.GetWorldRotation().GetColumn0() * m_pPlayer->GetCVars().m_viewDistanceFromPlayer;

	// Get the direction from camera to player entity
	Vec3 directionToPlayer = (entity.GetWorldPos() - viewParams.position).GetNormalized();

	// Create rotation, facing the player
	viewParams.rotation = Quat::CreateRotationVDir(directionToPlayer);

	// Offset the view upwards, note that this is done after rotation update to not angle the view downwards
	viewParams.position.z += m_pPlayer->GetCVars().m_viewOffsetZ;
}