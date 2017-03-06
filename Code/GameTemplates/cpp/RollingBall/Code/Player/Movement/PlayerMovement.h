#pragma once
#include <CryEntitySystem/IEntityComponent.h>

class CPlayer;

// Declared in the header so PlayerInput could send them.
struct MovementParams
{
	Vec3 pos;
	void SerializeWith(TSerialize ser)
	{
		ser.Value("pos", pos, 'wrld');
	}
};

////////////////////////////////////////////////////////
// Player extension to manage movement
////////////////////////////////////////////////////////
class CPlayerMovement : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CPlayerMovement,
		"CPlayerMovement", 0x1B9F01EDDDA6459B, 0x99CFCFE44E576AA8);
public:
	CPlayerMovement();
	virtual ~CPlayerMovement() {}

	// IEntityComponent
	virtual void Initialize() override;
	virtual void ProcessEvent(SEntityEvent& entityEvent) override;
	virtual uint64 GetEventMask() const override;

	void Update(SEntityUpdateContext& ctx);
	void Physicalize();

	bool SvMovement(MovementParams&& p, INetChannel *);
	bool ClMovementFinal(MovementParams&& p, INetChannel *);

protected:
	CPlayer *m_pPlayer;
};
