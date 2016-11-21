#include "StdAfx.h"
#include "RopeEntity.h"

class CRopeEntityRegistrator
	: public IEntityRegistrator
{
	static void RegisterIfNecessary(const char* szName, const char* szEditorPath)
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass(szName) != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class %s, overridden by game", szName);
			return;
		}
		RegisterEntityWithDefaultComponent<CNativeEntityBase>(szName, szEditorPath);
	}

	virtual void Register() override
	{
		RegisterIfNecessary("Rope", "Physics");
		RegisterIfNecessary("RopeEntity", "Physics");
	}
};

CRopeEntityRegistrator g_ropeRegistrator;
