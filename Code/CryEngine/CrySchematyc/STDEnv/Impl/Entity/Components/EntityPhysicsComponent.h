// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <CrySerialization/Forward.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityPhysicsComponent final : public CComponent
{
public:
	struct SProperties
	{
		void Serialize(Serialization::IArchive& archive);
		
		bool bEnabled = true;
		bool bStatic = false;
		float mass = -1.0f;
		float density = -1.0f;
	};

	// CComponent
	virtual void Run(ESimulationMode simulationMode) override;
	virtual bool Init() override;
	virtual void Shutdown() override;
	// ~CComponent
		
	void OnGeometryChanged();
	void SetPhysicalize(bool bActive);
	void SetEnabled(bool bEnable);

	static SGUID ReflectSchematycType(CTypeInfo<CEntityPhysicsComponent>& typeInfo);
	static void  Register(IEnvRegistrar& registrar);

private:
	CConnectionScope    m_connectionScope;
};
} // Schematyc
