#pragma once

#include "Player/ISimpleActor.h"

#include <CryMath/Cry_Camera.h>

class CPlayerInput;
class CPlayerMovement;
class CPlayerView;

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
	CPlayer();
	virtual ~CPlayer();

	// ISimpleActor
	virtual bool Init(IGameObject* pGameObject) override;
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void ProcessEvent(SEntityEvent& event) override;

	virtual void SetHealth(float health) override;
	// ~ISimpleActor

	CPlayerInput *GetInput() const { return m_pInput; }
	CPlayerMovement *GetMovement() const { return m_pMovement; }

protected:
	CPlayerInput *m_pInput;
	CPlayerMovement *m_pMovement;
	CPlayerView *m_pView;
};
