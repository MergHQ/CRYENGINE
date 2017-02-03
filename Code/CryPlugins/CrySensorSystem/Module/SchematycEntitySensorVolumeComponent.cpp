// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SchematycEntitySensorVolumeComponent.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySerialization/DynArray.h>

#include "SensorMap.h"
#include "SensorSystem.h"
#include "SensorTagLibrary.h"

SERIALIZATION_ENUM_BEGIN_NESTED(CSchematycEntitySensorVolumeComponent, EVolumeShape, "Sensor volume shape")
SERIALIZATION_ENUM(CSchematycEntitySensorVolumeComponent::EVolumeShape::Box, "Box", "Box")
SERIALIZATION_ENUM(CSchematycEntitySensorVolumeComponent::EVolumeShape::Sphere, "Sphere", "Sphere")
SERIALIZATION_ENUM_END()

inline bool Serialize(Serialization::IArchive& archive, SSensorTagName& value, const char* szName, const char* szLabel)
{
	return static_cast<CSensorTagLibrary&>(CSensorSystem::GetInstance().GetTagLibrary()).SerializeTagName(archive, static_cast<string&>(value), szName, szLabel); // #TODO : Find a nicer way to access CSensorTagLibrary!!!
}

void CSchematycEntitySensorVolumeComponent::SDimensions::ReflectType(Schematyc::CTypeDesc<CSchematycEntitySensorVolumeComponent::SDimensions>& desc)
{
	desc.SetGUID("ed2add7d-d063-4371-9a91-b6c13e0c2751"_schematyc_guid);
}

// N.B. Non-intrusive serialization is used here only to ensure backward compatibility.
inline bool Serialize(Serialization::IArchive& archive, CSchematycEntitySensorVolumeComponent::SDimensions& value, const char* szName, const char* szLabel)
{
	archive(value.shape, "shape", "Shape");
	archive.doc("Shape");
	switch (value.shape)
	{
	case CSchematycEntitySensorVolumeComponent::EVolumeShape::Box:
		{
			archive(value.size, "size", "Size");
			archive.doc("Size");
			break;
		}
	case CSchematycEntitySensorVolumeComponent::EVolumeShape::Sphere:
		{
			archive(value.radius, "radius", "Radius");
			archive.doc("Radius");
			break;
		}
	}
	return true;
}

void CSchematycEntitySensorVolumeComponent::STags::ReflectType(Schematyc::CTypeDesc<CSchematycEntitySensorVolumeComponent::STags>& desc)
{
	desc.SetGUID("6f03eddc-71ff-467e-9d8d-ad7e57438aad"_schematyc_guid);
}

// N.B. Non-intrusive serialization is used here only to ensure backward compatibility.
//      If files were patched we could instead reflect members and let the system serialize them automatically.
inline bool Serialize(Serialization::IArchive& archive, CSchematycEntitySensorVolumeComponent::STags& value, const char* szName, const char* szLabel)
{
	if (archive.openBlock("tags", "Tags"))
	{
		archive(value.attributeTags, "attributeTags", "+Attribute");
		archive.doc("Tags describing this volume.");

		archive(value.listenerTags, "listenerTags", "+Listener");
		archive.doc("Listen for collision with volumes that have one or more of these tags set.");

		archive.closeBlock();
	}
	return true;
}

void CSchematycEntitySensorVolumeComponent::SPreviewProperties::Serialize(Serialization::IArchive& archive)
{
	archive(bShowVolumes, "bShowVolumes", "Show Volumes");
}

void CSchematycEntitySensorVolumeComponent::CPreviewer::SerializeProperties(Serialization::IArchive& archive)
{
	archive(m_properties, "properties", "Sensor Component");
}

