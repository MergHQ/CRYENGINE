#pragma once

#include <CryEntitySystem/IEntityComponent.h>
#include <IActionMapManager.h>

class CPlayer;
struct SvInputParams;

////////////////////////////////////////////////////////
// Player extension to manage input
////////////////////////////////////////////////////////
class CPlayerInput 
	: public IEntityComponent
	, public IActionListener
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CPlayerInput,
		"CPlayerInput", 0x1DE317DF05054D59, 0x901CCD149B654765);
	
	enum EInputFlagType
	{
		eInputFlagType_Hold = 0,
		eInputFlagType_Toggle
	};

	const EEntityAspects kInputAspect = eEA_GameClientD;

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
	virtual ~CPlayerInput();

	// IEntityComponent
	virtual void Initialize() override;
	virtual void ProcessEvent(SEntityEvent& entityEvent) override;
	virtual uint64 GetEventMask() const override;
	void Update(SEntityUpdateContext &ctx);

	void InitLocalPlayer();

	// IActionListener
	virtual void OnAction(const ActionId &action, int activationMode, float value) override;
	// ~IActionListener

	void OnPlayerRespawn();

	const TInputFlags GetInputFlags() const { return m_inputFlags; }
	const Vec2 GetMouseDeltaRotation() const { return m_mouseDeltaRotation; }

	const Quat &GetLookOrientation() const { return m_lookOrientation; }

	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
	virtual NetworkAspectType GetNetSerializeAspectMask() const { return kInputAspect; };

protected:
	void InitializeActionHandler();

	void HandleInputFlagChange(TInputFlags flags, int activationMode, EInputFlagType type = eInputFlagType_Hold);

	// Start actions below
protected:
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionMouseRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMouseRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionShoot(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	
	bool SvInput(SvInputParams&& p, INetChannel *);

protected:
	CPlayer *m_pPlayer;

	TInputFlags m_inputFlags;

	Vec2 m_mouseDeltaRotation;

	// Should translate to head orientation in the future
	Quat m_lookOrientation;

	// Handler for actionmap events that maps actions to callbacks
	TActionHandler<CPlayerInput> m_actionHandler;
};
