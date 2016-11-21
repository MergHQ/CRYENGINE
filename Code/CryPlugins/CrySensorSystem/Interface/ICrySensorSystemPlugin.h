// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct ISensorSystem;

struct ICrySensorSystemPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(ICrySensorSystemPlugin, 0x43678bb848cd4bb8, 0xae118f6b0a347d9a);

public:

	virtual ISensorSystem& GetSensorSystem() const = 0;
};
