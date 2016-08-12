#pragma once

#include "Player/ISimpleActor.h"

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
class CPlayer 
	: public CGameObjectExtensionHelper<CPlayer, ISimpleActor>
{
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

	// ISimpleActor
	virtual bool Init(IGameObject* pGameObject) override;
	virtual void PostInit(IGameObject* pGameObject) override;

	virtual void HandleEvent(const SGameObjectEvent &event) override;
	virtual void ProcessEvent(SEntityEvent& event) override;

	virtual void SetHealth(float health) override;
	virtual float GetHealth() const override { return m_bAlive ? GetMaxHealth() : 0.f; }
	// ~ISimpleActor

	CPlayerInput *GetInput() const { return m_pInput; }
	CPlayerMovement *GetMovement() const { return m_pMovement; }
	CPlayerView *GetView() const { return m_pView; }

	const bool IsLocalClient() const { return m_bIsLocalClient; }

	const SExternalCVars &GetCVars() const;

protected:
	void SelectSpawnPoint();
	void SetPlayerModel();

protected:
	CPlayerInput *m_pInput;
	CPlayerMovement *m_pMovement;
	CPlayerView *m_pView;
	CPlayerAnimations *m_pAnimations;

	bool m_bIsLocalClient;
	bool m_bAlive;
};
