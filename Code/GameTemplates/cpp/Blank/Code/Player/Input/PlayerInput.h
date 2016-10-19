#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage input
////////////////////////////////////////////////////////
class CPlayerInput 
	: public CGameObjectExtensionHelper<CPlayerInput, ISimpleExtension>
	, public IActionListener
{
public:
	virtual ~CPlayerInput() {}

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;
	// ~ISimpleExtension

	// IActionListener
	virtual void OnAction(const ActionId &action, int activationMode, float value) override;
	// ~IActionListener

	void OnPlayerRespawn();

	const Vec2 &GetMovementDirection() const { return m_moveDirection; }

	Vec2 GetAndResetMouseDeltaRotation() 
	{
		auto deltaRotation = m_mouseDeltaRotation;

		m_mouseDeltaRotation = ZERO;
		return deltaRotation;
	}

protected:
	void InitializeActionHandler();

	// Start actions below
protected:
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionMouseRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMouseRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	
protected:
	CPlayer *m_pPlayer;

	Vec3 m_moveDirection;
	Vec2 m_mouseDeltaRotation;

	// Handler for actionmap events that maps actions to callbacks
	TActionHandler<CPlayerInput> m_actionHandler;
};
