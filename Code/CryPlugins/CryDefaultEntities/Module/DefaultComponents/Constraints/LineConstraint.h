#pragma once

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CLineConstraintComponent final
			: public IEntityComponent
		{
		public:
			virtual ~CLineConstraintComponent();

			// IEntityComponent
			virtual void Run(Schematyc::ESimulationMode simulationMode) override;

			virtual void ProcessEvent(SEntityEvent& event) override;
			virtual uint64 GetEventMask() const override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CLineConstraintComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{D25359AC-E566-47F9-B056-AC52C4361348}"_cry_guid;
				return id;
			}

			void ConstrainToEntity(Schematyc::ExplicitEntityId targetEntityId, bool bDisableCollisionsWith, bool bAllowRotation)
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

			void ConstrainToPoint(bool bAllowRotation)
			{
				ConstrainTo(WORLD_ENTITY, false, bAllowRotation);
			}

			void ConstrainTo(IPhysicalEntity* pEntity, bool bDisableCollisionsWith = false, bool bAllowRotation = true)
			{
				Remove();

				// Force create a dummy entity slot to allow designer transformation change
				SEntitySlotInfo slotInfo;
				if (!m_pEntity->GetSlotInfo(GetOrMakeEntitySlotId(), slotInfo))
				{
					m_pEntity->SetSlotRenderNode(GetOrMakeEntitySlotId(), nullptr);
				}

				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					const Matrix34& slotTransform = m_pEntity->GetSlotWorldTM(GetEntitySlotId());

					pe_action_add_constraint constraint;
					constraint.flags = world_frames | constraint_no_tears | constraint_line;
					constraint.pt[0] = constraint.pt[1] = slotTransform.GetTranslation();
					constraint.qframe[0] = constraint.qframe[1] = Quat(slotTransform) * Quat::CreateRotationV0V1(Vec3(1, 0, 0), m_axis);
					constraint.xlimits[0] = m_limitMin;
					constraint.xlimits[1] = m_limitMax;
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

			void Remove()
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

			void Activate(bool bActivate)
			{
				m_bActive = bActivate;

				m_pEntity->UpdateComponentEventMask(this);
			}
			bool IsActive() const { return m_bActive; }

			void SetAxis(Vec3& axis) { m_axis = axis; }
			const Vec3& GetAxis() const { return m_axis; }

			void SetLimits(float minLimit, float maxLimit) { m_limitMin = minLimit; m_limitMax = maxLimit; }
			float GetMinimumLimit() const { return m_limitMin; }
			float GetMaximumLimit() const { return m_limitMax; }

			void SetDamping(float damping) { m_damping = damping; }
			float GetDamping() const { return m_damping; }

		protected:
			bool m_bActive = true;
			Vec3 m_axis = Vec3(0, 0, 1);

			Schematyc::Range<-10000, 10000> m_limitMin = 0.f;
			Schematyc::Range<-10000, 10000> m_limitMax = 1.f;

			Schematyc::Range<-10000, 10000> m_damping = 0.f;

			std::vector<int> m_constraintIds;
		};
	}
}