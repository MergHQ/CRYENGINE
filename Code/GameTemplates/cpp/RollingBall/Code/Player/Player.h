#pragma once

#include <CryEntitySystem/IEntityComponent.h>
#include <CryMath/Cry_Camera.h>

class CPlayerInput;
class CPlayerMovement;
class CPlayerView;
class CPlayerAnimations;

class CPlayer;

class CSpawnPoint;

struct ISimpleWeapon;

////////////////////////////////////////////////////////
// Represents a player participating in gameplay
////////////////////////////////////////////////////////
class CPlayer : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CPlayer,
		"CPlayer", 0xC11F7CF4FEF448D8, 0x8A5D66122D945460);

public:
	enum EGeometrySlots
	{
		eGeometry_ThirdPerson = 0,
	};

	struct SExternalCVars
	{
		float m_mass;
		float m_moveImpulseStrength;

		float m_rotationSpeedYaw;
		float m_rotationSpeedPitch;

		float m_rotationLimitsMinPitch;
		float m_rotationLimitsMaxPitch;

		float m_viewDistance;

		ICVar *m_pGeometry;
	};

public:
	CPlayer();
	virtual ~CPlayer();

	virtual void Initialize() override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual uint64 GetEventMask() const override;

	void Respawn();
	void DisplayText(const Vec3 &pos, const char *text);

	CPlayerInput *GetInput() const { return m_pInput; }
	CPlayerMovement *GetMovement() const { return m_pMovement; }
	CPlayerView *GetView() const { return m_pView; }

	const SExternalCVars &GetCVars() const;

protected:
	void SelectSpawnPoint();
	void SetPlayerModel();

protected:
	CPlayerInput *m_pInput;
	CPlayerMovement *m_pMovement;
	CPlayerView *m_pView;
	CPlayerAnimations *m_pAnimations;
};
