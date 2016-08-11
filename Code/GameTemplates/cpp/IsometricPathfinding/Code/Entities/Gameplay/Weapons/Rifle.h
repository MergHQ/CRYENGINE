#pragma once

#include "ISimpleWeapon.h"

////////////////////////////////////////////////////////
// Rifle weapon entity that shoots entities as bullets
////////////////////////////////////////////////////////
class CRifle 
	: public CGameObjectExtensionHelper<CRifle, ISimpleWeapon>
{
public:
	virtual ~CRifle() {}

	// ISimpleExtension	
	virtual void PostInit(IGameObject *pGameObject) override;
	// ~ISimpleExtension

	// IWeapon
	virtual void RequestFire() override;
	// ~IWeapon
};