void CSchematycEntitySensorVolumeComponent::CPreviewer::Render(const Schematyc::IObject& object, const Schematyc::CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const
{
	const CSchematycEntitySensorVolumeComponent& sensorComponent = static_cast<const CSchematycEntitySensorVolumeComponent&>(component);
	if (m_properties.bShowVolumes)
	{
		sensorComponent.RenderVolume();
	}
}

CSchematycEntitySensorVolumeComponent::SEnteringSignal::SEnteringSignal() {}

CSchematycEntitySensorVolumeComponent::SEnteringSignal::SEnteringSignal(EntityId _entityId)
	: entityId(static_cast<Schematyc::ExplicitEntityId>(_entityId))
{}

void CSchematycEntitySensorVolumeComponent::SEnteringSignal::ReflectType(Schematyc::CTypeDesc<SEnteringSignal>& desc)
{
	desc.SetGUID("19cb7f01-1921-441c-b90e-64f940f38e80"_schematyc_guid);
	desc.SetLabel("Entering");
	desc.SetDescription("Sent when an entity enters the sensor volume.");
	desc.AddMember(&SEnteringSignal::entityId, 'ent', "entityId", "EntityId", "Id of entity entering sensor volume");
}

CSchematycEntitySensorVolumeComponent::SLeavingSignal::SLeavingSignal() {}

CSchematycEntitySensorVolumeComponent::SLeavingSignal::SLeavingSignal(EntityId _entityId)
	: entityId(static_cast<Schematyc::ExplicitEntityId>(_entityId))
{}

void CSchematycEntitySensorVolumeComponent::SLeavingSignal::ReflectType(Schematyc::CTypeDesc<SLeavingSignal>& desc)
{
	desc.SetGUID("474cc5fe-d7f0-4606-a06c-72cc0b3080cb"_schematyc_guid);
	desc.SetLabel("Leaving");
	desc.SetDescription("Sent when an entity leaves the sensor volume.");
	desc.AddMember(&SLeavingSignal::entityId, 'ent', "entityId", "EntityId", "Id of entity leaving sensor volume");
}

bool CSchematycEntitySensorVolumeComponent::Init()
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);

	SSensorVolumeParams volumeParams;
	volumeParams.entityId = entity.GetId();
	volumeParams.bounds = CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform());
	volumeParams.attributeTags = GetTags(m_tags.attributeTags);
	volumeParams.listenerTags = GetTags(m_tags.listenerTags);
	volumeParams.eventListener = SCHEMATYC_MEMBER_DELEGATE(&CSchematycEntitySensorVolumeComponent::OnSensorEvent, *this);

	m_volumeId = CSensorSystem::GetInstance().GetMap().CreateVolume(volumeParams);

	return true;
}

void CSchematycEntitySensorVolumeComponent::Run(Schematyc::ESimulationMode simulationMode)
{
	switch (simulationMode)
	{
	case Schematyc::ESimulationMode::Game:
		{
			Schematyc::IEntityObject& entityObject = Schematyc::EntityUtils::GetEntityObject(*this);
			entityObject.GetEventSignalSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(&CSchematycEntitySensorVolumeComponent::OnEntityEvent, *this), m_connectionScope);
			break;
		}
	}

	// #TODO : Implement enable/disable functionality!!!
	//if (m_properties.bEnabled)
	//{
	//	Enable();
	//}
}

void CSchematycEntitySensorVolumeComponent::Shutdown()
{
	CSensorSystem::GetInstance().GetMap().DestroyVolume(m_volumeId);
	m_volumeId = SensorVolumeId();
}

void CSchematycEntitySensorVolumeComponent::Enable()
{
	// #TODO : Implement enable/disable functionality!!!
	//CSensorSystem::GetInstance().GetDefaultMap().EnableVolume(m_volumeId, true);
}

void CSchematycEntitySensorVolumeComponent::Disable()
{
	// #TODO : Implement enable/disable functionality!!!
	//CSensorSystem::GetInstance().GetDefaultMap().EnableVolume(m_volumeId, false);
}

void CSchematycEntitySensorVolumeComponent::SetVolumeSize(const Vec3& size)
{
	m_dimensions.size = size;

	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform()));
}

Vec3 CSchematycEntitySensorVolumeComponent::GetVolumeSize() const
{
	return m_dimensions.size;
}

void CSchematycEntitySensorVolumeComponent::SetVolumeRadius(float radius)
{
	m_dimensions.radius = radius;

	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform()));
}

float CSchematycEntitySensorVolumeComponent::GetVolumeRadius() const
{
	return m_dimensions.radius;
}

void CSchematycEntitySensorVolumeComponent::ReflectType(Schematyc::CTypeDesc<CSchematycEntitySensorVolumeComponent>& desc)
{
	desc.SetGUID("5F0322C0-2EB0-46C7-B3E3-56AB5F200E74"_schematyc_guid);
	desc.SetLabel("SensorVolume");
	desc.SetDescription("Entity sensor volume component");
	desc.SetIcon("icons:schematyc/entity_sensor_volume_component.ico");
	desc.SetComponentFlags({ Schematyc::EComponentFlags::Transform, Schematyc::EComponentFlags::Attach });
	desc.SetPreviewer(CPreviewer());
	desc.AddMember(&CSchematycEntitySensorVolumeComponent::m_dimensions, 'dims', "dimensions", "Dimensions", nullptr);
	desc.AddMember(&CSchematycEntitySensorVolumeComponent::m_tags, 'tags', "tags", "Tags", nullptr);
}

