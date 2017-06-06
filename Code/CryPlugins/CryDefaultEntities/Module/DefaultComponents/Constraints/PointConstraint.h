#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CPointConstraintComponent
			: public IEntityComponent
		{
			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			virtual ~CPointConstraintComponent();

			static void ReflectType(Schematyc::CTypeDesc<CPointConstraintComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{4A010113-38A2-4CB0-8C98-BC1956675F40}"_cry_guid;
				return id;
			}

			virtual void ConstrainToEntity(Schematyc::ExplicitEntityId targetEntityId, bool bDisableCollisionsWith)
			{
				if ((EntityId)targetEntityId != INVALID_ENTITYID)
				{
					if (IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity((EntityId)targetEntityId))
					{
						if (IPhysicalEntity* pPhysicalEntity = pTargetEntity->GetPhysicalEntity())
						{
							ConstrainTo(pPhysicalEntity, bDisableCollisionsWith);
						}
					}
				}
			}

			virtual void ConstrainToPoint()
			{
				ConstrainTo(WORLD_ENTITY);
			}

			virtual void ConstrainTo(IPhysicalEntity* pEntity, bool bDisableCollisionsWith = false)
			{
				Remove();

				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					Matrix34 slotTransform = GetWorldTransformMatrix();

					pe_action_add_constraint constraint;
					constraint.flags = world_frames | constraint_no_tears;
					constraint.pt[0] = constraint.pt[1] = slotTransform.GetTranslation();
					constraint.qframe[0] = constraint.qframe[1] = Quat(slotTransform) * Quat::CreateRotationV0V1(Vec3(1, 0, 0), m_axis);
					constraint.xlimits[0] = m_rotationLimitsX0.ToRadians();
					constraint.xlimits[1] = m_rotationLimitsX1.ToRadians();
					constraint.yzlimits[0] = m_rotationLimitsYZ0.ToRadians();
					constraint.yzlimits[1] = m_rotationLimitsYZ1.ToRadians();
					constraint.damping = m_damping;

					if (bDisableCollisionsWith && pEntity != WORLD_ENTITY && pEntity != pPhysicalEntity)
					{
						constraint.flags |= constraint_ignore_buddy | constraint_inactive;

						constraint.pBuddy = pPhysicalEntity;
						pEntity->Action(&constraint);

						constraint.flags &= ~constraint_inactive;
					}

					constraint.pBuddy = pEntity;

					m_constraintIds.emplace_back(pPhysicalEntity->Action(&constraint));
				}
			}

			virtual void Remove()
			{
				for (int constraintId : m_constraintIds)
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
					{
						pe_action_update_constraint constraint;
						constraint.idConstraint = constraintId;
						constraint.bRemove = 1;
						pPhysicalEntity->Action(&constraint);
					}
				}

				m_constraintIds.clear();
			}

			virtual void Activate(bool bActivate)
			{
				m_bActive = bActivate;

				m_pEntity->UpdateComponentEventMask(this);
			}
			bool IsActive() const { return m_bActive; }

			virtual void SetAxis(Vec3& axis) { m_axis = axis; }
			const Vec3& GetAxis() const { return m_axis; }

			virtual void SetRotationLimitsX(CryTransform::CAngle minValue, CryTransform::CAngle maxValue) { m_rotationLimitsX0 = minValue; m_rotationLimitsX1 = maxValue; }
			CryTransform::CAngle GetRotationLimitXMin() const { return m_rotationLimitsX0; }
			CryTransform::CAngle GetRotationLimitXMax() const { return m_rotationLimitsX1; }

			virtual void SetRotationLimitsyz(CryTransform::CAngle minValue, CryTransform::CAngle maxValue) { m_rotationLimitsYZ0 = minValue; m_rotationLimitsYZ1 = maxValue; }
			CryTransform::CAngle GetRotationLimitYZMin() const { return m_rotationLimitsYZ0; }
			CryTransform::CAngle GetRotationLimitYZMax() const { return m_rotationLimitsYZ1; }

			virtual void SetDamping(float damping) { m_damping = damping; }
			float GetDamping() const { return m_damping; }

		protected:
			void Reset();

		protected:
			bool m_bActive = false;

			Schematyc::UnitLength<Vec3> m_axis = Vec3(0, 0, 1);

			CryTransform::CClampedAngle<-360, 360> m_rotationLimitsX0 = 0.0_degrees;
			CryTransform::CClampedAngle<-360, 360> m_rotationLimitsX1 = 360.0_degrees;
			CryTransform::CClampedAngle<-360, 360> m_rotationLimitsYZ0 = 0.0_degrees;
			CryTransform::CClampedAngle<-360, 360> m_rotationLimitsYZ1 = 360.0_degrees;

			Schematyc::Range<-10000, 10000> m_damping = 0.f;

			std::vector<int> m_constraintIds;
		};

	}
}