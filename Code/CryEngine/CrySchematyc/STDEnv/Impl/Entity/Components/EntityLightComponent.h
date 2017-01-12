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
private:

	struct SProperties
	{
		SProperties();

		void Serialize(Serialization::IArchive& archive);

		bool   bOn;
		ColorF color;
		float  diffuseMultiplier;
		float  specularMultiplier;
		float  hdrDynamicMultiplier;
		float  radius;
		float  frustumAngle;
		float  attenuationBulbSize;
	};

public:

	CEntityLightComponent();

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

	int  m_slot;
	bool m_bOn;
};

} // Schematyc