void CSchematycEntitySensorVolumeComponent::Register(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(Schematyc::g_entityClassGUID);
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSchematycEntitySensorVolumeComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::SetVolumeSize, "C926DA55-97A2-4C97-A7E6-C1416DD6284A"_schematyc_guid, "SetVolumeSize");
			pFunction->SetDescription("Set size of box volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'size', "Size", nullptr, Vec3(1.0f));
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::GetVolumeSize, "E8E87E86-C96B-4948-A298-0FE010E04F2E"_schematyc_guid, "GetVolumeSize");
			pFunction->SetDescription("Get size of box volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'size', "Size");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::SetVolumeRadius, "17FB678F-2256-4F7B-AB46-E8EAACD2497C"_schematyc_guid, "SetVolumeRadius");
			pFunction->SetDescription("Set radius of sphere volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'rad', "Radius", nullptr, 0.0f);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::GetVolumeRadius, "3950621F-8D47-4BE7-836C-35F211B41FBB"_schematyc_guid, "GetVolumeRadius");
			pFunction->SetDescription("Get radius of sphere volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'rad', "Radius");
			componentScope.Register(pFunction);
		}
		// Signals
		{
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SEnteringSignal));
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SLeavingSignal));
		}
	}
}

void CSchematycEntitySensorVolumeComponent::OnEntityEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_XFORM:
		{
			IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
			CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform()));
		}
		break;
	}
}

void CSchematycEntitySensorVolumeComponent::OnSensorEvent(const SSensorEvent& event)
{
	const SSensorVolumeParams otherVolumeParams = CSensorSystem::GetInstance().GetMap().GetVolumeParams(event.eventVolumeId);

	switch (event.type)
	{
	case ESensorEventType::Entering:
		{
			CComponent::OutputSignal(SEnteringSignal(otherVolumeParams.entityId));
			break;
		}
	case ESensorEventType::Leaving:
		{
			CComponent::OutputSignal(SLeavingSignal(otherVolumeParams.entityId));
			break;
		}
	}
}

CSensorBounds CSchematycEntitySensorVolumeComponent::CreateBounds(const Matrix34& worldTM, const Schematyc::CTransform& transform) const
{
	switch (m_dimensions.shape)
	{
	case EVolumeShape::Box:
		{
			return CreateOBBBounds(worldTM, transform.GetTranslation(), m_dimensions.size, transform.GetRotation().ToMatrix33());
		}
	case EVolumeShape::Sphere:
		{
			return CreateSphereBounds(worldTM, transform.GetTranslation(), m_dimensions.radius);
		}
	}
	return CSensorBounds();
}

CSensorBounds CSchematycEntitySensorVolumeComponent::CreateOBBBounds(const Matrix34& worldTM, const Vec3& pos, const Vec3& size, const Matrix33& rot) const
{
	const QuatT transform = QuatT(worldTM) * QuatT(Quat(rot), pos);
	return CSensorBounds(OBB::CreateOBB(Matrix33(transform.q), size * 0.5f, transform.q.GetInverted() * transform.t));
}

CSensorBounds CSchematycEntitySensorVolumeComponent::CreateSphereBounds(const Matrix34& worldTM, const Vec3& pos, float radius) const
{
	return CSensorBounds(Sphere(worldTM.TransformPoint(pos), radius));
}

SensorTags CSchematycEntitySensorVolumeComponent::GetTags(const SensorTagNames& tagNames) const
{
	ISensorTagLibrary& sensorTagLibrary = CSensorSystem::GetInstance().GetTagLibrary();
	SensorTags tags;
	for (const SSensorTagName& tagName : tagNames)
	{
		tags.Add(sensorTagLibrary.GetTag(tagName.c_str()));
	}
	return tags;
}

void CSchematycEntitySensorVolumeComponent::RenderVolume() const
{
	CSensorSystem::GetInstance().GetMap().GetVolumeParams(m_volumeId).bounds.DebugDraw(ColorB(0, 150, 0, 128));
}
