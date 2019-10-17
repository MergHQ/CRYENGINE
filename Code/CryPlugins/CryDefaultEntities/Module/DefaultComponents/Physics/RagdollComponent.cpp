#include "StdAfx.h"
#include "../Geometry/AnimatedMeshComponent.h"
#include "../Geometry/AdvancedAnimationComponent.h"
#include "RagdollComponent.h"

namespace Cry
{
namespace DefaultComponents
{

void CRagdollComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::Enable, "{5982B27A-8860-45B7-A65E-FAE0791C377B}"_cry_guid, "Enable");
		pFunction->SetDescription("Enables the physical collider");
		pFunction->BindInput(1, 'enab', "enable");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::IsEnabled, "{C3BB5879-40C2-4618-8E62-F71DE68E6E91}"_cry_guid, "IsEnabled");
		pFunction->SetDescription("Checks if the physical collider is enabled");
		pFunction->BindOutput(0, 'enab', "enabled");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::SetVelocity, "{993A5C3F-4771-4806-9E33-49A84FF01343}"_cry_guid, "SetVelocity");
		pFunction->SetDescription("Set Entity Velocity");
		pFunction->BindInput(1, 'vel', "velocity");
		pFunction->BindInput(2, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetVelocity, "{E20042B6-6CEB-490D-B1EE-FDC42ADF2FCF}"_cry_guid, "GetVelocity");
		pFunction->SetDescription("Get Entity Velocity");
		pFunction->BindInput(1, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		pFunction->BindOutput(0, 'vel', "velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::SetAngularVelocity, "{2A8D1DA3-BB7E-4FFA-B156-BDB2E96D90AE}"_cry_guid, "SetAngularVelocity");
		pFunction->SetDescription("Set Entity Angular Velocity");
		pFunction->BindInput(1, 'vel', "angular velocity");
		pFunction->BindInput(2, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::GetAngularVelocity, "{12BC10C4-5DC2-4C19-867F-26627619BFAC}"_cry_guid, "GetAngularVelocity");
		pFunction->SetDescription("Get Entity Angular Velocity");
		pFunction->BindInput(1, 'part', "part id", "Physical part id; <0 to ignore and use default", -1);
		pFunction->BindOutput(0, 'vel', "angular velocity");
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyImpulse, "{2E291BA6-9FBB-44DE-B5E0-B74B3339F448}"_cry_guid, "ApplyImpulse");
		pFunction->SetDescription("Applies an impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		pFunction->BindInput(2, 'pt', "Point", "Application point in the world space; (0,0,0) to ignore the point and apply the impulse to the center of mass", Vec3(ZERO));
		pFunction->BindInput(3, 'part', "Part Id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CRigidBodyComponent::ApplyAngularImpulse, "{839CBDF4-FD9D-450D-8773-11EA465CE74E}"_cry_guid, "ApplyAngularImpulse");
		pFunction->SetDescription("Applies an angular impulse to the physics object");
		pFunction->BindInput(1, 'for', "Force", "Force of the impulse", Vec3(ZERO));
		pFunction->BindInput(2, 'part', "Part Id", "Physical part id; <0 to ignore and use default", -1);
		componentScope.Register(pFunction);
	}
}

CBaseMeshComponent *CRagdollComponent::GetCharMesh() const
{
	if (IEntityComponent *pChar = GetEntity()->GetComponentByTypeId(Schematyc::GetTypeGUID<CAdvancedAnimationComponent>()))
		return static_cast<CAdvancedAnimationComponent*>(pChar);
	if (IEntityComponent *pChar = GetEntity()->GetComponentByTypeId(Schematyc::GetTypeGUID<CAnimatedMeshComponent>()))
		return static_cast<CAnimatedMeshComponent*>(pChar);
	return nullptr;
}

void CRagdollComponent::Physicalize()
{
	if (CBaseMeshComponent *pMesh = GetCharMesh())
	{
		pMesh->m_ragdollLod = 1;
		pMesh->m_ragdollStiffness = m_stiffness * (m_extraStiff ? -1 : 1);
	}

	CRigidBodyComponent::Physicalize();
}

void CRagdollComponent::ProcessEvent(const SEntityEvent& event)
{
	CRigidBodyComponent::ProcessEvent(event);
	if (event.event == ENTITY_EVENT_RESET)
		if (CBaseMeshComponent *pMesh = GetCharMesh())
			if (ICharacterInstance *pChar = GetEntity()->GetCharacter(pMesh->GetEntitySlotId()))
			{
				pChar->GetISkeletonPose()->SetDefaultPose();
				Physicalize();
			}
}

}
}
