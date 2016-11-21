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

class CSchematycEntitySensorComponent final : public Schematyc::CComponent
{
public:

	enum class EVolumeShape
	{
		Box,
		Sphere
	};

private:

	struct SProperties
	{
		SProperties();

		void Serialize(Serialization::IArchive& archive);

		SensorTagNames tags;
		SensorTagNames monitorTags;
		EVolumeShape   shape;
		Vec3           size;
		float          radius;
	};

	struct SPreviewProperties
	{
		void Serialize(Serialization::IArchive& archive);

		bool  bShowVolumes = false;
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

	typedef std::vector<EntityId> EntityIds;

	struct SEnterSignal
	{
		SEnterSignal();
		SEnterSignal(EntityId _entityId);

		static Schematyc::SGUID ReflectSchematycType(Schematyc::CTypeInfo<SEnterSignal>& typeInfo);

		Schematyc::ExplicitEntityId entityId;
	};

	struct SLeaveSignal
	{
		SLeaveSignal();
		SLeaveSignal(EntityId _entityId);

		static Schematyc::SGUID ReflectSchematycType(Schematyc::CTypeInfo<SLeaveSignal>& typeInfo);

		Schematyc::ExplicitEntityId entityId;
	};

public:

	// Schematyc::CComponent
	virtual bool Init() override;
	virtual void Run(Schematyc::ESimulationMode simulationMode) override;
	virtual void Shutdown() override;
	// ~Schematyc::CComponent

	void                    Enable();
	void                    Disable();

	void                    SetVolumeSize(const Vec3& size);
	Vec3                    GetVolumeSize() const;

	void                    SetVolumeRadius(float radius);
	float                   GetVolumeRadius() const;

	static Schematyc::SGUID ReflectSchematycType(Schematyc::CTypeInfo<CSchematycEntitySensorComponent>& typeInfo);
	static void             Register(Schematyc::IEnvRegistrar& registrar);

private:

	void          OnEntityEvent(const SEntityEvent& event);
	void          OnSensorEvent(const SSensorEvent& event);

	CSensorBounds CreateBounds(const Matrix34& worldTM, const Schematyc::CTransform& transform, const SProperties& properties) const;
	CSensorBounds CreateOBBBounds(const Matrix34& worldTM, const Vec3& pos, const Vec3& size, const Matrix33& rot) const;
	CSensorBounds CreateSphereBounds(const Matrix34& worldTM, const Vec3& pos, float radius) const;

	SensorTags    GetTags(const SensorTagNames& tagNames) const;

	void          RenderVolume() const;

private:

	SensorVolumeId                         m_volumeId;

	Schematyc::EntityNotHiddenUpdateFilter m_updateFilter;
	Schematyc::CConnectionScope            m_connectionScope;
};
