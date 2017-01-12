// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/BasicTypes.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;
// Forward declare structures.
struct SUpdateContext;

class CEntityUtilsComponent final : public CComponent
{
public:

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	// ~CComponent

	ExplicitEntityId GetEntityId() const;

	static void      ReflectType(CTypeDesc<CEntityUtilsComponent>& desc);
	static void      Register(IEnvRegistrar& registrar);
};

} // Schematyc
