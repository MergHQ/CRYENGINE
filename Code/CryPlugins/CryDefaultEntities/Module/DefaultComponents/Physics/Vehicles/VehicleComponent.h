#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

#include <CryMath/Angle.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CVehiclePhysicsComponent
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
			struct SCollisionSignal
			{
				EntityId otherEntity = INVALID_ENTITYID;
				Schematyc::SurfaceTypeName surfaceType;

				SCollisionSignal() {};
				SCollisionSignal(EntityId id, const Schematyc::SurfaceTypeName &srfType) : otherEntity(id), surfaceType(srfType) {}
			};

			struct SEngineParams
			{
				inline bool operator==(const SEngineParams &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				Schematyc::Range<0, 10000000> m_power = 50000.f;
				float m_maxRPM = 1200.f;
				float m_minRPM = 60.f;
				float m_idleRPM = 120.f;
				float m_startRPM = 400.f;
			};

			struct SGearParams
			{
				inline bool operator==(const SGearParams &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				struct SGear
				{
					inline bool operator==(const SGear &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
					inline bool operator!=(const SGear &rhs) const { return 0 != memcmp(this, &rhs, sizeof(rhs)); }

					void Serialize(Serialization::IArchive& archive)
					{
						archive(m_ratio, "Ratio", "Ratio");
					}

					Schematyc::Range<0, 2> m_ratio;
				};

				Schematyc::CArray<SGear> m_gears;

				float m_shiftUpRPM = 600.f;
				float m_shiftDownRPM = 240.f;

				float m_directionSwitchRPM = 10.f;
			};

			enum class EGear
			{
				Reverse = -1,
				Neutral,
				// First forward gear, note how there can be more specified by the user
				Forward,

				DefaultCount = Forward + 2
			};

			CVehiclePhysicsComponent();
			virtual ~CVehiclePhysicsComponent();

			static void ReflectType(Schematyc::CTypeDesc<CVehiclePhysicsComponent>& desc);
			static CryGUID& IID()
			{
				static CryGUID id = "{47EBC019-41CB-415E-AB57-2A3A99B918C2}}"_cry_guid;
				return id;
			}

			virtual void UseHandbrake(bool bSet) 
			{
				pe_action_drive driveAction;
				driveAction.bHandBrake = bSet ? 1 : 0;
				m_pEntity->GetPhysicalEntity()->Action(&driveAction);
			}
			bool IsUsingHandbrake() const { return m_vehicleStatus.bHandBrake != 0; }

			virtual void SetThrottle(Schematyc::Range<0, 1> ratio) 
			{ 
				pe_action_drive driveAction;
				driveAction.pedal = ratio;
				m_pEntity->GetPhysicalEntity()->Action(&driveAction);
			}
			float GetThrottle() const { return m_vehicleStatus.pedal; }

			void SetBrake(Schematyc::Range<0, 1> ratio) 
			{
				pe_action_drive driveAction;
				driveAction.pedal = -ratio;
				m_pEntity->GetPhysicalEntity()->Action(&driveAction);
			}
			Schematyc::Range<0, 1> GetBrake() const { return m_vehicleStatus.footbrake; }

			virtual void SetCurrentGear(int gearId) 
			{
				pe_action_drive driveAction;
				driveAction.iGear = gearId + 1;
				m_pEntity->GetPhysicalEntity()->Action(&driveAction);
			}
			int GetCurrentGear() const { return m_vehicleStatus.iCurGear - 1; }

			virtual void GearUp() { SetCurrentGear(min(m_vehicleStatus.iCurGear + 1, (int)m_gearParams.m_gears.Size())); }
			virtual void GearDown() { SetCurrentGear(max(m_vehicleStatus.iCurGear - 1, (int)EGear::Reverse + 1)); }

			virtual void SetSteeringAngle(CryTransform::CAngle angle) 
			{
				pe_action_drive driveAction;
				driveAction.steer = angle.ToRadians();
				m_pEntity->GetPhysicalEntity()->Action(&driveAction);
			}
			CryTransform::CAngle GetSteeringAngle() const { return CryTransform::CAngle::FromRadians(m_vehicleStatus.steer); }

			float GetEngineRPM() const { return m_vehicleStatus.engineRPM; }
			float GetTorque() const { return m_vehicleStatus.drivingTorque; }
			bool HasWheelContact() const { return m_vehicleStatus.bWheelContact != 0; }

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

		protected:
			// Needs to be persistent since physics is on another thread
			std::vector<float> m_gearRatios;

			SEngineParams m_engineParams;
			SGearParams m_gearParams;

			pe_status_vehicle m_vehicleStatus;
			bool m_bSendCollisionSignal = false;
		};
	}
}