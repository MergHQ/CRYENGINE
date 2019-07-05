#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <Cry3DEngine/ISurfaceType.h>
#include <CryPhysics/physinterface.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		// Interface for physicalizing as PE_PARTICLE
		class CPhysParticleComponent : public IEntityComponent
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
			static void ReflectType(Schematyc::CTypeDesc<CPhysParticleComponent>& desc)
			{
				desc.SetGUID("{45DB74C2-18A3-460D-BC78-541555CE7424}"_cry_guid);
				desc.SetEditorCategory("Physics");
				desc.SetLabel("PhysParticle");
				desc.SetDescription("Physicalizes the entity as a particle");
				desc.SetIcon("icons:ObjectTypes/object.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{98183F31-A685-43CD-92A9-815274F0A81C}"_cry_guid); // Character Controller
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{47EBC019-41CB-415E-AB57-2A3A99B918C2}}"_cry_guid);	// vehicle
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{EC7F145B-D48F-4863-B9C2-3D3E2C8DCC61}"_cry_guid); // area
				desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, "{912C6CE8-56F7-4FFA-9134-F98D4E307BD6}"_cry_guid); // rigidbody

				desc.AddMember(&CPhysParticleComponent::m_mass, 'mass', "Mass", "Mass", "Particle's mass", 0.2f);
				desc.AddMember(&CPhysParticleComponent::m_size, 'size', "Size", "Size", "'Size' in the direction of movement. If particle is imagined as a sphere, 'size' is its diameter", 0.05f);
				desc.AddMember(&CPhysParticleComponent::m_thickness, 'thik', "Thickness", "Thickness", "Size along the normal when lying (can be different from size in the direction of movement)", 0.05f);
				desc.AddMember(&CPhysParticleComponent::m_normal, 'norm', "Normal", "Normal", "Direction in the entity space that gets aligned with the surface normal when lying", Vec3(0, 0, 1));
				desc.AddMember(&CPhysParticleComponent::m_rollAxis, 'rlax', "RollAxis", "Roll Axis", "Roll axis in the entity space", Vec3(0, 0, 1));
				desc.AddMember(&CPhysParticleComponent::m_thrust, 'thst', "Thrust", "Thrust", "Acceleration in the direction movement", 0);
				desc.AddMember(&CPhysParticleComponent::m_lift, 'lift', "Lift", "Lift", "Acceleration along the entity 'up' direction", 0);
				desc.AddMember(&CPhysParticleComponent::m_gravity, 'grav', "Gravity", "Gravity Scale", "Particle's acceleration due to gravity = world gravity * this scale", 1);
				desc.AddMember(&CPhysParticleComponent::m_gravityWater, 'wgrv', "WGravity", "Water Gravity Scale", "World gravity scaling coefficient when in water", 0.8f);
				desc.AddMember(&CPhysParticleComponent::m_airResistance, 'kair', "AirResistance", "Air Resistance Scale", "Scale to the global air resistance coefficient", 1);
				desc.AddMember(&CPhysParticleComponent::m_waterResistance, 'kawt', "WaterResistance", "Water Resistance Scale", "Scale to the global water resistance coefficient", 1);
				desc.AddMember(&CPhysParticleComponent::m_minBounceVel, 'minb', "MinBounceVel", "Bounce Speed Threshold", "Minimal approach velocity (along the normal) that results in a bounce", 1.5f);
				desc.AddMember(&CPhysParticleComponent::m_minVel, 'minv', "MinVel", "Sleep Threshold", "Sends the particle to sleep if its speed falls below this threshold", 0.02f);
				desc.AddMember(&CPhysParticleComponent::m_awake, 'awak', "Awake", "Awake", "Particle is awake when the game starts", true);
				desc.AddMember(&CPhysParticleComponent::m_isNetworked, 'netw', "Networked", "Network Synced", "Syncs the physical entity over the network, and keeps it in sync with the server", false);
				desc.AddMember(&CPhysParticleComponent::m_bSendCollisionSignal, 'send', "SendCollisionSignal", "Send Collision Signal", "Whether or not this component should listen for collisions and report them", false);
				desc.AddMember(&CPhysParticleComponent::m_traceable, 'trac', "Traceable", "Traceable", "Particle is registered in the entity grid and can be seen by raytracing end entities-in-box queries", true);
				desc.AddMember(&CPhysParticleComponent::m_singleContact, 'sngc', "SingleContact", "Single Contact", "Stops the particle after its first contact", false);
				desc.AddMember(&CPhysParticleComponent::m_constantOrientation, 'cntq', "ConstantRot", "Constant Orientation", "Particle never changes rotation on its own", false);
				desc.AddMember(&CPhysParticleComponent::m_noRoll, 'nrol', "NoRoll", "No Roll", "Particle slides on the ground instead of rolling (which also triggers ground alignment along 'normal')", false);
				desc.AddMember(&CPhysParticleComponent::m_noSpin, 'nspn', "NoSpin", "No Spin", "Particle doesn't spin when flying", false);
				desc.AddMember(&CPhysParticleComponent::m_noPathAlignment, 'npal', "NoPath", "No Path Alignment", "Particle doesn't align its (0,1,0) axis with the movement direction", false);
				desc.AddMember(&CPhysParticleComponent::m_noSelfCollisions, 'nscl', "NoSelfColl", "No Self Collisions", "Doesn't collide with other particles with the same flag", false);
				desc.AddMember(&CPhysParticleComponent::m_noImpulse, 'nimp', "NoImpulse", "No Impulse", "Particle doesn't add impulse on contact", false);
				desc.AddMember(&CPhysParticleComponent::m_surfType, 'surf', "SurfaceType", "Surface Type", "Particle's surface type (used for bounciness, friction, and particle effects)", Schematyc::SurfaceTypeName());
			}

			struct SCollisionSignal
			{
				Schematyc::ExplicitEntityId otherEntity;
				Schematyc::SurfaceTypeName surfaceType;

				SCollisionSignal() {};
				SCollisionSignal(EntityId id, const Schematyc::SurfaceTypeName &srfType) : otherEntity(Schematyc::ExplicitEntityId(id)), surfaceType(srfType) {}
			};

			CPhysParticleComponent() {}
			virtual ~CPhysParticleComponent();

			virtual void SetVelocity(const Vec3& velocity)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_action_set_velocity action_set_velocity;
					action_set_velocity.v = velocity;
					pPhysicalEntity->Action(&action_set_velocity);
				}
			}

			Vec3 GetVelocity() const
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_status_dynamics dynStatus;
					if (pPhysicalEntity->GetStatus(&dynStatus))
					{
						return dynStatus.v;
					}
				}

				return ZERO;
			}

			virtual void SetAngularVelocity(const Vec3& velocity)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_action_set_velocity action_set_velocity;
					action_set_velocity.w = velocity;
					pPhysicalEntity->Action(&action_set_velocity);
				}
			}

			virtual void Enable(bool bEnable)
			{
				m_pEntity->EnablePhysics(bEnable);
			}

			bool IsEnabled() const
			{
				return m_pEntity->IsPhysicsEnabled();
			}

		protected:
			virtual void Physicalize();

		public:
			float m_mass = 0.2f;
			float m_size = 0.05f;
			float m_thickness = 0.05f;
			float m_thrust = 0;
			float m_lift = 0;
			float m_gravity = 1;
			float m_gravityWater = 0.8f;
			float m_airResistance = 1;
			float m_waterResistance = 1;
			float m_minBounceVel = 1.5f;
			float m_minVel = 0.02f;

			Schematyc::UnitLength<Vec3> m_normal = Vec3(0, 0, 1);
			Schematyc::UnitLength<Vec3> m_rollAxis = Vec3(0, 0, 1);

			bool m_isNetworked = false;
			bool m_bSendCollisionSignal = false;
			bool m_awake = true;
			bool m_traceable = true;
			bool m_singleContact = false;
			bool m_constantOrientation = false;
			bool m_noRoll = false;
			bool m_noSpin = false;
			bool m_noPathAlignment = false;
			bool m_noSelfCollisions = false;
			bool m_noImpulse = false;

			Schematyc::SurfaceTypeName m_surfType = "default";
			int                        m_idxSurfType = 0; // cached index; non persistent
		};
	}
}