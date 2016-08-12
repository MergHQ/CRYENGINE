#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage the local client's view / camera
////////////////////////////////////////////////////////
class CPlayerView 
	: public CGameObjectExtensionHelper<CPlayerView, ISimpleExtension>
	, public IGameObjectView
{
public:
	CPlayerView();
	virtual ~CPlayerView();

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;

	virtual void HandleEvent(const SGameObjectEvent &event) override;
	// ~ISimpleExtension

	// IGameObjectView
	virtual void UpdateView(SViewParams &viewParams) override;
	virtual void PostUpdateView(SViewParams &viewParams) override {}
	// ~IGameObjectView

protected:
	CPlayer *m_pPlayer;
};
