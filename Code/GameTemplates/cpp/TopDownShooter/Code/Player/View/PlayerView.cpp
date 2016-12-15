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

	// Offset the camera upwards
	viewParams.position.z += m_pPlayer->GetCVars().m_viewDistanceFromPlayer;

	// Create rotation, facing the player
	viewParams.rotation = Quat::CreateRotationX(DEG2RAD(-90));
}