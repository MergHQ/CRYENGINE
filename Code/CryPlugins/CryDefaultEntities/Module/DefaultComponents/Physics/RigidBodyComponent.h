#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryPhysics/physinterface.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		// Interface for physicalizing as static or rigid
		class CRigidBodyComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(const SEntityEvent& event) override;
			virtual Cry::Entity::EventFlags GetEventMask() const override;

			virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
			virtual NetworkAspectType GetNetSerializeAspectMask() const override { return eEA_Physics; };
			// ~IEntityComponent

		public:
			struct SBuoyancyParameters
			{
				inline bool operator==(const SBuoyancyParameters &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				static void ReflectType(Schematyc::CTypeDesc<SBuoyancyParameters>& desc)
				{
					desc.SetGUID("{2C5EFA87-F7D0-43F1-90B8-576EEB60FC37}"_cry_guid);
					desc.SetLabel("Buoyancy Parameters");
					desc.AddMember(&CRigidBodyComponent::SBuoyancyParameters::damping, 'damp', "Damping", "Damping", "Uniform damping while submerged, will be scaled with submerged fraction", 0.f);
					desc.AddMember(&CRigidBodyComponent::SBuoyancyParameters::density, 'dens', "Density", "Density", "Density of the fluid", 1000.f);
					desc.AddMember(&CRigidBodyComponent::SBuoyancyParameters::resistance, 'rest', "Resistance", "Resistance", "Resistance of the fluid", 1000.f);
				}

				Schematyc::PositiveFloat damping = 0.0f;
				Schematyc::PositiveFloat density = 1000.0f;
				Schematyc::PositiveFloat resistance = 1000.0f;
			};

			struct SSimulationParameters
			{
				inline bool operator==(const SSimulationParameters &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				static void ReflectType(Schematyc::CTypeDesc<SSimulationParameters>& desc)
				{
					desc.SetGUID("{1A56128F-48CB-48A7-A674-FA9AEB5A7AFF}"_cry_guid);
					desc.SetLabel("Simulation Parameters");
					desc.AddMember(&CRigidBodyComponent::SSimulationParameters::maxTimeStep, 'maxt', "MaxTimeStep", "Maximum Time Step", "The largest time step the entity can make before splitting. Smaller time steps increase stability (can be required for long and thin objects, for instance), but are more expensive.", 0.02f);
					desc.AddMember(&CRigidBodyComponent::SSimulationParameters::sleepSpeed, 'slps', "SleepSpeed", "Sleep Speed", "If the object's kinetic energy falls below some limit over several frames, the object is considered sleeping. This limit is proportional to the square of the sleep speed value.", 0.04f);
					desc.AddMember(&CRigidBodyComponent::SSimulationParameters::damping, 'damp', "Damping", "Damping", "This sets the strength of damping on an object's movement. Most objects can work with 0 damping; if an object has trouble coming to rest, try values like 0.2-0.3.", 0.0f);
				}

				Schematyc::PositiveFloat maxTimeStep = 0.02f;
				Schematyc::PositiveFloat sleepSpeed = 0.04f;
				float                    damping = 0.0f;
			};

			static void ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent>& desc)
			{
				desc.SetGUID("{912C6CE8-56F7-4FFA-9134-F98D4E307BD6}"_cry_guid);
				desc.SetEditorCategory("Physics");
				desc.SetLabel("Rigidbody");
				desc.SetDescription("Creates a physical representation of the entity, ");
				desc.SetIcon("icons:ObjectTypes/object.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

				// Mark the Character Controller component as incompatible
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{98183F31-A685-43CD-92A9-815274F0A81C}"_cry_guid);
				// Mark the Vehicle Physics component as incompatible
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{47EBC019-41CB-415E-AB57-2A3A99B918C2}}"_cry_guid);
				// Mark the Area component as incompatible
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{EC7F145B-D48F-4863-B9C2-3D3E2C8DCC61}"_cry_guid);
				// Static meshes must be initialized since they need to have slots assigned during physicalization
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::SoftDependency, "{6DDD0033-6AAA-4B71-B8EA-108258205E29}"_cry_guid);

				desc.AddMember(&CRigidBodyComponent::m_isNetworked, 'netw', "Networked", "Network Synced", "Syncs the physical entity over the network, and keeps it in sync with the server", false);
				desc.AddMember(&CRigidBodyComponent::m_isEnabledByDefault, 'enab', "EnabledByDefault", "Enabled by Default", "Whether the component is enabled by default", true);
				desc.AddMember(&CRigidBodyComponent::m_isResting, 'res', "Resting", "Resting", "If resting is enabled the object will only start to be simulated if it was hit by something else.", false);
				desc.AddMember(&CRigidBodyComponent::m_type, 'type', "Type", "Type", "Type of physicalized object to create", CRigidBodyComponent::EPhysicalType::Rigid);
				desc.AddMember(&CRigidBodyComponent::m_bSendCollisionSignal, 'send', "SendCollisionSignal", "Send Collision Signal", "Whether or not this component should listen for collisions and report them", false);
				desc.AddMember(&CRigidBodyComponent::m_buoyancyParameters, 'buoy', "Buoyancy", "Buoyancy Parameters", "Fluid behavior related to this entity", SBuoyancyParameters());
				desc.AddMember(&CRigidBodyComponent::m_simulationParameters, 'simp', "Simulation", "Simulation Parameters", "Parameters related to the simulation of this entity", SSimulationParameters());
			}

			struct SCollisionSignal
			{
				Schematyc::ExplicitEntityId otherEntity;
				Schematyc::SurfaceTypeName surfaceType;

				SCollisionSignal() {};
				SCollisionSignal(EntityId id, const Schematyc::SurfaceTypeName &srfType) : otherEntity(Schematyc::ExplicitEntityId(id)), surfaceType(srfType) {}
			};

			enum class EPhysicalType
			{
				Static = PE_STATIC,
				Rigid = PE_RIGID,
				Articulated = PE_ARTICULATED
			};

			CRigidBodyComponent() {}
			virtual ~CRigidBodyComponent();

			virtual void SetVelocity(const Vec3& velocity, int idPart = -1)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_action_set_velocity action_set_velocity;
					action_set_velocity.v = velocity;
					if (idPart >= 0) action_set_velocity.partid = idPart;
					pPhysicalEntity->Action(&action_set_velocity);
				}
			}

			Vec3 GetVelocity(int idPart = -1) const
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_status_dynamics dynStatus;
					if (idPart >= 0) dynStatus.partid = idPart;
					if (pPhysicalEntity->GetStatus(&dynStatus))
					{
						return dynStatus.v;
					}
				}

				return ZERO;
			}

			virtual void SetAngularVelocity(const Vec3& angularVelocity, int idPart = -1)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_action_set_velocity action_set_velocity;
					action_set_velocity.w = angularVelocity;
					if (idPart >= 0) action_set_velocity.partid = idPart;
					pPhysicalEntity->Action(&action_set_velocity);
				}
			}

			Vec3 GetAngularVelocity(int idPart = -1) const
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_status_dynamics dynStatus;
					if (idPart >= 0) dynStatus.partid = idPart;
					if (pPhysicalEntity->GetStatus(&dynStatus))
					{
						return dynStatus.w;
					}
				}

				return ZERO;
			}

			virtual void Enable(bool bEnable)
			{
				m_pEntity->EnablePhysics(bEnable);
			}

			bool IsEnabled() const
			{
				return m_pEntity->IsPhysicsEnabled();
			}

			virtual void ApplyImpulse(const Vec3& force, const Vec3& point = Vec3(ZERO), int idPart = -1)
			{
				// Only dispatch the impulse to physics if one was provided
				if (!force.IsZero())
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
					{
						pe_action_impulse impulseAction;
						impulseAction.impulse = force;
						if (point.len2()) impulseAction.point = point;
						if (idPart >= 0) impulseAction.partid = idPart;

						pPhysicalEntity->Action(&impulseAction);
					}
				}
			}

			virtual void ApplyAngularImpulse(const Vec3& force, int idPart = -1)
			{
				// Only dispatch the impulse to physics if one was provided
				if (!force.IsZero())
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
					{
						pe_action_impulse impulseAction;
						impulseAction.angImpulse = force;
						if (idPart >= 0) impulseAction.partid = idPart;

						pPhysicalEntity->Action(&impulseAction);
					}
				}
			}

		protected:
			virtual void Physicalize();

		public:
			bool m_isNetworked = false;

			bool m_isResting = false;

			bool m_isEnabledByDefault = true;
			EPhysicalType m_type = EPhysicalType::Rigid;
			bool m_bSendCollisionSignal = false;
			SBuoyancyParameters m_buoyancyParameters;
			SSimulationParameters m_simulationParameters;
		};

		static void ReflectType(Schematyc::CTypeDesc<CRigidBodyComponent::EPhysicalType>& desc)
		{
			desc.SetGUID("{C8B86CC7-E86D-46BA-9110-7FEA8052FABC}"_cry_guid);
			desc.SetLabel("Simple Physicalization Type");
			desc.SetDescription("Determines how to physicalize a simple (CGF) object");
			desc.SetDefaultValue(CRigidBodyComponent::EPhysicalType::Rigid);
			desc.AddConstant(CRigidBodyComponent::EPhysicalType::Static, "Static", "Static");
			desc.AddConstant(CRigidBodyComponent::EPhysicalType::Rigid, "Rigid", "Rigid");
		}
	}
}