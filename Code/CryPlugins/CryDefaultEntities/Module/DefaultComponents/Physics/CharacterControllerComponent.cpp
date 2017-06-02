#include "StdAfx.h"
#include "CharacterControllerComponent.h"

#include "SimplePhysicsComponent.h"

namespace Cry
{
namespace DefaultComponents
{
void RegisterCharacterControllerComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CCharacterControllerComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::IsOnGround, "{C41A9117-0292-4FF6-978A-F433345841B3}"_cry_guid, "IsOnGround");
			pFunction->SetDescription("Checks whether or not the character is on ground");
			pFunction->BindOutput(0, 'ongr', "OnGround");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetGroundNormal, "{72858886-D2D8-4FDD-A8C3-30A6AA85E7FD}"_cry_guid, "GetGroundNormal");
			pFunction->SetDescription("Gets the latest ground normal, returns last ground normal if in air");
			pFunction->BindOutput(0, 'norm', "Normal");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::IsWalking, "{80796905-AC9E-4C0F-A1F1-4CBE0B0807C3}"_cry_guid, "IsWalking");
			pFunction->SetDescription("Checks whether or not the player is moving on ground");
			pFunction->BindOutput(0, 'walk', "Walking");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::IsTurning, "{85280172-CA25-4436-B2BD-3DF2C5A19B40}"_cry_guid, "IsTurning");
			pFunction->SetDescription("Checks whether or not the player is turning around its Z-axis");
			pFunction->BindOutput(0, 'turn', "Turning");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::AddVelocity, "{B9D4C79B-1BDF-4A5C-969D-3456036C9CD4}"_cry_guid, "AddVelocity");
			pFunction->SetDescription("Adds velocity to that of the character");
			pFunction->BindInput(1, 'vel', "Velocity");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::SetVelocity, "{32A91404-8E63-4D59-8330-ADF5F2AB71DE}"_cry_guid, "SetVelocity");
			pFunction->SetDescription("Overrides the character's velocity");
			pFunction->BindInput(1, 'vel', "Velocity");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetMoveDirection, "{7D21BDFC-FE14-4020-A685-6F7917AC759C}"_cry_guid, "GetMovementDirection");
			pFunction->SetDescription("Gets the direction the character is moving in");
			pFunction->BindOutput(0, 'dir', "Direction");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetVelocity, "{BF80D631-05D3-4571-A51D-E0AF6A252F91}"_cry_guid, "GetVelocity");
			pFunction->SetDescription("Gets the character's velocity");
			pFunction->BindOutput(0, 'vel', "Velocity");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::Ragdollize, "{1927B2CC-9CC0-4872-93C1-33A4BFC86145}"_cry_guid, "ActivateRagdoll");
			pFunction->SetDescription("Turns the character into a ragdoll, and disables controlled movement");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::PhysicalizeCharacter, "{FA4C7D76-61BF-41DF-B14D-F72D161496EB}"_cry_guid, "DeactivateRagdoll");
			pFunction->SetDescription("Disables ragdoll and allows the player to control movement again");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::ActivateContext, "{22B083CA-9E9E-40CF-A506-D794B9FBFACF}"_cry_guid, "ActivateContext");
			pFunction->SetDescription("Activates a Mannequin context");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'cont', "Context Name");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::QueueFragment, "{BE05E85C-9AFD-4193-96B3-91A4B5BC3602}"_cry_guid, "QueueFragment");
			pFunction->SetDescription("Queues a Mannequin fragment for playback");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'frag', "Fragment Name");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::SetTag, "{216C303C-ED8F-4E3D-9762-4E33F69BF045}"_cry_guid, "SetTag");
			pFunction->SetDescription("Sets a Mannequin tag's state to true or false");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'tagn', "Tag Name");
			pFunction->BindInput(2, 'set', "Set");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::SetMotionParameter, "{471E9B94-0F64-4C39-9AB7-D882D8634B93}"_cry_guid, "SetMotionParameter");
			pFunction->SetDescription("Sets a motion parameter to affect a blend space");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'mtnp', "Motion Parameter");
			pFunction->BindInput(2, 'val', "Value");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetWorldTransform, "{EF15FE71-816A-4ABB-BCB1-6ED3EB541204}"_cry_guid, "GetWorldTransform");
			pFunction->SetDescription("Gets the transformation of this component in world space");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'tran', "World Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetLocalTransform, "{00B3294C-3590-4BF3-A06F-1C37B74E7B78}"_cry_guid, "GetLocalTransform");
			pFunction->SetDescription("Gets the transformation of this component in local space");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'tran', "Local Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::GetLocalRotation, "{3BC56A26-9DD1-4BCA-BB27-F92495E30078}"_cry_guid, "GetLocalRotation");
			pFunction->SetDescription("Gets the local rotation of this component");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'tran', "Local Rotation");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CCharacterControllerComponent::SetLocalRotation, "{06140663-1E8C-4A73-A954-0DBE1456007D}"_cry_guid, "SetLocalRotation");
			pFunction->SetDescription("Sets the local rotation of this component");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'tran', "Local Rotation");
			componentScope.Register(pFunction);
		}
		// Signals
		{
			auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL(CCharacterControllerComponent::SCollisionSignal);
			pSignal->SetDescription("Sent when the entity collided with another object");
			componentScope.Register(pSignal);
		}
	}
}

