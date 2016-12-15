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
	enum ePhysicType
	{
		ePhysicType_Static,
		ePhysicType_Rigid,
		ePhysicType_Living
	};

	struct SProperties
	{
		void Serialize(Serialization::IArchive& archive);

		bool bEnabled = true;
		float mass = -1.0f;
		float density = -1.0f;
		ePhysicType type = ePhysicType_Rigid;
		
		//only needed for ePhysicType_Living
		float colliderHeight = 0.45f;
		float colliderRadius = 0.45f;
		float colliderVerticalOffset = 0.0f;
		float friction = 0.95f;
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
