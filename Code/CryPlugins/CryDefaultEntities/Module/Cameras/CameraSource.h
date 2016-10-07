#pragma once

#include "Helpers/ISimpleExtension.h"

////////////////////////////////////////////////////////
// Sample entity for creating a camera source
////////////////////////////////////////////////////////
class CCameraSource : public ISimpleExtension
{
	virtual void PostInit(IGameObject* pGameObject) override;
};
