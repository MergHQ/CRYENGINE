#include "StdAfx.h"
#include "PlaneConstraint.h"
#include <CryRenderer/IRenderAuxGeom.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void CPlaneConstraintComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPlaneConstraintComponent::ConstrainToEntity, "{761D04E2-672B-4EAD-885E-B7B056001DFA}"_cry_guid, "ConstrainToEntity");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the specified entity");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'enti', "Target Entity", "Defines the entity we want to be constrained to, or the point itself if id is 0", Schematyc::ExplicitEntityId());
				pFunction->BindInput(2, 'igno', "Ignore Collisions With", "Whether or not to ignore collisions between this entity and the target", false);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPlaneConstraintComponent::ConstrainToPoint, "{2A1E5BF3-C98F-40B3-86C6-FF21CB35F4A9}"_cry_guid, "ConstrainToPoint");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the point");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPlaneConstraintComponent::Remove, "{F3E7141F-508F-479D-AD72-2BD838C09905}"_cry_guid, "Remove");
				pFunction->SetDescription("Removes the constraint");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
		}

		void CPlaneConstraintComponent::Initialize()
		{
			Reset();
		}

		void CPlaneConstraintComponent::OnShutDown()
		{
			Remove();
		}

		void CPlaneConstraintComponent::Reset()
		{
			if (m_bActive)
			{
				auto attach = m_attacher.FindAttachments(this);
				ConstrainTo(attach.first, m_attacher.noAttachColl, attach.second);
			}
			else
			{
				Remove();
			}
		}

		void CPlaneConstraintComponent::ProcessEvent(const SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_START_GAME || event.event == ENTITY_EVENT_LAYER_UNHIDE)
			{
				Reset();
			}
			else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				m_pEntity->UpdateComponentEventMask(this);

				m_axis = m_axis.GetNormalized();

				Reset();
			}
			else if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED)
			{
				m_constraintIds.clear();
			}
		}

		Cry::Entity::EventFlags CPlaneConstraintComponent::GetEventMask() const
		{
			Cry::Entity::EventFlags bitFlags = m_bActive ? (ENTITY_EVENT_START_GAME | ENTITY_EVENT_LAYER_UNHIDE | ENTITY_EVENT_PHYSICAL_TYPE_CHANGED) : Cry::Entity::EventFlags();
			bitFlags |= ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED;

			return bitFlags;
		}

#ifndef RELEASE
		void CPlaneConstraintComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
		{
			if (context.bSelected)
			{
				Quat rot = Quat::CreateRotationV0V1(Vec3(0, 0, 1), m_axis).GetInverted() * Quat(m_pEntity->GetSlotLocalTM(GetEntitySlotId(), false)).GetInverted() * m_pEntity->GetRotation().GetInverted();
				Vec3 org = GetWorldTransformMatrix().GetTranslation();
				if (gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep && m_pEntity->GetPhysicalEntity() && !m_constraintIds.empty())
				{
					pe_status_constraint sc;
					sc.id = m_constraintIds[0].second;
					if (m_pEntity->GetPhysicalEntity()->GetStatus(&sc) && (org - sc.pt[0]).len2() > sqr(0.001f))
					{
						org = sc.pt[0];
						rot = Quat::CreateRotationV0V1(Vec3(0, 0, 1), sc.n).GetInverted();
					}
				}

				Vec3 pos1 = Vec3(-1, 1, 0) * rot + org;
				Vec3 pos2 = Vec3(1, 1, 0) * rot + org;
				Vec3 pos3 = Vec3(1, -1, 0) * rot + org;
				Vec3 pos4 = Vec3(-1, -1, 0) * rot + org;

				gEnv->pAuxGeomRenderer->DrawQuad(pos4, context.debugDrawInfo.color, pos3, context.debugDrawInfo.color, pos2, context.debugDrawInfo.color, pos1, context.debugDrawInfo.color);
				gEnv->pAuxGeomRenderer->DrawQuad(pos1, context.debugDrawInfo.color, pos2, context.debugDrawInfo.color, pos3, context.debugDrawInfo.color, pos4, context.debugDrawInfo.color);
				gEnv->pAuxGeomRenderer->DrawLine(org, context.debugDrawInfo.color, org + Vec3(0, 0, 2.2f) * rot, ColorB(0,0,0));
			}
		}
#endif
	}
}