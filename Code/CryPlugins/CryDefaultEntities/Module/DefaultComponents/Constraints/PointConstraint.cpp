#include "StdAfx.h"
#include "PointConstraint.h"

namespace Cry
{
	namespace DefaultComponents
	{
		void CPointConstraintComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPointConstraintComponent::ConstrainToEntity, "{7310C27B-1B70-4274-80EE-2DBF46085DC8}"_cry_guid, "ConstrainToEntity");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the specified entity");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'enti', "Target Entity", "Defines the entity we want to be constrained to, or the point itself if id is 0", Schematyc::ExplicitEntityId());
				pFunction->BindInput(2, 'igno', "Ignore Collisions With", "Whether or not to ignore collisions between this entity and the target", false);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPointConstraintComponent::ConstrainToPoint, "{97A9A9D5-6821-4914-87FB-0990534E8409}"_cry_guid, "ConstrainToPoint");
				pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the point");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CPointConstraintComponent::Remove, "{DD75FE1D-7147-4609-974E-EE6F051ADCC1}"_cry_guid, "Remove");
				pFunction->SetDescription("Removes the constraint");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				componentScope.Register(pFunction);
			}
		}

		void CPointConstraintComponent::ReflectType(Schematyc::CTypeDesc<CPointConstraintComponent>& desc)
		{
			desc.SetGUID(CPointConstraintComponent::IID());
			desc.SetEditorCategory("Physics Constraints");
			desc.SetLabel("Point Constraint");
			desc.SetDescription("Constrains the physical object to a point");
			//desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CPointConstraintComponent::m_bActive, 'actv', "Active", "Active", "Whether or not the constraint should be added on component reset", true);
			desc.AddMember(&CPointConstraintComponent::m_axis, 'axis', "Axis", "Axis", "Axis around which the physical entity is constrained", Vec3(0.f, 0.f, 1.f));

			desc.AddMember(&CPointConstraintComponent::m_rotationLimitsX0, 'rlx0', "RotationLimitsX0", "Minimum X Angle", nullptr, 0.0_degrees);
			desc.AddMember(&CPointConstraintComponent::m_rotationLimitsX1, 'rlx1', "RotationLimitsX1", "Minimum X Angle", nullptr, 360.0_degrees);
			desc.AddMember(&CPointConstraintComponent::m_rotationLimitsYZ0, 'rly0', "RotationLimitsYZ0", "Minimum YZ Angle", nullptr, 0.0_degrees);
			desc.AddMember(&CPointConstraintComponent::m_rotationLimitsYZ1, 'rly1', "RotationLimitsYZ1", "Minimum YZ Angle", nullptr, 360.0_degrees);

			desc.AddMember(&CPointConstraintComponent::m_damping, 'damp', "Damping", "Damping", nullptr, 0.f);
		}

		CPointConstraintComponent::~CPointConstraintComponent()
		{
			Remove();
		}

		void CPointConstraintComponent::Initialize()
		{
			Reset();
		}

		void CPointConstraintComponent::Reset()
		{
			if (m_bActive)
			{
				ConstrainToPoint();
			}
			else
			{
				Remove();
			}
		}

		void CPointConstraintComponent::ProcessEvent(SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_START_GAME)
			{
				Reset();
			}
			else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				m_pEntity->UpdateComponentEventMask(this);

				Reset();
			}
		}

		uint64 CPointConstraintComponent::GetEventMask() const
		{
			uint64 bitFlags = m_bActive ? BIT64(ENTITY_EVENT_START_GAME) : 0;
			bitFlags |= BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

			return bitFlags;
		}
	}
}