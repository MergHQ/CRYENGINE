// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "BaseMeshComponent.h"
#include <CrySerialization/Enum.h>

namespace Cry
{
namespace DefaultComponents
{
	YASLI_ENUM_BEGIN_NESTED(SPhysicsParameters, EWeightType, "WeightType")
	YASLI_ENUM_VALUE_NESTED(SPhysicsParameters, EWeightType::Mass, "Mass")
	YASLI_ENUM_VALUE_NESTED(SPhysicsParameters, EWeightType::Density, "Density")
	YASLI_ENUM_VALUE_NESTED(SPhysicsParameters, EWeightType::Immovable, "Immovable")
	YASLI_ENUM_END()

void CBaseMeshComponent::ProcessEvent(const SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED || event.event == ENTITY_EVENT_SLOT_CHANGED)
	{
		ApplyBaseMeshProperties();
	}
	else
	{
		if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
		{
			switch (m_physics.m_weightType)
			{
			case SPhysicsParameters::EWeightType::Mass:
				{
					m_physics.m_mass = max((float)m_physics.m_mass, SPhysicsParameters::s_weightTypeEpsilon);
				}
				break;
			case SPhysicsParameters::EWeightType::Density:
				{
					m_physics.m_density = max((float)m_physics.m_density, SPhysicsParameters::s_weightTypeEpsilon);
				}
				break;
			}

			m_pEntity->UpdateComponentEventMask(this);

			ApplyBaseMeshProperties();
		}
	}
}

void CBaseMeshComponent::ApplyBaseMeshProperties()
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

		if (IRenderNode* pRenderNode = m_pEntity->GetSlotRenderNode(GetEntitySlotId()))
		{
			uint32 slotFlags = m_pEntity->GetSlotFlags(GetEntitySlotId());
			if (m_renderParameters.m_castShadowSpec != EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_renderParameters.m_castShadowSpec)
			{
				slotFlags |= ENTITY_SLOT_CAST_SHADOW;
			}
			if (m_renderParameters.m_bIgnoreVisAreas)
			{
				slotFlags |= ENTITY_SLOT_IGNORE_VISAREAS;
			}

			UpdateGIModeEntitySlotFlags((uint8)m_renderParameters.m_giMode, slotFlags);

			m_pEntity->SetSlotFlags(GetEntitySlotId(), slotFlags);

			// We want to manage our own view distance ratio
			m_pEntity->AddFlags(ENTITY_FLAG_CUSTOM_VIEWDIST_RATIO);
			pRenderNode->SetViewDistRatio((int)floor(((float)m_renderParameters.m_viewDistanceRatio / 100.f) * 255));
			pRenderNode->SetLodRatio((int)floor(((float)m_renderParameters.m_lodDistance / 100.f) * 255));
		}

		m_pEntity->UnphysicalizeSlot(GetEntitySlotId());

		if (((uint32)m_type & (uint32)EMeshType::Collider) != 0)
		{
			if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
			{
				SEntityPhysicalizeParams physParams;
				physParams.nSlot = GetEntitySlotId();
				physParams.type = pPhysicalEntity->GetType();

				switch (m_physics.m_weightType)
				{
				case SPhysicsParameters::EWeightType::Mass:
					{
						physParams.mass = m_physics.m_mass;
						physParams.density = -1.0f;
					}
					break;
				case SPhysicsParameters::EWeightType::Density:
					{
						physParams.density = m_physics.m_density;
						physParams.mass = -1.0f;
					}
					break;
				case SPhysicsParameters::EWeightType::Immovable:
					{
						physParams.density = -1.0f;
						physParams.mass = 0.0f;
					}
					break;
				}

				m_pEntity->PhysicalizeSlot(GetEntitySlotId(), physParams);
			}
		}
	}
}
}
}
