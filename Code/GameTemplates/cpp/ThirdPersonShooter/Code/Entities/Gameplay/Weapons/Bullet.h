#pragma once

#include "Entities/Helpers/NativeEntityBase.h"

////////////////////////////////////////////////////////
// Physicalized bullet shot from weaponry, expires on collision with another object
////////////////////////////////////////////////////////
class CBullet 
	: public CGameObjectExtensionHelper<CBullet, CNativeEntityBase>
{
public:
	virtual ~CBullet() {}

	// ISimpleExtension	
	virtual void PostInit(IGameObject *pGameObject) override;
	virtual void HandleEvent(const SGameObjectEvent &event) override;
	// ~ISimpleExtension

protected:
	void Physicalize();
};