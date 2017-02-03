// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

#include "SensorMap.h"
#include "SensorTagLibrary.h"

struct SSensorTagName : public string // #TODO : Move to separate header?
{
	using string::string;
};

bool Serialize(Serialization::IArchive& archive, SSensorTagName& value, const char* szName, const char* szLabel);

typedef DynArray<SSensorTagName> SensorTagNames;

class CSchematycEntitySensorVolumeComponent final : public Schematyc::CComponent
{
public:

	enum class EVolumeShape
	{
		Box,
		Sphere
	};

	struct SDimensions
	{
		static void ReflectType(Schematyc::CTypeDesc<SDimensions>& desc);

		EVolumeShape   shape = EVolumeShape::Box;
		Vec3           size = Vec3(1.0f);
		float          radius = 1.0f;
	};

	struct STags
	{
		static void ReflectType(Schematyc::CTypeDesc<STags>& desc);

		SensorTagNames attributeTags;
		SensorTagNames listenerTags;
		EVolumeShape   shape = EVolumeShape::Box;
	};

private:

	struct SPreviewProperties
	{
		void Serialize(Serialization::IArchive& archive);

		bool bShowVolumes = false;
	};

	class CPreviewer : public Schematyc::IComponentPreviewer
	{
	public:

		// IComponentPreviewer
		virtual void SerializeProperties(Serialization::IArchive& archive) override;
		virtual void Render(const Schematyc::IObject& object, const Schematyc::CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const override;
		// ~IComponentPreviewer

	private:

		SPreviewProperties m_properties;
	};

	struct SEnteringSignal
	{
		SEnteringSignal();
		SEnteringSignal(EntityId _entityId);

		static void ReflectType(Schematyc::CTypeDesc<SEnteringSignal>& desc);

		Schematyc::ExplicitEntityId entityId;
	};

	struct SLeavingSignal
	{
		SLeavingSignal();
		SLeavingSignal(EntityId _entityId);

		static void ReflectType(Schematyc::CTypeDesc<SLeavingSignal>& desc);

		Schematyc::ExplicitEntityId entityId;
	};

public:

	// Schematyc::CComponent
	virtual bool Init() override;
	virtual void Run(Schematyc::ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	// ~Schematyc::CComponent

	void        Enable();
	void        Disable();

	void        SetVolumeSize(const Vec3& size);
	Vec3        GetVolumeSize() const;

	void        SetVolumeRadius(float radius);
	float       GetVolumeRadius() const;

	static void ReflectType(Schematyc::CTypeDesc<CSchematycEntitySensorVolumeComponent>& desc);
	static void Register(Schematyc::IEnvRegistrar& registrar);

private:

	void          OnEntityEvent(const SEntityEvent& event);
	void          OnSensorEvent(const SSensorEvent& event);

	CSensorBounds CreateBounds(const Matrix34& worldTM, const Schematyc::CTransform& transform) const;
	CSensorBounds CreateOBBBounds(const Matrix34& worldTM, const Vec3& pos, const Vec3& size, const Matrix33& rot) const;
	CSensorBounds CreateSphereBounds(const Matrix34& worldTM, const Vec3& pos, float radius) const;

	SensorTags    GetTags(const SensorTagNames& tagNames) const;

	void          RenderVolume() const;

private:

	SDimensions                            m_dimensions;
	STags                                  m_tags;

	SensorVolumeId                         m_volumeId = SensorVolumeId::Invalid;
	Schematyc::EntityNotHiddenUpdateFilter m_updateFilter;
	Schematyc::CConnectionScope            m_connectionScope;
};
