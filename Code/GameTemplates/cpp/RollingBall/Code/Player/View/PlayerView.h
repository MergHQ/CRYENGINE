#pragma once

#include <CryEntitySystem/IEntityComponent.h>
#include <IGameObject.h>	// only for IGameObjectView
#include <IViewSystem.h>

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage the local client's view / camera
////////////////////////////////////////////////////////
class CPlayerView 
	: public IEntityComponent
	, public IGameObjectView
{
public:
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CPlayerView,
		"CPlayerView", 0x857FB306FC0E4DDA, 0x9E5E1D0FFD20B0A1);

	CPlayerView();
	virtual ~CPlayerView();

	// IEntityComponent
	virtual void Initialize() override;
	virtual uint64 GetEventMask() const override;
	virtual void ProcessEvent(SEntityEvent& event) override;

	void InitLocalPlayer();

	// IGameObjectView
	virtual void UpdateView(SViewParams &viewParams) override;
	virtual void PostUpdateView(SViewParams &viewParams) override {}
	// ~IGameObjectView

	const Quat &GetViewRotation() const { return m_viewRotation; }

protected:
	CPlayer *m_pPlayer;
	IView *m_pView;

	Quat m_viewRotation;
};