void CCharacterControllerComponent::ReflectType(Schematyc::CTypeDesc<CCharacterControllerComponent>& desc)
{
	desc.SetGUID(CCharacterControllerComponent::IID());
	desc.SetEditorCategory("Physics");
	desc.SetLabel("Character Controller");
	desc.SetDescription("Functionality for a standard FPS / TPS style walking character");
	//desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	// Entities can only have one physical entity type, thus these are incompatible
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CSimplePhysicsComponent>());

	CAdvancedAnimationComponent::ReflectMembers(desc);

	desc.AddMember(&CCharacterControllerComponent::m_physics, 'phys', "Physics", "Physics", "Physical properties for the character", CCharacterControllerComponent::SPhysics());
	desc.AddMember(&CCharacterControllerComponent::m_movement, 'move', "Movement", "Movement", "Movement properties for the character", CCharacterControllerComponent::SMovement());

	desc.AddMember(&CCharacterControllerComponent::m_bGroundAlignment, 'grou', "GroundAlign", "Use Ground Alignment", "Enables adjustment of leg positions to align to the ground surface", false);
}

static void ReflectType(Schematyc::CTypeDesc<CCharacterControllerComponent::SPhysics>& desc)
{
	desc.SetGUID("{3341F1DC-0753-466E-BC7A-FA77A49D3CB4}"_cry_guid);
	desc.AddMember(&CCharacterControllerComponent::SPhysics::m_mass, 'mass', "Mass", "Mass", "Mass of the character in kg", 80.f);
	desc.AddMember(&CCharacterControllerComponent::SPhysics::m_colliderSize, 'size', "Size", "Collider Size", "Dimensions of the capsule or cylinder", Vec3(0.45f, 0.45f, 0.935f * 0.5f));
	desc.AddMember(&CCharacterControllerComponent::SPhysics::m_bCapsule, 'caps', "Capsule", "Use Capsule", "Whether or not to use a capsule as the main collider, otherwise cylinder", false);

	desc.AddMember(&CCharacterControllerComponent::SPhysics::m_bSendCollisionSignal, 'send', "SendCollisionSignal", "Send Collision Signal", "Whether or not this component should listen for collisions and report them", false);
}

static void ReflectType(Schematyc::CTypeDesc<CCharacterControllerComponent::SMovement>& desc)
{
	desc.SetGUID("{71D2F72F-1AAF-4368-A356-FC07938837EB}"_cry_guid);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_airControlRatio, 'airc', "AirControl", "Air Control Ratio", "Indicates how much the character can move in the air, 0 means no movement while 1 means full control.", 1.f);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_airResistance, 'airr', "AirResistance", "Air Resistance", nullptr, 0.2f);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_inertia, 'iner', "Inertia", "Inertia Coefficient", "More amount gives less inertia, 0 being none", 8.f);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_inertiaAcceleration, 'inea', "InertiaAcc", "Inertia Acceleration Coefficient", "More amount gives less inertia on acceleration, 0 being none", 8.f);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_maxClimbAngle, 'maxc', "MaxClimb", "Maximum Climb Angle", "Maximum angle the character can climb", 50.0_degrees);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_maxJumpAngle, 'maxj', "MaxJump", "Maximum Jump Angle", "Maximum angle the character can jump at", 50.0_degrees);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_minSlideAngle, 'mins', "MinSlide", "Minimum Angle For Slide", "Minimum angle before the player starts sliding", 70.0_degrees);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_minFallAngle, 'minf', "MinFall", "Minimum Angle For Fall", "Minimum angle before the character starts falling", 80.0_degrees);
	desc.AddMember(&CCharacterControllerComponent::SMovement::m_maxGroundVelocity, 'maxg', "MaxGroundVelocity", "Maximum Surface Velocity", "Maximum velocity of the surface the character is on before they are considered airborne and slide off", DEG2RAD(50.f));
}

