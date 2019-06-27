#include "StdAfx.h"
#include "RigidBodyComponent.h"

#include "CharacterControllerComponent.h"
#include "Vehicles/VehicleComponent.h"

#include <Cry3DEngine/IRenderNode.h>
#include <Cry3DEngine/ISurfaceType.h>

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
		pFunction->BindInput(2, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetVelocity, "{1E8C3D8F-404E-452D-B6BE-C5B8EF996FFC}"_cry_guid, "GetVelocity");
		pFunction->SetDescription("Get Entity Velocity");
		pFunction->BindInput(1, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		pFunction->BindOutput(0, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::SetAngularVelocity, "{33590792-AD99-4C20-B4D9-DA7C4E40B255}"_cry_guid, "SetAngularVelocity");
		pFunction->SetDescription("Set Entity Angular Velocity");
		pFunction->BindInput(1, 'vel', "angular velocity");
		pFunction->BindInput(2, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetAngularVelocity, "{AEEF2373-B1E3-4334-9D39-CB3376C3C6B3}"_cry_guid, "GetAngularVelocity");
		pFunction->SetDescription("Get Entity Angular Velocity");
		pFunction->BindInput(1, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		pFunction->BindOutput(0, 'vel', "angular velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyImpulse, "{FB7197A9-9545-46BD-8748-1B5829CA3AFA}"_cry_guid, "ApplyImpulse");
		pFunction->SetDescription("Applies an impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		pFunction->BindInput(2, 'pt', "Point", "Application point in the world space; (0,0,0) to ignore the point and apply the impulse to the center of mass", Vec3(ZERO));
		pFunction->BindInput(3, 'part', "Part Id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyAngularImpulse, "{22926748-513F-4566-AE40-455A51600A3D}"_cry_guid, "ApplyAngularImpulse");
		pFunction->SetDescription("Applies an angular impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		pFunction->BindInput(2, 'part', "Part Id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}
	// Signals
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(CRigidBodyComponent::SCollisionSignal);
		pSignal->SetDescription("Sent when the entity collided with another object");
		componentScope.Register(pSignal);
	}
}

static void ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent::SCollisionSignal>& desc)
{
	desc.SetGUID("{3E2E1015-0B63-44EC-9993-21E568295CB4}"_cry_guid);
	desc.SetLabel("On Collision");
	desc.AddMember(&CRigidBodyComponent::SCollisionSignal::otherEntity, 'ent', "OtherEntity", "OtherEntity", "Other Colliding Entity", Schematyc::ExplicitEntityId());
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

	if (m_isNetworked)
	{
		m_pEntity->GetNetEntity()->EnableDelegatableAspect(eEA_Physics, false);
		m_pEntity->GetNetEntity()->BindToNetwork();
		m_pEntity->GetNetEntity()->SetAspectProfile(eEA_Physics, static_cast<int>(m_type));
	}
}

void CRigidBodyComponent::Physicalize()
{
	SEntityPhysicalizeParams physParams;
	physParams.type = (int)m_type;
	
	Enable(m_isEnabledByDefault);

	// Don't physicalize a slot by default
	physParams.nSlot = std::numeric_limits<int>::max();
	m_pEntity->Physicalize(physParams);

	pe_simulation_params simParams;
	simParams.maxTimeStep = m_simulationParameters.maxTimeStep;
	simParams.minEnergy = sqr(m_simulationParameters.sleepSpeed);
	simParams.damping = m_simulationParameters.damping;
	m_pEntity->GetPhysicalEntity()->SetParams(&simParams);

	pe_params_buoyancy buoyancyParams;
	buoyancyParams.waterDensity = m_buoyancyParameters.density;
	buoyancyParams.waterResistance = m_buoyancyParameters.resistance;
	buoyancyParams.waterDamping = m_buoyancyParameters.damping;
	m_pEntity->GetPhysicalEntity()->SetParams(&buoyancyParams);

	pe_action_awake aa;
	aa.bAwake = m_isResting ? 0 : 1;
	m_pEntity->GetPhysicalEntity()->Action(&aa);
}

void CRigidBodyComponent::ProcessEvent(const SEntityEvent& event)
{

	switch (event.event)
	{
	case ENTITY_EVENT_COLLISION:
		{
			// Collision info can be retrieved using the event pointer
			EventPhysCollision* physCollision = reinterpret_cast<EventPhysCollision*>(event.nParam[0]);

			const char* surfaceTypeName = "";
			EntityId otherEntityId = INVALID_ENTITYID;

			ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();

			IEntity* pOtherEntity = gEnv->pEntitySystem->GetEntityFromPhysics(physCollision->pEntity[0]);
			ISurfaceType* pSurfaceType = pSurfaceTypeManager->GetSurfaceType(physCollision->idmat[0]);

			if (pOtherEntity == m_pEntity || pOtherEntity == nullptr)
			{
				pSurfaceType = pSurfaceTypeManager->GetSurfaceType(physCollision->idmat[1]);
				pOtherEntity = gEnv->pEntitySystem->GetEntityFromPhysics(physCollision->pEntity[1]);
			}

			if (pSurfaceType != nullptr)
			{
				surfaceTypeName = pSurfaceType->GetName();
			}

			if (pOtherEntity == nullptr)
			{
				return;
			}

			otherEntityId = pOtherEntity->GetId();

			// Send OnCollision signal
			if (Schematyc::IObject* pSchematycObject = m_pEntity->GetSchematycObject())
			{
				pSchematycObject->ProcessSignal(SCollisionSignal(otherEntityId, Schematyc::SurfaceTypeName(surfaceTypeName)), GetGUID());
			}
		}
		break;
	case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
		{
			m_pEntity->UpdateComponentEventMask(this);

			Physicalize();
		}
		break;
	case ENTITY_EVENT_START_GAME:
		{
			if (m_isEnabledByDefault)
			{
				pe_action_awake pa;
				pa.bAwake = !m_isResting;
				m_pEntity->GetPhysicalEntity()->Action(&pa);
			}
		}
		break;
	default: break;
	}

}

Cry::Entity::EventFlags CRigidBodyComponent::GetEventMask() const
{
	Cry::Entity::EventFlags bitFlags = ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED | ENTITY_EVENT_START_GAME;
	if (m_bSendCollisionSignal)
	{
		bitFlags |= ENTITY_EVENT_COLLISION;
	}

	return bitFlags;
}

bool CRigidBodyComponent::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (aspect == eEA_Physics)
	{
		// Serialize physics state in order to keep the clients in sync
		m_pEntity->PhysicsNetSerializeTyped(ser, static_cast<int>(m_type), flags);
	}

	return true;
}

}
}
