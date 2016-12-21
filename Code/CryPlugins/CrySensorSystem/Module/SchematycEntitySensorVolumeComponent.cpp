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

bool Serialize(Serialization::IArchive& archive, SSensorTagName& value, const char* szName, const char* szLabel)
{
	return static_cast<CSensorTagLibrary&>(CSensorSystem::GetInstance().GetTagLibrary()).SerializeTagName(archive, static_cast<string&>(value), szName, szLabel); // #TODO : Find a nicer way to access CSensorTagLibrary!!!
}

CSchematycEntitySensorVolumeComponent::SProperties::SProperties()
	: shape(EVolumeShape::Box)
	, size(1.0f)
	, radius(1.0f)
{}

void CSchematycEntitySensorVolumeComponent::SProperties::Serialize(Serialization::IArchive& archive)
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

	if (archive.openBlock("tags", "Tags"))
	{
		archive(attributeTags, "attributeTags", "+Attribute");
		archive.doc("Tags describing this volume.");

		archive(listenerTags, "listenerTags", "+Listener");
		archive.doc("Listen for collision with volumes that have one or more of these tags set.");

		archive.closeBlock();
	}
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

Schematyc::SGUID CSchematycEntitySensorVolumeComponent::SEnteringSignal::ReflectSchematycType(Schematyc::CTypeInfo<SEnteringSignal>& typeInfo)
{
	typeInfo.AddMember(&SEnteringSignal::entityId, 'ent', "EntityId", "Id of entity entering sensor volume");
	return "19cb7f01-1921-441c-b90e-64f940f38e80"_schematyc_guid;
}

CSchematycEntitySensorVolumeComponent::SLeavingSignal::SLeavingSignal() {}

CSchematycEntitySensorVolumeComponent::SLeavingSignal::SLeavingSignal(EntityId _entityId)
	: entityId(static_cast<Schematyc::ExplicitEntityId>(_entityId))
{}

Schematyc::SGUID CSchematycEntitySensorVolumeComponent::SLeavingSignal::ReflectSchematycType(Schematyc::CTypeInfo<SLeavingSignal>& typeInfo)
{
	typeInfo.AddMember(&SLeavingSignal::entityId, 'ent', "EntityId", "Id of entity leaving sensor volume");
	return "474cc5fe-d7f0-4606-a06c-72cc0b3080cb"_schematyc_guid;
}

bool CSchematycEntitySensorVolumeComponent::Init()
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	SSensorVolumeParams volumeParams;
	volumeParams.entityId = entity.GetId();
	volumeParams.bounds = CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform(), *pProperties);
	volumeParams.attributeTags = GetTags(pProperties->attributeTags);
	volumeParams.listenerTags = GetTags(pProperties->listenerTags);
	volumeParams.eventListener = SCHEMATYC_MEMBER_DELEGATE(CSchematycEntitySensorVolumeComponent::OnSensorEvent, *this);

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
			entityObject.GetEventSignalSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(CSchematycEntitySensorVolumeComponent::OnEntityEvent, *this), m_connectionScope);
			break;
		}
	}

	// #TODO : Implement enable/disable functionality!!!
	//const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());
	//if (pProperties->bEnabled)
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
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	SProperties* pProperties = static_cast<SProperties*>(Schematyc::CComponent::GetProperties());

	pProperties->size = size;

	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform(), *pProperties));
}

Vec3 CSchematycEntitySensorVolumeComponent::GetVolumeSize() const
{
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	return pProperties->size;
}

void CSchematycEntitySensorVolumeComponent::SetVolumeRadius(float radius)
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	SProperties* pProperties = static_cast<SProperties*>(Schematyc::CComponent::GetProperties());

	pProperties->radius = radius;

	CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform(), *pProperties));
}

float CSchematycEntitySensorVolumeComponent::GetVolumeRadius() const
{
	const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

	return pProperties->radius;
}

Schematyc::SGUID CSchematycEntitySensorVolumeComponent::ReflectSchematycType(Schematyc::CTypeInfo<CSchematycEntitySensorVolumeComponent>& typeInfo)
{
	return "5F0322C0-2EB0-46C7-B3E3-56AB5F200E74"_schematyc_guid;
}

void CSchematycEntitySensorVolumeComponent::Register(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(Schematyc::g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CSchematycEntitySensorVolumeComponent, "SensorVolume");
		pComponent->SetAuthor(Schematyc::g_szCrytek);
		pComponent->SetDescription("Entity sensor volume component");
		pComponent->SetIcon("icons:schematyc/entity_sensor_volume_component.ico");
		pComponent->SetFlags({ Schematyc::EEnvComponentFlags::Transform, Schematyc::EEnvComponentFlags::Attach });
		pComponent->SetProperties(SProperties());
		pComponent->SetPreviewer(CPreviewer());
		scope.Register(pComponent);

		Schematyc::CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::SetVolumeSize, "C926DA55-97A2-4C97-A7E6-C1416DD6284A"_schematyc_guid, "SetVolumeSize");
			pFunction->SetAuthor(Schematyc::g_szCrytek);
			pFunction->SetDescription("Set size of box volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'size', "Size", nullptr, Vec3(1.0f));
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::GetVolumeSize, "E8E87E86-C96B-4948-A298-0FE010E04F2E"_schematyc_guid, "GetVolumeSize");
			pFunction->SetAuthor(Schematyc::g_szCrytek);
			pFunction->SetDescription("Get size of box volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'size', "Size");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::SetVolumeRadius, "17FB678F-2256-4F7B-AB46-E8EAACD2497C"_schematyc_guid, "SetVolumeRadius");
			pFunction->SetAuthor(Schematyc::g_szCrytek);
			pFunction->SetDescription("Set radius of sphere volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'rad', "Radius", nullptr, 0.0f);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntitySensorVolumeComponent::GetVolumeRadius, "3950621F-8D47-4BE7-836C-35F211B41FBB"_schematyc_guid, "GetVolumeRadius");
			pFunction->SetAuthor(Schematyc::g_szCrytek);
			pFunction->SetDescription("Get radius of sphere volume.");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'rad', "Radius");
			componentScope.Register(pFunction);
		}
		// Signals
		{
			auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SEnteringSignal, "Entering");
			pSignal->SetAuthor(Schematyc::g_szCrytek);
			pSignal->SetDescription("Sent when an entity enters the sensor volume.");
			componentScope.Register(pSignal);
		}
		{
			auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SLeavingSignal, "Leaving");
			pSignal->SetAuthor(Schematyc::g_szCrytek);
			pSignal->SetDescription("Sent when an entity leaves the sensor volume.");
			componentScope.Register(pSignal);
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
			const SProperties* pProperties = static_cast<const SProperties*>(Schematyc::CComponent::GetProperties());

			CSensorSystem::GetInstance().GetMap().UpdateVolumeBounds(m_volumeId, CreateBounds(entity.GetWorldTM(), Schematyc::CComponent::GetTransform(), *pProperties));
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
			CComponent::GetObject().ProcessSignal(SEnteringSignal(otherVolumeParams.entityId));
			break;
		}
	case ESensorEventType::Leaving:
		{
			CComponent::GetObject().ProcessSignal(SLeavingSignal(otherVolumeParams.entityId));
			break;
		}
	}
}

CSensorBounds CSchematycEntitySensorVolumeComponent::CreateBounds(const Matrix34& worldTM, const Schematyc::CTransform& transform, const SProperties& properties) const
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
