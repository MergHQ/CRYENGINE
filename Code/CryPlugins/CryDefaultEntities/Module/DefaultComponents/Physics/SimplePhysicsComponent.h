#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CSimplePhysicsComponent
			: public IEntityComponent
		{
		public:
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
			virtual ~CSimplePhysicsComponent()
			{
				SEntityPhysicalizeParams physParams;
				physParams.type = PE_NONE;
				m_pEntity->Physicalize(physParams);
			}

			// IEntityComponent
			virtual void Initialize() override
			{
				SEntityPhysicalizeParams physParams;
				physParams.type = (int)m_type;

				// Don't physicalize a slot by default
				physParams.nSlot = std::numeric_limits<int>::max();
				m_pEntity->Physicalize(physParams);

				Enable(m_bEnabledByDefault);
			}

			virtual void ProcessEvent(SEntityEvent& event) override;
			virtual uint64 GetEventMask() const override;

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			void SetVelocity(const Vec3& velocity)
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

			void SetAngularVelocity(const Vec3& angularVelocity)
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

			void Enable(bool bEnable)
			{
				m_pEntity->EnablePhysics(bEnable);
			}

			bool IsEnabled() const
			{
				return m_pEntity->IsPhysicsEnabled();
			}

			void ApplyImpulse(const Vec3& force)
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

			void ApplyAngularImpulse(const Vec3& force)
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

			bool m_bEnabledByDefault = true;
			EPhysicalType m_type = EPhysicalType::Rigid;
			bool m_bSendCollisionSignal = false;
		};
	}
}