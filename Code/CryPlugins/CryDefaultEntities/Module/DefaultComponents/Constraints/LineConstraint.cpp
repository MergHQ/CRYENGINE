#include "StdAfx.h"
#include "LineConstraint.h"
#include <CryRenderer/IRenderAuxGeom.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void CLineConstraintComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CLineConstraintComponent::ConstrainToEntity, "{6768FAAD-9218-4B05-8AF4-ED57406C6DFE}"_cry_guid, "ConstrainToEntity");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the specified entity");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'enti', "Target Entity", "Defines the entity we want to be constrained to, or the point itself if id is 0", Schematyc::ExplicitEntityId());
				pFunction->BindInput(2, 'igno', "Ignore Collisions With", "Whether or not to ignore collisions between this entity and the target", false);
				pFunction->BindInput(3, 'arot', "Allow Rotation", "Whether or not to allow rotations when the constraint is active", true);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CLineConstraintComponent::ConstrainToPoint, "{233F472B-0836-468B-9A49-76F84A264833}"_cry_guid, "ConstrainToPoint");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the point");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'arot', "Allow Rotation", "Whether or not to allow rotations when the constraint is active", true);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CLineConstraintComponent::Remove, "{55369F15-600D-46E1-9446-93BBC456FF11}"_cry_guid, "Remove");
				pFunction->SetDescription("Removes the constraint");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
		}

		void CLineConstraintComponent::Initialize()
		{
			Reset();
		}

		void CLineConstraintComponent::OnShutDown()
		{
			Remove();
		}

		void CLineConstraintComponent::Reset()
		{
			if (m_bActive)
			{
				auto attach = m_attacher.FindAttachments(this);
				ConstrainTo(attach.first, m_attacher.noAttachColl, !m_bLockRotation, attach.second);
			}
			else
			{
				Remove();
			}
		}

		void CLineConstraintComponent::ProcessEvent(const SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_START_GAME || event.event == ENTITY_EVENT_LAYER_UNHIDE)
			{
				Reset();
			}
			else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				m_axis = m_axis.Normalize();

				m_pEntity->UpdateComponentEventMask(this);

				Reset();
			}
			else if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED)
			{
				m_constraintIds.clear();
			}
		}

		Cry::Entity::EventFlags CLineConstraintComponent::GetEventMask() const
		{
			Cry::Entity::EventFlags bitFlags = m_bActive ? (ENTITY_EVENT_START_GAME | ENTITY_EVENT_LAYER_UNHIDE | ENTITY_EVENT_PHYSICAL_TYPE_CHANGED) : Cry::Entity::EventFlags();
			bitFlags |= ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED;

			return bitFlags;
		}

#ifndef RELEASE
		void CLineConstraintComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
		{
			if (context.bSelected)
			{
				Vec3 axis = m_axis * m_pEntity->GetRotation().GetInverted();

				Vec3 pos1 = m_pEntity->GetSlotWorldTM(GetEntitySlotId()).GetTranslation(), pos2 = pos1;
				if (gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep && m_pEntity->GetPhysicalEntity() && !m_constraintIds.empty())
				{
					pe_status_constraint sc;
					sc.id = m_constraintIds[0].second;
					if (m_pEntity->GetPhysicalEntity()->GetStatus(&sc) && (pos1 - sc.pt[0]).len2() > sqr(0.001f))
					{
						pos1 = pos2 = sc.pt[0];
						axis = sc.n;
					}
				}
				pos1.x += m_limitMin * axis.x;
				pos1.y += m_limitMin * axis.y;
				pos1.z += m_limitMin * axis.z;

				pos2.x += m_limitMax * axis.x;
				pos2.y += m_limitMax * axis.y;
				pos2.z += m_limitMax * axis.z;

				gEnv->pAuxGeomRenderer->DrawLine(pos1, context.debugDrawInfo.color, pos2, context.debugDrawInfo.color, 10.0f);
			}
		}
#endif
	}
}