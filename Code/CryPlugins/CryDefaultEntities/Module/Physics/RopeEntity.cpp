#include "StdAfx.h"
#include "RopeEntity.h"

class CRopeEntityRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("Rope") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class Rope, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CNativeEntityBase>("Rope", "Physics");
	}
};

CRopeEntityRegistrator g_ropeRegistrator;
