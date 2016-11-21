#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage input
////////////////////////////////////////////////////////
class CPlayerInput
	: public CGameObjectExtensionHelper<CPlayerInput, ISimpleExtension>
	, public IActionListener
	, public IGameFrameworkListener
{
public:
	CPlayerInput();
	virtual ~CPlayerInput();

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void ProcessEvent(SEntityEvent &event) override;
	// ~ISimpleExtension

	// IActionListener
	virtual void OnAction(const ActionId &action, int activationMode, float value) override;
	// ~IActionListener

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) override; // To ensure we update the cursor position after setting the modelviewmatrix for that frame
	virtual void OnSaveGame(ISaveGame* pSaveGame) override {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) override {}
	virtual void OnLevelEnd(const char* nextLevel) override {}
	virtual void OnActionEvent(const SActionEvent& event) override {}
	// ~IGameFrameworkListener

	Vec3 GetWorldCursorPosition() const { return m_cursorPositionInWorld; }

protected:
	void SpawnCursorEntity();

	void InitializeActionHandler();

	// Start actions below
protected:
	bool OnActionShoot(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionNavigateTo(EntityId entityId, const ActionId& actionId, int activationMode, float value);

protected:
	CPlayer *m_pPlayer;

	// Handler for actionmap events that maps actions to callbacks
	TActionHandler<CPlayerInput> m_actionHandler;

	Vec3 m_cursorPositionInWorld;

	// Entity used as cursor
	IEntity *m_pCursorEntity;
};
