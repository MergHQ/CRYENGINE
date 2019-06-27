#pragma once

#include "RigidBodyComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		// Interface for physicalizing as static or rigid
		class CRagdollComponent	: public CRigidBodyComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			virtual void ProcessEvent(const SEntityEvent& event);
			virtual Cry::Entity::EventFlags GetEventMask() const { return CRigidBodyComponent::GetEventMask() | ENTITY_EVENT_RESET; }

		public:
			static void ReflectType(Schematyc::CTypeDesc<CRagdollComponent>& desc)
			{
				desc.SetGUID("{BE737930-0F3D-482E-8194-F2C22B546E41}"_cry_guid);
				desc.SetEditorCategory("Physics");
				desc.SetLabel("Ragdoll");
				desc.SetDescription("Physicalizes a single animated mesh or advanced animation component as a ragdoll");
				desc.SetIcon("icons:ObjectTypes/object.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{912C6CE8-56F7-4FFA-9134-F98D4E307BD6}"_cry_guid); // rigidbody
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{98183F31-A685-43CD-92A9-815274F0A81C}"_cry_guid); // Character Controller
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{47EBC019-41CB-415E-AB57-2A3A99B918C2}}"_cry_guid);	// vehicle
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{EC7F145B-D48F-4863-B9C2-3D3E2C8DCC61}"_cry_guid); // area
				// Soft dependency on AnimatedMesh and AndvancedAnimation to make sure they have slots assigned during Ragdoll initialization
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::SoftDependency, "{5F543092-53EA-46D7-9376-266E778317D7}"_cry_guid);
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::SoftDependency, "{3CD5DDC5-EE15-437F-A997-79C2391537FE}"_cry_guid);

				desc.AddMember(&CRigidBodyComponent::m_isNetworked, 'netw', "Networked", "Network Synced", "Syncs the physical entity over the network, and keeps it in sync with the server", false);
				desc.AddMember(&CRigidBodyComponent::m_isResting, 'res', "Resting", "Resting", "If resting is enabled the object will only start to be simulated if it was hit by something else.", false);
				desc.AddMember(&CRigidBodyComponent::m_buoyancyParameters, 'buoy', "Buoyancy", "Buoyancy Parameters", "Fluid behavior related to this entity", SBuoyancyParameters());
				desc.AddMember(&CRigidBodyComponent::m_simulationParameters, 'simp', "Simulation", "Simulation Parameters", "Parameters related to the simulation of this entity", SSimulationParameters());
				desc.AddMember(&CRagdollComponent::m_stiffness, 'stif', "Stiffness", "Stiffness", "If >0, allows ragdoll to play animations by applying torques at joints", 0.0f);
				desc.AddMember(&CRagdollComponent::m_extraStiff, 'xstf', "ExtraStiff", "Extra Stiff Mode", 
					"Much more stable with high stiffness values, but uses the same stiffness for playing animation and enforcing locked axes/limits", false);
			}

			CRagdollComponent() { m_type = EPhysicalType::Articulated; }
			virtual ~CRagdollComponent() {}

		protected:
			virtual void Physicalize();

			CBaseMeshComponent *GetCharMesh() const;

			float m_stiffness = 0.0f;
			bool  m_extraStiff = false;
		};
	}
}