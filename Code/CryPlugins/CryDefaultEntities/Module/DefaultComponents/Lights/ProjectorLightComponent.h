#pragma once

#include "PointLightComponent.h"

#include <CrySchematyc/MathTypes.h>
#include <CryMath/Angle.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CProjectorLightComponent
			: public IEntityComponent
		{
			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			CProjectorLightComponent() {}
			virtual ~CProjectorLightComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{07D0CAD1-8E79-4177-9ADD-A2464A009FA5}"_cry_guid;
				return id;
			}

			struct SProjectorOptions
			{
				inline bool operator==(const SProjectorOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				void SetTexturePath(const char* szPath);
				const char* GetTexturePath() const { return m_texturePath.value.c_str(); }
				void SetMaterialPath(const char* szPath);
				const char* GetMaterialPath() const { return m_materialPath.value.c_str(); }
				bool HasMaterialPath() const { return m_materialPath.value.size() > 0; }

				Schematyc::Range<0, 10000> m_nearPlane = 0.f;

				Schematyc::TextureFileName m_texturePath;
				Schematyc::MaterialFileName m_materialPath;
			};

			struct SFlare
			{
				inline bool operator==(const SFlare &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				void SetTexturePath(const char* szPath);
				const char* GetTexturePath() const { return m_texturePath.value.c_str(); }
				bool HasTexturePath() const { return m_texturePath.value.size() > 0; }

				CryTransform::CClampedAngle<5, 360> m_angle = 360.0_degrees;

				Schematyc::TextureFileName m_texturePath;
			};

			virtual void Enable(bool bEnable) { m_bActive = bEnable; }
			bool IsEnabled() const { return m_bActive; }

			virtual void SetRadius(float radius) { m_radius = radius; }
			float GetRadius() const { return m_radius; }

			virtual void SetFieldOfView(CryTransform::CAngle angle) { m_angle = angle; }
			CryTransform::CAngle GetFieldOfView() { return m_angle; }

			virtual void SetNearPlane(float nearPlane) { m_projectorOptions.m_nearPlane = nearPlane; }
			float GetNearPlane() const { return m_projectorOptions.m_nearPlane; }

			virtual CPointLightComponent::SOptions& GetOptions() { return m_options; }
			const CPointLightComponent::SOptions& GetOptions() const { return m_options; }

			virtual CPointLightComponent::SColor& GetColorParameters() { return m_color; }
			const CPointLightComponent::SColor& GetColorParameters() const { return m_color; }

			virtual CPointLightComponent::SShadows& GetShadowParameters() { return m_shadows; }
			const CPointLightComponent::SShadows& GetShadowParameters() const { return m_shadows; }

			virtual CPointLightComponent::SAnimations& GetAnimationParameters() { return m_animations; }
			const CPointLightComponent::SAnimations& GetAnimationParameters() const { return m_animations; }

			virtual void SetFlareAngle(CryTransform::CAngle angle) { m_flare.m_angle = angle; }
			CryTransform:: CAngle GetFlareAngle() const { return m_flare.m_angle; }

		protected:
			bool m_bActive = true;
			Schematyc::Range<0, std::numeric_limits<int>::max()> m_radius = 10.f;
			CryTransform::CClampedAngle<0, 180> m_angle = 45.0_degrees;

			SProjectorOptions m_projectorOptions;
			CPointLightComponent::SOptions m_options;
			CPointLightComponent::SColor m_color;
			CPointLightComponent::SShadows m_shadows;
			CPointLightComponent::SAnimations m_animations;

			SFlare m_flare;
		};
	}
}