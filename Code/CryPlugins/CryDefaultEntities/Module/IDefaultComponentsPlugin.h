#pragma once
#include <CrySystem/ICryPlugin.h>
#include "DefaultComponents/Cameras/ICameraManager.h"

class IPlugin_CryDefaultEntities
	: public Cry::IEnginePlugin
	, public ISystemEventListener
{
public:
	CRYINTERFACE_DECLARE_GUID(IPlugin_CryDefaultEntities, "{CB9E7C85-3289-41B6-983A-6A076ABA6351}"_cry_guid);

	virtual ~IPlugin_CryDefaultEntities() { };

	virtual ICameraManager* GetICameraManager() = 0;
};