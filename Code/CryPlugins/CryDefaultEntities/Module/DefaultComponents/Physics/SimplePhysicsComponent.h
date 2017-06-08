#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CSimplePhysicsComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			static void ReflectType(Schematyc::CTypeDesc<CSimplePhysicsComponent>& desc);

			struct SCollisionSignal
			{
				EntityId otherEntity = INVALID_ENTITYID;
				Schematyc::SurfaceTypeName surfaceType;

				SCollisionSignal() {};
				SCollisionSignal(EntityId id, const Schematyc::SurfaceTypeName &srfType) : otherEntity(id), surfaceType(srfType) {}
			};

			enum class EPhysicalType
			{
				Static = PE_STATIC,
				Rigid = PE_RIGID
			};

			CSimplePhysicsComponent() {}
			virtual ~CSimplePhysicsComponent();

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

			virtual void SetAngularVelocity(const Vec3& angularVelocity)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_action_set_velocity action_set_velocity;
					action_set_velocity.w = angularVelocity;
					pPhysicalEntity->Action(&action_set_velocity);
				}
			}

			Vec3 GetAngularVelocity() const
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
				{
					pe_status_dynamics dynStatus;
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

			virtual void ApplyImpulse(const Vec3& force)
			{
				// Only dispatch the impulse to physics if one was provided
				if (!force.IsZero())
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
					{
						pe_action_impulse impulseAction;
						impulseAction.impulse = force;

						pPhysicalEntity->Action(&impulseAction);
					}
				}
			}

			virtual void ApplyAngularImpulse(const Vec3& force)
			{
				// Only dispatch the impulse to physics if one was provided
				if (!force.IsZero())
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysics())
					{
						pe_action_impulse impulseAction;
						impulseAction.angImpulse = force;

						pPhysicalEntity->Action(&impulseAction);
					}
				}
			}

			static CryGUID& IID()
			{
				static CryGUID id = "{912C6CE8-56F7-4FFA-9134-F98D4E307BD6}"_cry_guid;
				return id;
			}

		protected:
			void Physicalize();

		public:

			bool m_bEnabledByDefault = true;
			EPhysicalType m_type = EPhysicalType::Rigid;
			bool m_bSendCollisionSignal = false;
		};
	}
}