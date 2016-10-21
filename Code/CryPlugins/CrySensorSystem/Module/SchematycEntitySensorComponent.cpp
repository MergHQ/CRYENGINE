// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SchematycEntitySensorComponent.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>

#include "SensorMap.h"
#include "SensorSystem.h"
#include "SensorTagLibrary.h"

SERIALIZATION_ENUM_BEGIN_NESTED(CSchematycEntitySensorComponent, EVolumeShape, "Sensor volume shape")
SERIALIZATION_ENUM(CSchematycEntitySensorComponent::EVolumeShape::Box, "Box", "Box")
SERIALIZATION_ENUM(CSchematycEntitySensorComponent::EVolumeShape::Sphere, "Sphere", "Sphere")
SERIALIZATION_ENUM_END()

bool Serialize(Serialization::IArchive& archive, SSensorTagName& value, const char* szName, const char* szLabel)
{
	return static_cast<CSensorTagLibrary&>(CSensorSystem::GetInstance().GetTagLibrary()).SerializeTagName(archive, static_cast<string&>(value), szName, szLabel); // #TODO : Find a nicer way to access CSensorTagLibrary!!!
}

CSchematycEntitySensorComponent::SVolumeProperties::SVolumeProperties()
	: shape(EVolumeShape::Box)
	, size(1.0f)
	, radius(1.0f)
{}

void CSchematycEntitySensorComponent::SVolumeProperties::Serialize(Serialization::IArchive& archive)
{
	archive(shape, "shape", "Shape");
	archive.doc("Shape");

	switch (shape)
	{
	case EVolumeShape::Box:
		{
			archive(size, "size", "Size");
			archive.doc("Size");
			break;
		}
	case EVolumeShape::Sphere:
		{
			archive(radius, "radius", "Radius");
			archive.doc("Radius");
			break;
		}
	}

	archive(tags, "tags", "+Tags");
	archive.doc("Tags");
	archive(monitorTags, "monitorTags", "+Monitor Tags");
	archive.doc("Signals will be sent when volumes with one or more of these tags set enter/leave this volume.");
}

CSchematycEntitySensorComponent::SProperties::SProperties()
	: bEnabled(true)
{}

void CSchematycEntitySensorComponent::SProperties::Serialize(Serialization::IArchive& archive)
{
	archive(transform, "transform", "Transform");
	archive.doc("Transform");

	archive(volume, "volume", "Volume");
	archive.doc("Volume");
}

CSchematycEntitySensorComponent::SPreviewProperties::SPreviewProperties()
	: gizmoLength(1.0f)
	, bShowGizmos(false)
	, bShowVolumes(false)
{}

void CSchematycEntitySensorComponent::SPreviewProperties::Serialize(Serialization::IArchive& archive)
{
	archive(bShowGizmos, "bShowGizmos", "Show Gizmos");
	archive(gizmoLength, "gizmoLength", "Gizmo Length");
	archive(bShowVolumes, "bShowVolumes", "Show Volumes");
}

void CSchematycEntitySensorComponent::CPreviewer::SerializeProperties(Serialization::IArchive& archive)
{
	archive(m_properties, "properties", "Sensor Component");
}

void CSchematycEntitySensorComponent::CPreviewer::Render(const Schematyc::IObject& object, const Schematyc::CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const
{
	const CSchematycEntitySensorComponent& sensorComponent = static_cast<const CSchematycEntitySensorComponent&>(component);
	if (m_properties.bShowGizmos)
	{
		sensorComponent.RenderGizmo(m_properties.gizmoLength);
	}
	if (m_properties.bShowVolumes)
	{
		sensorComponent.RenderVolume();
	}
}

CSchematycEntitySensorComponent::SEnterSignal::SEnterSignal() {}

CSchematycEntitySensorComponent::SEnterSignal::SEnterSignal(EntityId _entityId)
	: entityId(_entityId)
{}

Schematyc::SGUID CSchematycEntitySensorComponent::SEnterSignal::ReflectSchematycType(Schematyc::CTypeInfo<SEnterSignal>& typeInfo)
{
	typeInfo.AddMember(&SEnterSignal::entityId, 'ent', "EntityId", "Id of entity entering sensor volume");
	return "19cb7f01-1921-441c-b90e-64f940f38e80"_schematyc_guid;
}

CSchematycEntitySensorComponent::SLeaveSignal::SLeaveSignal() {}

CSchematycEntitySensorComponent::SLeaveSignal::SLeaveSignal(EntityId _entityId)
	: entityId(_entityId)
{}

Schematyc::SGUID CSchematycEntitySensorComponent::SLeaveSignal::ReflectSchematycType(Schematyc::CTypeInfo<SLeaveSignal>& typeInfo)
{
	typeInfo.AddMember(&SLeaveSignal::entityId, 'ent', "EntityId", "Id of entity leaving sensor volume");
	return "474cc5fe-d7f0-4606-a06c-72cc0b3080cb"_schematyc_guid;
}

bool CSchematycEntitySensorComponent::Init()
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	SSensorVolumeParams volumeParams;
	volumeParams.entityId = entity.GetId();
	volumeParams.bounds = CreateBounds(entity.GetWorldTM(), pProperties->transform, pProperties->volume);
	volumeParams.tags = GetTags(pProperties->volume.tags);
	volumeParams.monitorTags = GetTags(pProperties->volume.monitorTags);
	volumeParams.eventCallback = Schematyc::Delegate::Make(*this, &CSchematycEntitySensorComponent::OnSensorEvent);

	m_volumeId = CSensorSystem::GetInstance().GetMap().CreateVolume(volumeParams);

	return true;
}

