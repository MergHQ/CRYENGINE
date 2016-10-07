#include "StdAfx.h"
#include "PlayerView.h"

#include <IViewSystem.h>

#include "GamePlugin.h"

class CPlayerViewRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CGamePlugin::RegisterEntityComponent<CPlayerView>("PlayerView");
	}
};

CPlayerViewRegistrator g_playerViewRegistrator;

CPlayerView::CPlayerView()
{
}

CPlayerView::~CPlayerView()
{
	GetGameObject()->ReleaseView(this);
}

void CPlayerView::PostInit(IGameObject *pGameObject)
{
	const int requiredEvents[] = { eGFE_BecomeLocalPlayer };
	pGameObject->RegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));
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
	viewParams.position = GetEntity()->GetWorldPos();
	viewParams.rotation = GetEntity()->GetWorldRotation();
}