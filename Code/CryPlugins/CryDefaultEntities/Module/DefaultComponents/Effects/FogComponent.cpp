#include "StdAfx.h"
#include "FogComponent.h" 

namespace Cry
{
	namespace DefaultComponents
	{
		void CFogComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
		}

		void CFogComponent::Initialize()
		{
			if (m_bActive)
			{
				SFogVolumeProperties fogProperties;
				fogProperties.m_volumeType = m_type;
				fogProperties.m_size = m_size;
				fogProperties.m_color = m_color;

				fogProperties.m_useGlobalFogColor = m_options.m_bUseGlobalFogColor;
				fogProperties.m_fHDRDynamic = 0.f;

				fogProperties.m_globalDensity = m_options.m_globalDensity;
				fogProperties.m_densityOffset = m_options.m_densityOffset;
				fogProperties.m_nearCutoff = m_options.m_nearCutoff;
				fogProperties.m_softEdges = m_options.m_softEdges;
				fogProperties.m_heightFallOffDirLong = m_options.m_heightFallOffDirLong;
				fogProperties.m_heightFallOffDirLati = m_options.m_heightFallOffDirLati;
				fogProperties.m_heightFallOffShift = m_options.m_heightFallOffShift;
				fogProperties.m_heightFallOffScale = m_options.m_heightFallOffScale;
				fogProperties.m_rampStart = m_options.m_rampStart;
				fogProperties.m_rampEnd = m_options.m_rampEnd;
				fogProperties.m_rampInfluence = m_options.m_rampInfluence;
				fogProperties.m_windInfluence = m_options.m_windInfluence;
				fogProperties.m_densityNoiseScale = m_options.m_densityNoiseScale;
				fogProperties.m_densityNoiseOffset = m_options.m_densityNoiseOffset;
				fogProperties.m_densityNoiseTimeFrequency = m_options.m_densityNoiseTimeFrequency;
				fogProperties.m_densityNoiseFrequency = m_options.m_densityNoiseFrequency;

				const float kiloScale = 1000.0f;
				const float toLightUnitScale = kiloScale / RENDERER_LIGHT_UNIT_SCALE;

				m_pEntity->LoadFogVolume(GetOrMakeEntitySlotId(), fogProperties);
			}
			else
			{
				FreeEntitySlot();
			}
		}

		void CFogComponent::ProcessEvent(SEntityEvent& event)
		{
			switch (event.event)
			{
			case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
			{
				Initialize();
			}
			break;
			}
		}

		uint64 CFogComponent::GetEventMask() const
		{
			return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
		}
	}
}