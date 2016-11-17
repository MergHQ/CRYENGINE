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

	struct SParticleEmitterProperties
	{
		SParticleEmitterProperties();

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

	struct SProperties
	{
		void Serialize(Serialization::IArchive& archive);

		CTransform                 transform;
		SParticleEmitterProperties particleEmitter;
	};

	struct SPreviewProperties
	{
		SPreviewProperties();

		void Serialize(Serialization::IArchive& archive);

		bool  bShowGizmos;
		float gizmoLength;
	};

	class CPreviewer : public IComponentPreviewer
	{
	public:

		// IComponentPreviewer
		virtual void SerializeProperties(Serialization::IArchive& archive) override;
		virtual void Render(const IObject& object, const CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const override;
		// ~IComponentPreviewer

	private:

		SPreviewProperties m_properties;
	};

public:

	CEntityParticleEmitterComponent();

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void         SetTransform(const CTransform& transform);
	void         SetVisible(bool bVisible);
	bool         IsVisible() const;

	static SGUID ReflectSchematycType(CTypeInfo<CEntityParticleEmitterComponent>& typeInfo);
	static void  Register(IEnvRegistrar& registrar);

private:

	void LoadParticleEmitter();

	void RenderGizmo(float gizmoLength) const;

private:

	int m_slot;
};
} // Schematyc