void CSchematycEntitySensorComponent::Run(Schematyc::ESimulationMode simulationMode)
{
	switch (simulationMode)
	{
	case Schematyc::ESimulationMode::Game:
		{
			Schematyc::IEntityObject& entityObject = Schematyc::EntityUtils::GetEntityObject(*this);
			entityObject.GetEventSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CSchematycEntitySensorComponent::OnEntityEvent), m_connectionScope);
			break;
		}
	}

	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());
	if (pProperties->bEnabled)
	{
		Enable();
	}
}

void CSchematycEntitySensorComponent::Shutdown()
{
	CSensorSystem::GetInstance().GetMap().DestroyVolume(m_volumeId);
	m_volumeId = SensorVolumeId();
}

void CSchematycEntitySensorComponent::Enable()
{
	// #TODO : Implement!!!

	//CSensorSystem::GetInstance().GetDefaultMap().EnableVolume(m_volumeId, true);
}

void CSchematycEntitySensorComponent::Disable()
{
	// #TODO : Implement!!!

	//CSensorSystem::GetInstance().GetDefaultMap().EnableVolume(m_volumeId, false);
}

void CSchematycEntitySensorComponent::SetVolumeSize(const Vec3& size)
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	SProperties* pProperties = static_cast<SProperties*>(Schematyc::CComponent::GetProperties());

	pProperties->volume.size = size;

	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), pProperties->transform, pProperties->volume));
}

Vec3 CSchematycEntitySensorComponent::GetVolumeSize() const
{
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	return pProperties->volume.size;
}

void CSchematycEntitySensorComponent::SetVolumeRadius(float radius)
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	SProperties* pProperties = static_cast<SProperties*>(Schematyc::CComponent::GetProperties());

	pProperties->volume.radius = radius;

	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), pProperties->transform, pProperties->volume));
}

float CSchematycEntitySensorComponent::GetVolumeRadius() const
{
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	return pProperties->volume.radius;
}

Schematyc::SGUID CSchematycEntitySensorComponent::ReflectSchematycType(Schematyc::CTypeInfo<CSchematycEntitySensorComponent>& typeInfo)
{
	return "5F0322C0-2EB0-46C7-B3E3-56AB5F200E74"_schematyc_guid;
}

void CSchematycEntitySensorComponent::Register(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(Schematyc::g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CSchematycEntitySensorComponent, "Sensor");
		pComponent->SetAuthor("Paul Slinger");
		pComponent->SetDescription("Entity sensor volume component");
		pComponent->SetIcon("editor/icons/cryschematyc/entity_sensor_component.png");
		pComponent->SetFlags(Schematyc::EEnvComponentFlags::Attach);
		pComponent->SetProperties(SProperties());
		pComponent->SetPreviewer(CPreviewer());
		scope.Register(pComponent);

		Schematyc::CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorComponent::SetVolumeSize, "C926DA55-97A2-4C97-A7E6-C1416DD6284A"_schematyc_guid, "SetVolumeSize");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Set size of box volume.");
			pFunction->BindInput(1, 'size', "Size", nullptr, Vec3(1.0f));
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorComponent::GetVolumeSize, "E8E87E86-C96B-4948-A298-0FE010E04F2E"_schematyc_guid, "GetVolumeSize");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Get size of box volume.");
			pFunction->BindOutput(0, 'size', "Size");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorComponent::SetVolumeRadius, "17FB678F-2256-4F7B-AB46-E8EAACD2497C"_schematyc_guid, "SetVolumeRadius");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Set radius of sphere volume.");
			pFunction->BindInput(1, 'rad', "Radius", nullptr, 0.0f);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorComponent::GetVolumeRadius, "3950621F-8D47-4BE7-836C-35F211B41FBB"_schematyc_guid, "GetVolumeRadius");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Get radius of sphere volume.");
			pFunction->BindOutput(0, 'rad', "Radius");
			componentScope.Register(pFunction);
		}
		// Signals
		{
			auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SEnterSignal, "Enter");
			pSignal->SetAuthor("Paul Slinger");
			pSignal->SetDescription("Sent when an entity enters the sensor volume.");
			componentScope.Register(pSignal);
		}
		{
			auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SLeaveSignal, "Leave");
			pSignal->SetAuthor("Paul Slinger");
			pSignal->SetDescription("Sent when an entity leaves the sensor volume.");
			componentScope.Register(pSignal);
		}
	}
}

