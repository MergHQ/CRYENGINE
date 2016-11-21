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

protected:
	CPlayer *m_pPlayer;
};
