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

	const int requiredEvents[] = { eGFE_BecomeLocalPlayer };
	pGameObject->RegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));

	// Default view rotation to the entity's orientation
	m_viewRotation = GetEntity()->GetWorldRotation();
}

void CPlayerView::HandleEvent(const SGameObjectEvent &event)
{
	if (event.event == eGFE_BecomeLocalPlayer)
	{
		// Register for UpdateView callbacks
		GetGameObject()->CaptureView(this);
	}
}

void CPlayerView::UpdateView(SViewParams &viewParams)
{
	IEntity &entity = *GetEntity();

	// Start with changing view rotation to the requested mouse look orientation
	viewParams.rotation = Quat(m_pPlayer->GetInput()->GetLookOrientation());
	m_viewRotation = viewParams.rotation;

	// Start with matching view to the entity position
	viewParams.position = entity.GetWorldPos() - viewParams.rotation.GetColumn1() * m_pPlayer->GetCVars().m_viewDistance;
}