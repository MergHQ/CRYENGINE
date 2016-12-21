// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>
#include <CrySerialization/Forward.h>

namespace Schematyc
{
	// Forward declare interfaces.
	struct IEnvRegistrar;
	// Forward declare structures.
	struct SUpdateContext;

	class CEntityMovementComponent final : public CComponent
	{
	public:
		enum eMoveModifier
		{
			eMoveModifier_None = 0,
			eMoveModifier_IgnoreX = BIT(0),
			eMoveModifier_IgnoreY = BIT(1),
			eMoveModifier_IgnoreZ = BIT(2),
			eMoveModifier_IgnoreXandY = eMoveModifier_IgnoreX | eMoveModifier_IgnoreY,
			eMoveModifier_IgnoreNull = BIT(3),
		};

		// CComponent
		virtual bool Init() override;
		virtual void Run(ESimulationMode simulationMode) override;
		virtual void Shutdown() override;
		// ~CComponent

		void         Move(const Vec3& velocity, eMoveModifier moveFlags);
		void         SetRotation(const CRotation& rotation);
		void         Teleport(const CTransform& transform);

		static SGUID ReflectSchematycType(CTypeInfo<CEntityMovementComponent>& typeInfo);
		static void  Register(IEnvRegistrar& registrar);

	private:
		void Update(const SUpdateContext& updateContext);

		void HandleModifier(IPhysicalEntity* pPhysics);

		eMoveModifier    m_moveModifier = eMoveModifier_None;
		Vec3             m_velocity = Vec3(ZERO);
		CConnectionScope m_connectionScope;
	};
} // Schematyc
