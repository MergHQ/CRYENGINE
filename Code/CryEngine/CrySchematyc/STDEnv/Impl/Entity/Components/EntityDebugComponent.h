// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;
// Forward declare structures.
struct SUpdateContext;

class CEntityDebugComponent final : public CComponent
{
public:

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	// ~CComponent

	void        DrawText(const CSharedString& text, const Vec2& pos, const ColorF& color);

	static void ReflectType(CTypeDesc<CEntityDebugComponent>& desc);
	static void Register(IEnvRegistrar& registrar);
};

} // Schematyc
