#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage movement
////////////////////////////////////////////////////////
class CPlayerMovement : public CGameObjectExtensionHelper<CPlayerMovement, ISimpleExtension>
{
public:
	CPlayerMovement();
	virtual ~CPlayerMovement() {}

	//ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;

	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override;
	//~ISimpleExtension

	void Physicalize();

	void RequestMove(const Vec3 &direction);

	void SetVelocity(const Vec3 &velocity);
	Vec3 GetVelocity();

	// Gets the requested movement direction based on input data
	bool IsOnGround() const { return m_bOnGround; }
	Vec3 GetGroundNormal() const { return m_groundNormal; }

protected:
	// Get the stats from latest physics thread update
	void GetLatestPhysicsStats(IPhysicalEntity &physicalEntity);
	
protected:
	CPlayer *m_pPlayer;

	bool m_bOnGround;
	Vec3 m_groundNormal;
};
