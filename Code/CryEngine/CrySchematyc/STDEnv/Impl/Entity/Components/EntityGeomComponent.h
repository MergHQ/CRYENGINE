// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>
#include <Schematyc/Types/ResourceTypes.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityGeomComponent final : public CComponent
{
public:

	// CComponent
	virtual bool  Init() override;
	virtual void  Run(ESimulationMode simulationMode) override;
	virtual void  Shutdown() override;
	virtual int   GetSlot() const override;
	// ~CComponent

	void                Set(const GeomFileName& fileName);
	const GeomFileName& Get() const;
	void                SetTransform(const CTransform& transform);
	void                SetVisible(bool bVisible);
	bool                IsVisible() const;

	static void ReflectType(CTypeDesc<CEntityGeomComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

private:

	GeomFileName m_fileName;

	int          m_slot = EmptySlot;
};

} // Schematyc
