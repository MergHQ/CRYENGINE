#include "StdAfx.h"
#include "WheelComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void CWheelComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
		}

		void ReflectType(Schematyc::CTypeDesc<CWheelComponent>& desc)
		{
			desc.SetGUID(CWheelComponent::IID());
			desc.SetEditorCategory("Physics");
			desc.SetLabel("Wheel Collider");
			desc.SetDescription("Adds a wheel collider to the vehicle.");
			desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
			desc.AddComponentInteraction(SEntityComponentRequirements::EType::HardDependency, cryiidof<CVehiclePhysicsComponent>());

			desc.AddMember(&CWheelComponent::m_radius, 'radi', "Radius", "Radius", "Radius of the cylinder", 0.5f);
			desc.AddMember(&CWheelComponent::m_height, 'heig', "Height", "Height", "Height of the cylinder", 1.f);
			
			desc.AddMember(&CWheelComponent::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
			desc.AddMember(&CWheelComponent::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);

			desc.AddMember(&CWheelComponent::m_surfaceTypeName, 'surf', "Surface", "Surface Type", "Surface type assigned to this object, determines its physical properties", "");
			desc.AddMember(&CWheelComponent::m_axleIndex, 'axle', "AxleIndex", "Axle Index", nullptr, 0);
			desc.AddMember(&CWheelComponent::m_suspensionLength, 'susp', "SuspensionLength", "Suspension Length", nullptr, 0);
			desc.AddMember(&CWheelComponent::m_bRaycast, 'rayc', "RayCast", "Ray Cast", "Whether to cast a ray downwards to find collisions, or to check for mesh intersections", false);
		}
	}
}