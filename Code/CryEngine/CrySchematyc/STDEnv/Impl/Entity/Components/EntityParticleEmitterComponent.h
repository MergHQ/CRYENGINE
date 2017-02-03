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
public:

	struct SAdvancedProperties
	{
		static void ReflectType(CTypeDesc<SAdvancedProperties>& desc);

		float scale = 1.0f;
		float countScale = 1.0f;
		float speedScale = 1.0f;
		float timeScale = 1.0f;
		float pulsePeriod = 0.0f;
		float strength = -1.0f;
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

	ParticleEffectName  m_effectName;
	bool                m_bInitVisible = true;
	bool                m_bPrime = false;
	SAdvancedProperties m_advancedProperties;

	int                m_slot = EmptySlot;
	bool               m_bVisible = false;
};

} // Schematyc
