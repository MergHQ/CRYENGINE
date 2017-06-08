#include "StdAfx.h"
#include "SpherePrimitiveComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CSpherePrimitiveComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

void CSpherePrimitiveComponent::ReflectType(Schematyc::CTypeDesc<CSpherePrimitiveComponent>& desc)
{
	desc.SetGUID(CSpherePrimitiveComponent::IID());
	desc.SetEditorCategory("Physics");
	desc.SetLabel("Sphere Collider");
	desc.SetDescription("Adds a sphere primitive to the physical entity, requires a Simple Physics or Character Controller component.");
	desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CSpherePrimitiveComponent::m_radius, 'radi', "Radius", "Radius", "Radius of the sphere", 1.f);

	desc.AddMember(&CSpherePrimitiveComponent::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
	desc.AddMember(&CSpherePrimitiveComponent::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);

	desc.AddMember(&CSpherePrimitiveComponent::m_surfaceTypeName, 'surf', "Surface", "Surface Type", "Surface type assigned to this object, determines its physical properties", "");
}
}
}
