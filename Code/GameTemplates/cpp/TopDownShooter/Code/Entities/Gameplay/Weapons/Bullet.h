#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

////////////////////////////////////////////////////////
// Physicalized bullet shot from weaponry, expires on collision with another object
////////////////////////////////////////////////////////
class CBullet 
	: public ISimpleExtension
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