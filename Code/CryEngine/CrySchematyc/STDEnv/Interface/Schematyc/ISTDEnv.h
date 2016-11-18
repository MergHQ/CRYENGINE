// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>
#include <CrySystem/ICryPlugin.h>

namespace Schematyc
{
// Forward declare interfaces.
struct ISpatialIndex;

struct ISTDEnv : public ICryPlugin
{
	CRYINTERFACE_DECLARE(ISTDEnv, 0x2846c43757b24803, 0xa281b1938659395e)
};
} // Schematyc
