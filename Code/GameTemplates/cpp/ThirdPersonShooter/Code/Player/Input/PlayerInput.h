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
	enum EInputFlagType
	{
		eInputFlagType_Hold = 0,
		eInputFlagType_Toggle
	};

public:
	typedef uint8 TInputFlags;

	enum EInputFlags
		: TInputFlags
	{
		eInputFlag_MoveLeft = 1 << 0,
		eInputFlag_MoveRight = 1 << 1,
		eInputFlag_MoveForward = 1 << 2,
		eInputFlag_MoveBack = 1 << 3
	};

public:
	virtual ~CPlayerInput() {}

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot) override;
	// ~ISimpleExtension

	// IActionListener
	virtual void OnAction(const ActionId &action, int activationMode, float value) override;
	// ~IActionListener

	void OnPlayerRespawn();

	const TInputFlags GetInputFlags() const { return m_inputFlags; }
	const Vec2 GetMouseDeltaRotation() const { return m_mouseDeltaRotation; }

	const Quat &GetLookOrientation() const { return m_lookOrientation; }

protected:
	void InitializeActionHandler();

	void HandleInputFlagChange(EInputFlags flags, int activationMode, EInputFlagType type = eInputFlagType_Hold);

	// Start actions below
protected:
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionMouseRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMouseRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	
	bool OnActionShoot(EntityId entityId, const ActionId& actionId, int activationMode, float value);

protected:
	CPlayer *m_pPlayer;

	TInputFlags m_inputFlags;

	Vec2 m_mouseDeltaRotation;

	// Should translate to head orientation in the future
	Quat m_lookOrientation;

	// Handler for actionmap events that maps actions to callbacks
	TActionHandler<CPlayerInput> m_actionHandler;
};
