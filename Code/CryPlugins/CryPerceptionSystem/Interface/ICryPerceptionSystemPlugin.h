// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct IPerceptionManager;

struct ICryPerceptionSystemPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(ICryPerceptionSystemPlugin, 0x30c9bf81e5fd42df, 0xac3cbc5fc1e6f0b4);

public:
	virtual IPerceptionManager& GetPerceptionManager() const = 0;
};
