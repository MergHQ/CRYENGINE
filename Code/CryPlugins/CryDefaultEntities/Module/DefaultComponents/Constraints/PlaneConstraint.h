#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CPlaneConstraintComponent
			: public IEntityComponent
		{
			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			virtual ~CPlaneConstraintComponent();

			static void ReflectType(Schematyc::CTypeDesc<CPlaneConstraintComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{F9C3A204-809B-4DA8-965E-CA9417FEBD95}"_cry_guid;
				return id;
			}

			virtual void ConstrainToEntity(Schematyc::ExplicitEntityId targetEntityId, bool bDisableCollisionsWith, bool bAllowRotation)
			{
				if ((EntityId)targetEntityId != INVALID_ENTITYID)
				{
					if (IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity((EntityId)targetEntityId))
					{
						if (IPhysicalEntity* pPhysicalEntity = pTargetEntity->GetPhysicalEntity())
						{
							ConstrainTo(pPhysicalEntity, bDisableCollisionsWith, bAllowRotation);
						}
					}
				}
			}

			virtual void ConstrainToPoint(bool bAllowRotation)
			{
				ConstrainTo(WORLD_ENTITY, false, bAllowRotation);
			}

			virtual void ConstrainTo(IPhysicalEntity* pEntity, bool bDisableCollisionsWith = false, bool bAllowRotation = true)
			{
				Remove();

				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					Matrix34 slotTransform = GetWorldTransformMatrix();

					pe_action_add_constraint constraint;
					constraint.flags = world_frames | constraint_no_tears | constraint_plane;
					constraint.pt[0] = constraint.pt[1] = slotTransform.GetTranslation();
					constraint.qframe[0] = constraint.qframe[1] = Quat(slotTransform) * Quat::CreateRotationV0V1(Vec3(1, 0, 0), m_axis);
					constraint.xlimits[0] = m_limitMin;
					constraint.xlimits[1] = m_limitMax;
					constraint.yzlimits[0] = m_limitMinY;
					constraint.yzlimits[1] = m_limitMaxY;
					constraint.damping = m_damping;

					if (!bAllowRotation)
					{
						constraint.flags |= constraint_no_rotation;
					}

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

			virtual void SetLimitsX(float minLimit, float maxLimit) { m_limitMin = minLimit; m_limitMax = maxLimit; }
			float GetMinimumLimitX() const { return m_limitMin; }
			float GetMaximumLimitX() const { return m_limitMax; }

			virtual void SetLimitsY(float minLimit, float maxLimit) { m_limitMinY = minLimit; m_limitMaxY = maxLimit; }
			float GetMinimumLimitY() const { return m_limitMinY; }
			float GetMaximumLimitY() const { return m_limitMaxY; }

			virtual void SetDamping(float damping) { m_damping = damping; }
			float GetDamping() const { return m_damping; }

		protected:
			void Reset();

		protected:
			bool m_bActive = true;
			Vec3 m_axis = Vec3(0, 0, 1);

			Schematyc::Range<-10000, 10000> m_limitMin = 0.f;
			Schematyc::Range<-10000, 10000> m_limitMax = 1.f;
			Schematyc::Range<-10000, 10000> m_limitMinY = 0.f;
			Schematyc::Range<-10000, 10000> m_limitMaxY = 1.f;

			Schematyc::Range<-10000, 10000> m_damping = 0.f;

			std::vector<int> m_constraintIds;
		};

	}
}