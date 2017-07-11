#include "StdAfx.h"
#include "RigidBodyComponent.h"

#include "CharacterControllerComponent.h"
#include "Vehicles/VehicleComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CRigidBodyComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::Enable, "{4F1AB9D4-8FB9-4488-9D9A-26B3DDE50C35}"_cry_guid, "Enable");
		pFunction->SetDescription("Enables the physical collider");
		pFunction->BindInput(1, 'enab', "enable");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::IsEnabled, "{CAC21F92-74BE-4F29-9337-96C80B88F1AC}"_cry_guid, "IsEnabled");
		pFunction->SetDescription("Checks if the physical collider is enabled");
		pFunction->BindOutput(0, 'enab', "enabled");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::SetVelocity, "{FC7012F5-2FD2-4E06-BFA2-30A3C44C2AAB}"_cry_guid, "SetVelocity");
		pFunction->SetDescription("Set Entity Velocity");
		pFunction->BindInput(1, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetVelocity, "{1E8C3D8F-404E-452D-B6BE-C5B8EF996FFC}"_cry_guid, "GetVelocity");
		pFunction->SetDescription("Get Entity Velocity");
		pFunction->BindOutput(0, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::SetAngularVelocity, "{33590792-AD99-4C20-B4D9-DA7C4E40B255}"_cry_guid, "SetAngularVelocity");
		pFunction->SetDescription("Set Entity Angular Velocity");
		pFunction->BindInput(1, 'vel', "angular velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetAngularVelocity, "{AEEF2373-B1E3-4334-9D39-CB3376C3C6B3}"_cry_guid, "GetAngularVelocity");
		pFunction->SetDescription("Get Entity Angular Velocity");
		pFunction->BindOutput(0, 'vel', "angular velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyImpulse, "FB7197A9-9545-46BD-8748-1B5829CA3AFA"_cry_guid, "ApplyImpulse");
		pFunction->SetDescription("Applies an impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyAngularImpulse, "22926748-513F-4566-AE40-455A51600A3D"_cry_guid, "ApplyAngularImpulse");
		pFunction->SetDescription("Applies an angular impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		componentScope.Register(pFunction);
	}
	// Signals
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(CRigidBodyComponent::SCollisionSignal);
		pSignal->SetDescription("Sent when the entity collided with another object");
		componentScope.Register(pSignal);
	}
}

void CRigidBodyComponent::ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent>& desc)
{
	desc.SetGUID(CRigidBodyComponent::IID());
	desc.SetEditorCategory("Physics");
	desc.SetLabel("Rigidbody");
	desc.SetDescription("Creates a physical representation of the entity, ");
	desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

	// Entities can only have one physical entity type, thus these are incompatible
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CCharacterControllerComponent>());
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CVehiclePhysicsComponent>());

	desc.AddMember(&CRigidBodyComponent::m_bNetworked, 'netw', "Networked", "Network Synced", "Syncs the physical entity over the network, and keeps it in sync with the server", false);

	desc.AddMember(&CRigidBodyComponent::m_bEnabledByDefault, 'enab', "EnabledByDefault", "Enabled by Default", "Whether the component is enabled by default", true);
	desc.AddMember(&CRigidBodyComponent::m_type, 'type', "Type", "Type", "Type of physicalized object to create", CRigidBodyComponent::EPhysicalType::Rigid);
	desc.AddMember(&CRigidBodyComponent::m_bSendCollisionSignal, 'send', "SendCollisionSignal", "Send Collision Signal", "Whether or not this component should listen for collisions and report them", false);
}

static void ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent::EPhysicalType>& desc)
{
	desc.SetGUID("{C8B86CC7-E86D-46BA-9110-7FEA8052FABC}"_cry_guid);
	desc.SetLabel("Simple Physicalization Type");
	desc.SetDescription("Determines how to physicalize a simple (CGF) object");
	desc.SetDefaultValue(CRigidBodyComponent::EPhysicalType::Rigid);
	desc.AddConstant(CRigidBodyComponent::EPhysicalType::Static, "Static", "Static");
	desc.AddConstant(CRigidBodyComponent::EPhysicalType::Rigid, "Rigid", "Rigid");
}

static void ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent::SCollisionSignal>& desc)
{
	desc.SetGUID("{3E2E1015-0B63-44EC-9993-21E568295CB4}"_cry_guid);
	desc.SetLabel("On Collision");
	desc.AddMember(&CRigidBodyComponent::SCollisionSignal::otherEntity, 'ent', "OtherEntityId", "OtherEntityId", "Other Colliding Entity Id", Schematyc::ExplicitEntityId());
	desc.AddMember(&CRigidBodyComponent::SCollisionSignal::surfaceType, 'srf', "SurfaceType", "SurfaceType", "Material Surface Type at the collision point", "");
}

CRigidBodyComponent::~CRigidBodyComponent()
{
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_NONE;
	m_pEntity->Physicalize(physParams);
}

void CRigidBodyComponent::Initialize()
{
	Physicalize();

	if (m_bNetworked)
	{
		m_pEntity->GetNetEntity()->BindToNetwork();
	}
}

void CRigidBodyComponent::Physicalize()
{
	SEntityPhysicalizeParams physParams;
	physParams.type = (int)m_type;

	// Don't physicalize a slot by default
	physParams.nSlot = std::numeric_limits<int>::max();
	m_pEntity->Physicalize(physParams);

	Enable(m_bEnabledByDefault);
}

void CRigidBodyComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COLLISION)
	{
		// Collision info can be retrieved using the event pointer
		EventPhysCollision* physCollision = reinterpret_cast<EventPhysCollision*>(event.nParam[0]);

		const char* surfaceTypeName = "";
		EntityId otherEntityId = INVALID_ENTITYID;

		ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
		if (ISurfaceType* pSurfaceType = pSurfaceTypeManager->GetSurfaceType(physCollision->idmat[1]))
		{
			surfaceTypeName = pSurfaceType->GetName();
		}

		if (IEntity* pOtherEntity = gEnv->pEntitySystem->GetEntityFromPhysics(physCollision->pEntity[1]))
		{
			otherEntityId = pOtherEntity->GetId();
		}

		// Send OnCollision signal
		if (Schematyc::IObject* pSchematycObject = m_pEntity->GetSchematycObject())
		{
			pSchematycObject->ProcessSignal(SCollisionSignal(otherEntityId, Schematyc::SurfaceTypeName(surfaceTypeName)), GetGUID());
		}
	}
	else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		m_pEntity->UpdateComponentEventMask(this);

		Physicalize();
	}
}

uint64 CRigidBodyComponent::GetEventMask() const
{
	uint64 bitFlags = m_bSendCollisionSignal ? BIT64(ENTITY_EVENT_COLLISION) : 0;;
	bitFlags |= BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

	return bitFlags;
}
}
}