static void ReflectType(Schematyc::CTypeDesc<CCharacterControllerComponent::SCollisionSignal>& desc)
{
	desc.SetGUID("{24C421C5-3E60-42C2-B0D0-6BA3DFC7CF7C}"_cry_guid);
	desc.SetLabel("On Collision");
	desc.AddMember(&CCharacterControllerComponent::SCollisionSignal::otherEntity, 'ent', "OtherEntityId", "OtherEntityId", "Other Colliding Entity Id", INVALID_ENTITYID);
	desc.AddMember(&CCharacterControllerComponent::SCollisionSignal::surfaceType, 'srf', "SurfaceType", "SurfaceType", "Material Surface Type at the collision point", "");
}

void CCharacterControllerComponent::Run(Schematyc::ESimulationMode simulationMode)
{
	Initialize();

	// Start validating inputs
#ifndef RELEASE
	// Slide value will have no effect if larger than the fall angle, since we'll fall instead
	if (m_movement.m_minSlideAngle > m_movement.m_minFallAngle)
	{
		m_movement.m_minSlideAngle = m_movement.m_minFallAngle;
	}
#endif
}

void CCharacterControllerComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_UPDATE)
	{
		SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];

		IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics();
		CRY_ASSERT_MESSAGE(pPhysicalEntity != nullptr, "Physical entity removed without call to IEntity::UpdateComponentEventMask!");

		// Update stats
		pe_status_living livingStatus;
		if (pPhysicalEntity->GetStatus(&livingStatus) != 0)
		{
			m_bOnGround = !livingStatus.bFlying;

			// Store the ground normal in case it is needed
			// Note that users have to check if we're on ground before using, is considered invalid in air.
			m_groundNormal = livingStatus.groundSlope;
		}

		ICharacterInstance* pCharacter = GetCharacter();
		Matrix34 characterTransform = m_pEntity->GetSlotWorldTM(GetEntitySlotId());

		// Get the player's velocity from physics
		pe_status_dynamics playerDynamics;
		if (pPhysicalEntity->GetStatus(&playerDynamics) != 0 && pCharacter != nullptr)
		{
			// Set turn rate as the difference between previous and new entity rotation
			m_turnAngle = Ang3::CreateRadZ(characterTransform.GetColumn1(), m_prevForwardDir) / pCtx->fFrameTime;
			m_prevForwardDir = characterTransform.GetColumn1();
			m_velocity = playerDynamics.v;

			float travelAngle = Ang3::CreateRadZ(characterTransform.GetColumn1(), playerDynamics.v.GetNormalized());
			float travelSpeed = playerDynamics.v.GetLength2D();

			// Set the travel speed based on the physics velocity magnitude
			// Keep in mind that the maximum number for motion parameters is 10.
			// If your velocity can reach a magnitude higher than this, divide by the maximum theoretical account and work with a 0 - 1 ratio.
			pCharacter->GetISkeletonAnim()->SetDesiredMotionParam(eMotionParamID_TravelSpeed, travelSpeed, 0.f);

			// Update the turn speed in CryAnimation, note that the maximum motion parameter (10) applies here too.
			pCharacter->GetISkeletonAnim()->SetDesiredMotionParam(eMotionParamID_TurnAngle, m_turnAngle, 0.f);
			pCharacter->GetISkeletonAnim()->SetDesiredMotionParam(eMotionParamID_TravelAngle, travelAngle, 0.f);

			if (IsOnGround())
			{
				// Calculate slope value
				Vec3 groundNormal = GetGroundNormal().value * Quat(characterTransform);
				groundNormal.x = 0.0f;
				float cosine = Vec3Constants<float>::fVec3_OneZ | groundNormal;
				Vec3 sine = Vec3Constants<float>::fVec3_OneZ % groundNormal;

				float travelSlope = atan2f(sgn(sine.x) * sine.GetLength(), cosine);

				pCharacter->GetISkeletonAnim()->SetDesiredMotionParam(eMotionParamID_TravelSlope, travelSlope, 0.f);
			}

			if (m_pPoseAligner != nullptr && m_pPoseAligner->Initialize(*m_pEntity))
			{
				m_pPoseAligner->SetBlendWeight(1.f);
				m_pPoseAligner->Update(pCharacter, QuatT(characterTransform), pCtx->fFrameTime);
			}
		}
	}
	else if (event.event == ENTITY_EVENT_COLLISION)
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

	CAdvancedAnimationComponent::ProcessEvent(event);
}

uint64 CCharacterControllerComponent::GetEventMask() const
{
	uint64 eventMask = CAdvancedAnimationComponent::GetEventMask();

	// Only update when we have a physical entity
	if (m_pEntity->GetPhysicalEntity() != nullptr)
	{
		eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}

	if (m_physics.m_bSendCollisionSignal)
	{
		eventMask |= BIT64(ENTITY_EVENT_COLLISION);
	}

	return eventMask;
}
}
}
