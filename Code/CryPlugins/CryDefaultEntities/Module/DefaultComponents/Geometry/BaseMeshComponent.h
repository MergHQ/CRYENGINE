#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

namespace Cry
{
namespace DefaultComponents
{
enum class EMeshType : uint32
{
	None = 0,
	Render = 1 << 0,
	Collider = 1 << 1,
	RenderAndCollider = Render | Collider,
};

static void ReflectType(Schematyc::CTypeDesc<EMeshType>& desc)
{
	desc.SetGUID("{A786ABAF-3B6F-4EB4-A073-502A87EC5F64}"_cry_guid);
	desc.SetLabel("Mesh Type");
	desc.SetDescription("Determines what to use a mesh for");
	desc.SetDefaultValue(EMeshType::RenderAndCollider);
	desc.AddConstant(EMeshType::None, "None", "Disabled");
	desc.AddConstant(EMeshType::Render, "Render", "Render");
	desc.AddConstant(EMeshType::Collider, "Collider", "Collider");
	desc.AddConstant(EMeshType::RenderAndCollider, "RenderAndCollider", "Render and Collider");
}

struct SPhysicsParameters
{
	inline bool operator==(const SPhysicsParameters& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

	Schematyc::Range<0, 100000> m_mass = 10.f;
	Schematyc::Range<0, 100000> m_density = 0.f;

	// Used to reset mass or density to -1 in the UI
#ifndef RELEASE
	float m_prevMass = 0.f;
	float m_prevDensity = 0.f;
#endif
};

static void ReflectType(Schematyc::CTypeDesc<SPhysicsParameters>& desc)
{
	desc.SetGUID("{6CE17866-A74D-437D-98DB-F72E7AD7234E}"_cry_guid);
	desc.AddMember(&SPhysicsParameters::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
	desc.AddMember(&SPhysicsParameters::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);
}

// Base implementation for our physics mesh components
class CBaseMeshComponent
	: public IEntityComponent
{
protected:
	// IEntityComponent
	virtual void ProcessEvent(SEntityEvent& event) override
	{
		if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED)
		{
			Physicalize();
		}
		else
		{
			if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
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

				Physicalize();
			}
		}
	}

	virtual uint64 GetEventMask() const override
	{
		uint64 bitFlags = BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

		if (((uint32)m_type & (uint32)EMeshType::Collider) != 0)
		{
			bitFlags |= BIT64(ENTITY_EVENT_PHYSICAL_TYPE_CHANGED);
		}

		return bitFlags;
	}
	// ~IEntityComponent

	void Physicalize()
	{
		if (GetEntitySlotId() != EmptySlotId)
		{
			if (((uint32)m_type & (uint32)EMeshType::Render) != 0)
			{
				m_pEntity->SetSlotFlags(GetEntitySlotId(), ENTITY_SLOT_RENDER);
			}
			else
			{
				m_pEntity->SetSlotFlags(GetEntitySlotId(), 0);
			}

			m_pEntity->UnphysicalizeSlot(GetEntitySlotId());

			if ((m_physics.m_mass > 0 || m_physics.m_density > 0) && ((uint32)m_type & (uint32)EMeshType::Collider) != 0)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					SEntityPhysicalizeParams physParams;
					physParams.nSlot = GetEntitySlotId();
					physParams.type = pPhysicalEntity->GetType();

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
	}

public:
	virtual void        SetType(EMeshType type) { m_type = type; }
	EMeshType           GetType() const { return m_type; }

	virtual SPhysicsParameters&       GetPhysicsParameters()       { return m_physics; }
	const SPhysicsParameters&         GetPhysicsParameters() const { return m_physics; }

protected:
	SPhysicsParameters      m_physics;
	EMeshType               m_type = EMeshType::RenderAndCollider;
};
}
}
