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

	CEntityLightComponent();

	// CComponent
	virtual bool Init() override;
	virtual void Run(ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	virtual int  GetSlot() const override;
	// ~CComponent

	void         SetTransform(const CTransform& transform);
	void         Switch(bool bOn);

	static SGUID ReflectSchematycType(CTypeInfo<CEntityLightComponent>& typeInfo);
	static void  Register(IEnvRegistrar& registrar);

private:

	void SwitchOn();
	void SwitchOff();

	void FreeSlot();

	void RenderGizmo(float gizmoLength) const;

private:

	int  m_slot;
	bool m_bOn;
};
} // Schematyc
