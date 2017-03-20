// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryParticleSystem/IParticlesPfx2.h>
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

		SpawnParams spawnParams;
		GeomRef     geomRef;
	};

public:

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void       SetTransform(const CTransform& transform);
	void       SetVisible(bool bVisible);
	bool       IsVisible() const;
	void       SetParameters(float uniformScale, float countScale, float speedScale, float timeScale);
	void       SetAttributeAsFloat(CSharedString attributeName, float value);
	void       SetAttributeAsInteger(CSharedString attributeName, int value);
	void       SetAttributeAsBoolean(CSharedString attributeName, bool value);
	void       SetAttributeAsColor(CSharedString attributeName, ColorF value);
	void       SetSpawnGeom(const GeomFileName& geomName);

	static void ReflectType(CTypeDesc<CEntityParticleEmitterComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

private:

	void LoadParticleEmitter();

private:

	ParticleEffectName  m_effectName;
	bool                m_bInitVisible = true;
	SAdvancedProperties m_advancedProperties;
	int                 m_slot = EmptySlot;
	bool                m_bVisible = false;
};

} // Schematyc
