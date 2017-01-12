// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Reflection/TypeDesc.h>
#include <Schematyc/Utils/GUID.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;

struct SStartSignal
{
	static void ReflectType(CTypeDesc<SStartSignal>& desc);
};

struct SStopSignal
{
	static void ReflectType(CTypeDesc<SStopSignal>& desc);
};

struct SUpdateSignal
{
	SUpdateSignal(float _time = 0.0f);

	static void ReflectType(CTypeDesc<SUpdateSignal>& desc);

	float time = 0.0f;
};

void RegisterCoreEnvSignals(IEnvRegistrar& registrar);

}
