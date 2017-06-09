#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>
#include <Cry3DEngine/IRenderNode.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CFogComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			virtual ~CFogComponent() {}

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

			virtual void SetActive(bool bActive) { m_bActive = bActive; }
			bool IsActive() const { return m_bActive; }

			virtual void SetType(IFogVolumeRenderNode::eFogVolumeType type) { m_type = type; }
			IFogVolumeRenderNode::eFogVolumeType GetType() const { return m_type; }

			virtual void SetSize(const Vec3& size) { m_size = size; }
			const Vec3& GetSize() const { return m_size; }

			virtual void SetColor(const ColorF& color) { m_color = color; }
			const ColorF& GetColor() const { return m_color; }

			virtual SOptions& GetOptions() { return m_options; }
			const SOptions& GetOptions() const { return m_options; }

		protected:
			bool m_bActive = true;
			IFogVolumeRenderNode::eFogVolumeType m_type = IFogVolumeRenderNode::eFogVolumeType_Ellipsoid;
			Vec3 m_size = Vec3(1.f);
			ColorF m_color = ColorF(1.f);

			SOptions m_options;
		};
	}
}