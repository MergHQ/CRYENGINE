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

	virtual void Unregister() override {}
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
	// Register for UpdateView callbacks
	GetGameObject()->CaptureView(this);
}

void CPlayerView::UpdateView(SViewParams &viewParams)
{
	viewParams.position = GetEntity()->GetWorldPos();
	viewParams.rotation = GetEntity()->GetWorldRotation();
}