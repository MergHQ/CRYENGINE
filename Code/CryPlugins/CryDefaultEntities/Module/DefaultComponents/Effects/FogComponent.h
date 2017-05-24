#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>
#include <Cry3DEngine/IRenderNode.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CFogComponent final
			: public IEntityComponent
		{
		public:
			virtual ~CFogComponent() {}

			// IEntityComponent
			virtual void Initialize() override
			{
				if (m_bActive)
				{
					SFogVolumeProperties fogProperties;
					fogProperties.m_volumeType = m_type;
					fogProperties.m_size = m_size;
					fogProperties.m_color = m_color;

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

					ColorF color(m_emission);
					color.adjust_luminance(m_emissionIntensity * toLightUnitScale);

					fogProperties.m_emission = color.toVec3();

					m_pEntity->LoadFogVolume(GetOrMakeEntitySlotId(), fogProperties);
				}
				else
				{
					FreeEntitySlot();
				}
			}

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CFogComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{5E98F483-BDF2-4497-8654-3474643AF147}"_cry_guid;
				return id;
			}

			struct SOptions
			{
				inline bool operator==(const SOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				bool m_bUseGlobalFogColor = false;
				bool m_bIgnoreVisAreas = false;
				bool m_bAffectsOnlyThisArea = false;
				Schematyc::Range<0, 10000> m_globalDensity = 1.f;
				Schematyc::Range<0, 100> m_densityOffset = 0.f;
				Schematyc::Range<0, 100> m_nearCutoff = 0.f;
				Schematyc::Range<0, 1> m_softEdges = 1.f;
				Schematyc::Range<0, 100> m_heightFallOffDirLong = 0.f;
				Schematyc::Range<0, 100> m_heightFallOffDirLati = 0.f;
				Schematyc::Range<0, 100> m_heightFallOffShift = 0.f;
				Schematyc::Range<0, 100> m_heightFallOffScale = 1.f;
				Schematyc::Range<0, 30000> m_rampStart = 0.f;
				Schematyc::Range<0, 30000> m_rampEnd = 50.f;
				Schematyc::Range<0, 1> m_rampInfluence = 0.f;
				Schematyc::Range<0, 20> m_windInfluence = 1.f;
				Schematyc::Range<0, 20> m_densityNoiseScale = 0.f;
				Schematyc::Range<-20, 20> m_densityNoiseOffset = 0.f;
				Schematyc::Range<0, 10> m_densityNoiseTimeFrequency = 0.f;
				Vec3 m_densityNoiseFrequency = Vec3(1.f);
			};

			void SetActive(bool bActive) { m_bActive = bActive; }
			bool IsActive() const { return m_bActive; }

			void SetType(IFogVolumeRenderNode::eFogVolumeType type) { m_type = type; }
			IFogVolumeRenderNode::eFogVolumeType GetType() const { return m_type; }

			void SetSize(const Vec3& size) { m_size = size; }
			const Vec3& GetSize() const { return m_size; }

			void SetColor(const ColorF& color) { m_color = color; }
			const ColorF& GetColor() const { return m_color; }

			void SetEmissionColor(const ColorF& color) { m_emission = color; }
			const ColorF& GetEmissionColor() const { return m_emission; }

			void SetEmissionIntensity(float intensity) { m_emissionIntensity = intensity; }
			float GetEmissionIntensity() const { return m_emissionIntensity; }

			SOptions& GetOptions() { return m_options; }
			const SOptions& GetOptions() const { return m_options; }

		protected:
			bool m_bActive = true;
			IFogVolumeRenderNode::eFogVolumeType m_type = IFogVolumeRenderNode::eFogVolumeType_Ellipsoid;
			Vec3 m_size = Vec3(1.f);
			ColorF m_color = ColorF(1.f);
			ColorF m_emission = ColorF(0.f);
			Schematyc::Range<0, 10000> m_emissionIntensity = 1.f;

			SOptions m_options;
		};
	}
}