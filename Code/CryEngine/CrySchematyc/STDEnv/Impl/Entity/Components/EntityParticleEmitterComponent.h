// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>
#include <Schematyc/Types/ResourceTypes.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityParticleEmitterComponent final : public CComponent
{
private:

	struct SProperties
	{
		SProperties();

		void Serialize(Serialization::IArchive& archive);

		ParticleEffectName effectName;
		float              scale;
		bool               bVisible;
		bool               bPrime;
		float              countScale;
		float              speedScale;
		float              timeScale;
		float              pulsePeriod;
		float              strength;
	};

public:

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void        SetTransform(const CTransform& transform);
	void        SetVisible(bool bVisible);
	bool        IsVisible() const;

	static void ReflectType(CTypeDesc<CEntityParticleEmitterComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

private:

	void LoadParticleEmitter();

private:

	int  m_slot = EmptySlot;
	bool m_bVisible = false;
};

} // Schematyc
