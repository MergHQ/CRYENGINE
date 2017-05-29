#include "StdAfx.h"
#include "LineConstraint.h"

namespace Cry
{
	namespace DefaultComponents
	{
		void RegisterLineConstraintComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CLineConstraintComponent));
				// Functions
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CLineConstraintComponent::ConstrainToEntity, "{6768FAAD-9218-4B05-8AF4-ED57406C6DFE}"_cry_guid, "ConstrainToEntity");
					pFunction->SetDescription("Adds a constraint, tying this component's physical entity to the specified entity");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'enti', "Target Entity", "Defines the entity we want to be constrained to, or the point itself if id is 0", INVALID_ENTITYID);
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
		}

		CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterLineConstraintComponent)

		void CLineConstraintComponent::ReflectType(Schematyc::CTypeDesc<CLineConstraintComponent>& desc)
		{
			desc.SetGUID(CLineConstraintComponent::IID());
			desc.SetEditorCategory("Physics Constraints");
			desc.SetLabel("Line Constraint");
			desc.SetDescription("Constrains the physical object to a line");
			//desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CLineConstraintComponent::m_bActive, 'actv', "Active", "Active", "Whether or not the constraint should be added on component reset", true);
			desc.AddMember(&CLineConstraintComponent::m_axis, 'axis', "Axis", "Axis", "Axis around which the physical entity is constrained", Vec3(0.f, 1.f, 1.f));

			desc.AddMember(&CLineConstraintComponent::m_limitMin, 'lmin', "LimitMin", "Minimum Limit", nullptr, 0.f);
			desc.AddMember(&CLineConstraintComponent::m_limitMax, 'lmax', "LimitMax", "Maximum Limit", nullptr, 1.f);
			desc.AddMember(&CLineConstraintComponent::m_damping, 'damp', "Damping", "Damping", nullptr, 0.f);
		}

		CLineConstraintComponent::~CLineConstraintComponent()
		{
			Remove();
		}

		void CLineConstraintComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			m_pEntity->UpdateComponentEventMask(this);

			if (m_bActive)
			{
				ConstrainToPoint(true);
			}
			else
			{
				Remove();
			}
		}

		void CLineConstraintComponent::ProcessEvent(SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_START_GAME)
			{
				if (m_bActive)
				{
					ConstrainToPoint(true);
				}
				else
				{
					Remove();
				}
			}
		}

		uint64 CLineConstraintComponent::GetEventMask() const
		{
			return m_bActive ? BIT64(ENTITY_EVENT_START_GAME) : 0;
		}
	}
}