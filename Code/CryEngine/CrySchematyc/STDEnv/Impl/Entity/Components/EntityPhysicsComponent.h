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

	struct SCollider
	{
		static void ReflectType(CTypeDesc<SCollider>& desc);

		ePhysicType type = ePhysicType_Rigid;

		//only needed for ePhysicType_Living
		float height = 0.45f;
		float radius = 0.45f;
		float verticalOffset = 0.0f;
		float friction = 0.95f;
		float gravity = 9.81f;
	};

	// CComponent
	virtual void Run(ESimulationMode simulationMode) override;
	virtual bool Init() override;
	virtual void Shutdown() override;
	// ~CComponent

	void        OnGeometryChanged();
	void        SetPhysicalize(bool bActive);
	void        SetEnabled(bool bEnable);

	static void ReflectType(CTypeDesc<CEntityPhysicsComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

private:

	float            m_mass = -1.0f;
	float            m_density = -1.0f;
	bool             m_bEnabled = true;
	SCollider        m_collider;

	CConnectionScope m_connectionScope;
};

} // Schematyc
