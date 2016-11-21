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
private:

	struct SProperties
	{
		void Serialize(Serialization::IArchive& archive);

		GeomFileName fileName;
	};

public:

	CEntityGeomComponent();

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void         Set(const GeomFileName& fileName);
	void         SetTransform(const CTransform& transform);
	void         SetVisible(bool bVisible);
	bool         IsVisible() const;

	static SGUID ReflectSchematycType(CTypeInfo<CEntityGeomComponent>& typeInfo);
	static void  Register(IEnvRegistrar& registrar);

private:

	int m_slot;
};
} // Schematyc
