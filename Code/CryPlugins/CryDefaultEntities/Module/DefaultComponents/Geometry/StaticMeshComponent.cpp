#include "StdAfx.h"
#include "StaticMeshComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CStaticMeshComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

void CStaticMeshComponent::ReflectType(Schematyc::CTypeDesc<CStaticMeshComponent>& desc)
{
	desc.SetGUID(CStaticMeshComponent::IID());
	desc.SetEditorCategory("Geometry");
	desc.SetLabel("Static Mesh");
	desc.SetDescription("A component containing a simple mesh that can not be animated");
	desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CStaticMeshComponent::m_type, 'type', "Type", "Type", "Determines the behavior of the static mesh", CStaticMeshComponent::EType::RenderAndCollider);

	desc.AddMember(&CStaticMeshComponent::m_filePath, 'file', "FilePath", "File", "Determines the CGF to load", "%ENGINE%/EngineAssets/Objects/Default.cgf");
	desc.AddMember(&CStaticMeshComponent::m_physics, 'phys', "Physics", "Physics", "Physical properties for the object, only used if a simple physics or character controller is applied to the entity.", CStaticMeshComponent::SPhysics());
}

static void ReflectType(Schematyc::CTypeDesc<CStaticMeshComponent::SPhysics>& desc)
{
	desc.SetGUID("{6CE17866-A74D-437D-98DB-F72E7AD7234E}"_cry_guid);
	desc.AddMember(&CStaticMeshComponent::SPhysics::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
	desc.AddMember(&CStaticMeshComponent::SPhysics::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);
}

static void ReflectType(Schematyc::CTypeDesc<CStaticMeshComponent::EType>& desc)
{
	desc.SetGUID("{A786ABAF-3B6F-4EB4-A073-502A87EC5F64}"_cry_guid);
	desc.SetLabel("Mesh Type");
	desc.SetDescription("Determines what to use a mesh for");
	desc.SetDefaultValue(CStaticMeshComponent::EType::RenderAndCollider);
	desc.AddConstant(CStaticMeshComponent::EType::None, "None", "Disabled");
	desc.AddConstant(CStaticMeshComponent::EType::Render, "Render", "Render");
	desc.AddConstant(CStaticMeshComponent::EType::Collider, "Collider", "Collider");
	desc.AddConstant(CStaticMeshComponent::EType::RenderAndCollider, "RenderAndCollider", "Render and Collider");
}

void CStaticMeshComponent::Initialize()
{
	LoadFromDisk();
	ResetObject();
}

void CStaticMeshComponent::LoadFromDisk()
{
	if (m_filePath.value.size() > 0)
	{
		m_pCachedStatObj = gEnv->p3DEngine->LoadStatObj(m_filePath.value);
	}
}

void CStaticMeshComponent::SetObject(IStatObj* pObject, bool bSetDefaultMass)
{
	m_pCachedStatObj = pObject;
	m_filePath.value = m_pCachedStatObj->GetFilePath();

	if (bSetDefaultMass)
	{
		if (!m_pCachedStatObj->GetPhysicalProperties(m_physics.m_mass, m_physics.m_density))
		{
			m_physics.m_mass = 10;
		}
	}
}

void CStaticMeshComponent::ResetObject()
{
	if (m_pCachedStatObj != nullptr)
	{
		m_pEntity->SetStatObj(m_pCachedStatObj, GetOrMakeEntitySlotId(), false);

		if (((uint32)m_type & (uint32)EType::Render) != 0)
		{
			m_pEntity->SetSlotFlags(GetEntitySlotId(), ENTITY_SLOT_RENDER);
		}
		else
		{
			m_pEntity->SetSlotFlags(GetEntitySlotId(), 0);
		}

		if (GetEntitySlotId() != EmptySlotId)
		{
			m_pEntity->UnphysicalizeSlot(GetEntitySlotId());
		}

		if ((m_physics.m_mass > 0 || m_physics.m_density > 0) && ((uint32)m_type & (uint32)EType::Collider) != 0)
		{
			if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
			{
				SEntityPhysicalizeParams physParams;

				if (m_physics.m_mass > 0)
				{
					physParams.mass = m_physics.m_mass;
				}
				else
				{
					physParams.mass = -1.f;
				}

				if (m_physics.m_density > 0)
				{
					physParams.density = m_physics.m_density;
				}
				else
				{
					physParams.density = -1.f;
				}

				m_pEntity->PhysicalizeSlot(GetEntitySlotId(), physParams);
			}
		}
	}
	else
	{
		FreeEntitySlot();
	}
}

void CStaticMeshComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED)
	{
		if (GetEntitySlotId() != EmptySlotId)
		{
			m_pEntity->UnphysicalizeSlot(GetEntitySlotId());
		}

		if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
		{
			SEntityPhysicalizeParams physParams;
			if (m_physics.m_mass > 0)
			{
				physParams.mass = m_physics.m_mass;
			}
			else
			{
				physParams.mass = -1.f;
			}

			if (m_physics.m_density > 0)
			{
				physParams.density = m_physics.m_density;
			}
			else
			{
				physParams.density = -1.f;
			}

			// Mark as object with infinite mass
			if (isneg(physParams.mass) && isneg(physParams.density))
			{
				physParams.mass = 0;
			}

			m_pEntity->PhysicalizeSlot(GetEntitySlotId(), physParams);
		}
	}
	else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
#ifndef RELEASE
		// Reset mass or density to 0 in the UI if the other is changed to be positive.
		// It is not possible to use both at the same time, this makes that clearer for the designer.
		if (m_physics.m_mass != m_physics.m_prevMass && m_physics.m_mass >= 0)
		{
			m_physics.m_density = m_physics.m_prevDensity = 0;
			m_physics.m_prevMass = m_physics.m_mass;
		}
		if (m_physics.m_density != m_physics.m_prevDensity && m_physics.m_density >= 0)
		{
			m_physics.m_mass = m_physics.m_prevMass = 0;
			m_physics.m_prevDensity = m_physics.m_density;
		}
#endif

		m_pEntity->UpdateComponentEventMask(this);

		LoadFromDisk();
		ResetObject();
	}
}

uint64 CStaticMeshComponent::GetEventMask() const
{
	uint64 bitFlags = BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

	if (((uint32)m_type & (uint32)EType::Collider) != 0)
	{
		bitFlags |= BIT64(ENTITY_EVENT_PHYSICAL_TYPE_CHANGED);
	}

	return bitFlags;
}

void CStaticMeshComponent::SetFilePath(const char* szPath)
{
	m_filePath.value = szPath;
}
}} // namespace Cry::DefaultComponents
