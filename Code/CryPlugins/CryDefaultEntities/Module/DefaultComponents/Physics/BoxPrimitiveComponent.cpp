#include "StdAfx.h"
#include "BoxPrimitiveComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		static void RegisterBoxPrimitiveComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CBoxPrimitiveComponent));
				// Functions
			}
		}

		void ReflectType(Schematyc::CTypeDesc<CBoxPrimitiveComponent>& desc)
		{
			desc.SetGUID(CBoxPrimitiveComponent::IID());
			desc.SetEditorCategory("Physics");
			desc.SetLabel("Box Collider");
			desc.SetDescription("Adds a box primitive to the physical entity, requires a Simple Physics or Character Controller component.");
			desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CBoxPrimitiveComponent::m_size, 'size', "Size", "Size", "Size of the box", Vec3(1, 1, 1));

			desc.AddMember(&CBoxPrimitiveComponent::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
			desc.AddMember(&CBoxPrimitiveComponent::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);

			desc.AddMember(&CBoxPrimitiveComponent::m_surfaceTypeName, 'surf', "Surface", "Surface Type", "Surface type assigned to this object, determines its physical properties");
		}

		void CBoxPrimitiveComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			m_pEntity->UpdateComponentEventMask(this);

			Initialize();
		}
	}
}