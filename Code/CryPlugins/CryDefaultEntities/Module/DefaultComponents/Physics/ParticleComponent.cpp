#include "StdAfx.h"
#include "ParticleComponent.h"

#include <Cry3DEngine/IRenderNode.h>
#include <Cry3DEngine/ISurfaceType.h>

namespace Cry
{
namespace DefaultComponents
{

void CPhysParticleComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPhysParticleComponent::Enable, "{4D4205D7-8644-4CB1-856A-B43A31704A09}"_cry_guid, "Enable");
		pFunction->SetDescription("Enables the physical collider");
		pFunction->BindInput(1, 'enab', "enable");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPhysParticleComponent::IsEnabled, "{323ED07C-3B5C-4D5D-8E20-CC597415F294}"_cry_guid, "IsEnabled");
		pFunction->SetDescription("Checks if the physical collider is enabled");
		pFunction->BindOutput(0, 'enab', "enabled");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPhysParticleComponent::SetVelocity, "{157A144C-2C17-4502-8166-1A3DA3B84DC7}"_cry_guid, "SetVelocity");
		pFunction->SetDescription("Set Entity Velocity");
		pFunction->BindInput(1, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPhysParticleComponent::GetVelocity, "{CC8D2D20-8A51-45C2-9A6A-722D5098E167}"_cry_guid, "GetVelocity");
		pFunction->SetDescription("Get Entity Velocity");
		pFunction->BindOutput(0, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPhysParticleComponent::SetAngularVelocity, "{7C748AB3-7584-46C8-8A0A-57BA3BA1D0A0}"_cry_guid, "SetAngularVelocity");
		pFunction->SetDescription("Set Entity Angular Velocity");
		pFunction->BindInput(1, 'vel', "angular velocity");
		componentScope.Register(pFunction);
	}

	// Signals
	{
		auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(CPhysParticleComponent::SCollisionSignal);
		pSignal->SetDescription("Sent when the entity collided with another object");
		componentScope.Register(pSignal);
	}
}

static void ReflectType(Schematyc::CTypeDesc<CPhysParticleComponent::SCollisionSignal>& desc)
{
	desc.SetGUID("{ED695D65-EFC3-46A9-AE8C-604D74288801}"_cry_guid);
	desc.SetLabel("On Collision");
	desc.AddMember(&CPhysParticleComponent::SCollisionSignal::otherEntity, 'ent', "OtherEntity", "OtherEntity", "Other Colliding Entity", Schematyc::ExplicitEntityId());
	desc.AddMember(&CPhysParticleComponent::SCollisionSignal::surfaceType, 'srf', "SurfaceType", "SurfaceType", "Material Surface Type at the collision point", "");
}

CPhysParticleComponent::~CPhysParticleComponent()
{
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_NONE;
	m_pEntity->Physicalize(physParams);
}

void CPhysParticleComponent::Initialize()
{
	Physicalize();

	if (m_isNetworked)
	{
		m_pEntity->GetNetEntity()->EnableDelegatableAspect(eEA_Physics, false);
		m_pEntity->GetNetEntity()->BindToNetwork();
		m_pEntity->GetNetEntity()->SetAspectProfile(eEA_Physics, 1);
	}
}

void CPhysParticleComponent::Physicalize()
{
	pe_params_particle pp;
	pp.mass = m_mass;
	pp.size = m_size;
	pp.thickness = m_thickness;
	pp.normal = m_normal.value;
	pp.rollAxis = m_rollAxis.value;
	pp.accThrust = m_thrust;
	pp.accLift = m_lift;
	PhysicsVars &vars = *gEnv->pPhysicalWorld->GetPhysVars();
	pp.gravity = vars.gravity * m_gravity;
	pp.waterGravity = vars.gravity * m_gravityWater;
	pp.kAirResistance = m_airResistance;
	pp.kWaterResistance = m_waterResistance;
	pp.minBounceVel = m_minBounceVel;
	pp.minVel = m_minVel;

	IMaterialManager *pMatman = gEnv->p3DEngine->GetMaterialManager();
	if (strcmp(m_surfType.value, pMatman->GetSurfaceType(m_idxSurfType)->GetName() + 4))
	{
		ISurfaceType *pST = pMatman->GetSurfaceTypeByName(string("mat_") + m_surfType.value);
		if (strcmp(m_surfType.value, pST->GetName() + 4))
			m_surfType = pST->GetName() + 4;
		m_idxSurfType = pST->GetId();
	}
	pp.surface_idx = m_idxSurfType;

	pp.flags = (m_traceable ? pef_traceable : 0) | (m_singleContact ? particle_single_contact : 0) | (m_constantOrientation ? particle_constant_orientation : 0) |
		(m_noRoll ? particle_no_roll : 0) | (m_noSpin ? particle_no_spin : 0) | (m_noPathAlignment ? particle_no_path_alignment : 0) | 
		(m_noSelfCollisions ? particle_no_self_collisions : 0) | (m_noImpulse ? particle_no_impulse : 0) | (m_bSendCollisionSignal ? pef_log_collisions : 0);

	SEntityPhysicalizeParams physParams;
	physParams.type = PE_PARTICLE;
	// Don't physicalize a slot by default
	physParams.nSlot = std::numeric_limits<int>::max();
	physParams.pParticle = &pp;
	m_pEntity->Physicalize(physParams);

	pe_action_awake aa;
	aa.bAwake = m_awake ? 1 : 0;
	m_pEntity->GetPhysics()->Action(&aa);
}

void CPhysParticleComponent::ProcessEvent(const SEntityEvent& event)
{

	switch (event.event)
	{
		case ENTITY_EVENT_COLLISION:
		{
			EventPhysCollision* physCollision = reinterpret_cast<EventPhysCollision*>(event.nParam[0]);
			const char* surfaceTypeName = "";
			EntityId otherEntityId = INVALID_ENTITYID;
			IEntity* pOtherEntity = gEnv->pEntitySystem->GetEntityFromPhysics(physCollision->pEntity[1]);
			if (!pOtherEntity)
				return;
			if (ISurfaceType* pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceType(physCollision->idmat[1]))
				surfaceTypeName = pSurfaceType->GetName();
			otherEntityId = pOtherEntity->GetId();
			if (Schematyc::IObject* pSchematycObject = m_pEntity->GetSchematycObject())	// Send OnCollision signal
				pSchematycObject->ProcessSignal(SCollisionSignal(otherEntityId, Schematyc::SurfaceTypeName(surfaceTypeName)), GetGUID());
		}
		break;

		case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
			m_pEntity->UpdateComponentEventMask(this);
			Physicalize();
			break;

		case ENTITY_EVENT_START_GAME:
		{
			pe_action_awake aa;
			aa.bAwake = m_awake ? 1 : 0;
			m_pEntity->GetPhysics()->Action(&aa);
		}
		break;
	}

}

Cry::Entity::EventFlags CPhysParticleComponent::GetEventMask() const
{
	Cry::Entity::EventFlags flags =  ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED | ENTITY_EVENT_START_GAME;
	if (m_bSendCollisionSignal) flags |= ENTITY_EVENT_COLLISION;
	return flags;
}

bool CPhysParticleComponent::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (aspect == eEA_Physics)
	{
		// Serialize physics state in order to keep the clients in sync
		m_pEntity->PhysicsNetSerializeTyped(ser, PE_PARTICLE, flags);
	}

	return true;
}

}
}
