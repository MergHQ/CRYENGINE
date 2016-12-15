// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>

namespace Schematyc
{
	// Forward declare interfaces.
	struct IEnvRegistrar;
	// Forward declare structures.
	struct SUpdateContext;

	class CEntityMovementComponent final : public CComponent
	{
	public:
		// CComponent
		virtual bool Init() override;
		virtual void Run(ESimulationMode simulationMode) override;
		virtual void Shutdown() override;
		// ~CComponent

		void         Move(const Vec3& velolcity);
		void         SetRotation(const CRotation& rotation);
		void         Teleport(const CTransform& transform);

		static SGUID ReflectSchematycType(CTypeInfo<CEntityMovementComponent>& typeInfo);
		static void  Register(IEnvRegistrar& registrar);

	private:

		void Update(const SUpdateContext& updateContext);

	private:

		Vec3             m_velocity = Vec3(ZERO);
		CConnectionScope m_connectionScope;
	};
} // Schematyc
