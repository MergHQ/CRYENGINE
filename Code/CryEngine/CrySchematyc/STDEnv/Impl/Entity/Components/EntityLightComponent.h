// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityLightComponent final : public CComponent
{
public:

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void        SetTransform(const CTransform& transform);
	void        Switch(bool bOn);

	static void ReflectType(CTypeDesc<CEntityLightComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

private:

	void SwitchOn();
	void SwitchOff();

	void FreeSlot();

private:

	bool   m_bInitOn = true;
	ColorF m_color = Col_White;
	float  m_diffuseMultiplier = 1.0f;
	float  m_specularMultiplier = 1.0f;
	float  m_hdrDynamicMultiplier = 1.0f;
	float  m_radius = 10.0f;
	float  m_frustumAngle = 45.0f;
	float  m_attenuationBulbSize = 0.05f;

	int    m_slot = EmptySlot;
	bool   m_bOn = false;
};

} // Schematyc