void CSchematycEntitySensorComponent::OnEntityEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_XFORM:
		{
			IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
			const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

			CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), pProperties->transform, pProperties->volume));
		}
		break;
	}
}

void CSchematycEntitySensorComponent::OnSensorEvent(const SSensorEvent& event)
{
	const SSensorVolumeParams otherVolumeParams = CSensorSystem::GetInstance().GetMap().GetVolumeParams(event.eventVolumeId);

	switch (event.type)
	{
	case ESensorEventType::Enter:
		{
			CComponent::GetObject().ProcessSignal(SEnterSignal(otherVolumeParams.entityId));
			break;
		}
	case ESensorEventType::Leave:
		{
			CComponent::GetObject().ProcessSignal(SLeaveSignal(otherVolumeParams.entityId));
			break;
		}
	}
}

CSensorBounds CSchematycEntitySensorComponent::CreateBounds(const Matrix34& worldTM, const Schematyc::CTransform& transform, const SVolumeProperties& properties) const
{
	switch (properties.shape)
	{
	case EVolumeShape::Box:
		{
			return CreateOBBBounds(worldTM, transform.GetTranslation(), properties.size, transform.GetRotation().ToMatrix33());
		}
	case EVolumeShape::Sphere:
		{
			return CreateSphereBounds(worldTM, transform.GetTranslation(), properties.radius);
		}
	}
	return CSensorBounds();
}

CSensorBounds CSchematycEntitySensorComponent::CreateOBBBounds(const Matrix34& worldTM, const Vec3& pos, const Vec3& size, const Matrix33& rot) const
{
	const QuatT transform = QuatT(worldTM) * QuatT(Quat(rot), pos);
	return CSensorBounds(OBB::CreateOBB(Matrix33(transform.q), size * 0.5f, transform.q.GetInverted() * transform.t));
}

CSensorBounds CSchematycEntitySensorComponent::CreateSphereBounds(const Matrix34& worldTM, const Vec3& pos, float radius) const
{
	return CSensorBounds(Sphere(worldTM.TransformPoint(pos), radius));
}

SensorTags CSchematycEntitySensorComponent::GetTags(const SensorTagNames& tagNames) const
{
	ISensorTagLibrary& sensorTagLibrary = CSensorSystem::GetInstance().GetTagLibrary();
	SensorTags tags;
	for (const SSensorTagName& tagName : tagNames)
	{
		tags.Add(sensorTagLibrary.GetTag(tagName.c_str()));
	}
	return tags;
}

void CSchematycEntitySensorComponent::RenderGizmo(float gizmoLength) const
{
	IRenderAuxGeom& renderAuxGeom = *gEnv->pRenderer->GetIRenderAuxGeom();
	CComponent* pParent = CComponent::GetParent();
	const Matrix34 worldTM = pParent ? Schematyc::EntityUtils::GetEntity(*this).GetSlotWorldTM(pParent->GetSlot()) : Matrix34(IDENTITY);
	const float lineThickness = 4.0f;

	renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(255, 0, 0, 255), worldTM.GetTranslation() + (worldTM.GetColumn0().GetNormalized() * gizmoLength), ColorB(255, 0, 0, 255), lineThickness);
	renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(0, 255, 0, 255), worldTM.GetTranslation() + (worldTM.GetColumn1().GetNormalized() * gizmoLength), ColorB(0, 255, 0, 255), lineThickness);
	renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(0, 0, 255, 255), worldTM.GetTranslation() + (worldTM.GetColumn2().GetNormalized() * gizmoLength), ColorB(0, 0, 255, 255), lineThickness);
}

void CSchematycEntitySensorComponent::RenderVolume() const
{
	CSensorSystem::GetInstance().GetMap().GetVolumeParams(m_volumeId).bounds.DebugDraw(ColorB(0, 150, 0, 128));
}
