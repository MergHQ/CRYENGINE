#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		// Used to indicate the minimum graphical setting for an effect
		enum class EMiniumSystemSpec
		{
			Disabled = 0,
			Low,
			Medium,
			High,
			VeryHigh
		};

		class CPointLightComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void   ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			CPointLightComponent() {}
			virtual ~CPointLightComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CPointLightComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "0A86908D-642F-4590-ACEF-484E8E39F31B"_cry_guid;
				return id;
			}

			struct SOptions
			{
				inline bool operator==(const SOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				Schematyc::Range<0, 64000> m_attenuationBulbSize = CDLight().m_fAttenuationBulbSize;
				bool m_bIgnoreVisAreas = false;
				bool m_bVolumetricFogOnly = false;
				bool m_bAffectsVolumetricFog = true;
				bool m_bAffectsOnlyThisArea = true;
				bool m_bAmbient = false;
				Schematyc::Range<0, 10000> m_fogRadialLobe = CDLight().m_fFogRadialLobe;
			};

			struct SColor
			{
				inline bool operator==(const SColor &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				ColorF m_color = ColorF(1.f);
				Schematyc::Range<0, 10000, 0, 100> m_diffuseMultiplier = 1.f;
				Schematyc::Range<0, 10000> m_specularMultiplier = 1.f;
			};

			struct SShadows
			{
				inline bool operator==(const SShadows &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				EMiniumSystemSpec m_castShadowSpec = EMiniumSystemSpec::Disabled;
			};

			struct SAnimations
			{
				inline bool operator==(const SAnimations &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				uint32 m_style = 0;
				float m_speed = 1.f;
			};

			virtual void Enable(bool bEnable);
			bool IsEnabled() const { return m_bActive; }

			virtual SOptions& GetOptions() { return m_options; }
			const SOptions& GetOptions() const { return m_options; }

			virtual SColor& GetColorParameters() { return m_color; }
			const SColor& GetColorParameters() const { return m_color; }

			virtual SShadows& GetShadowParameters() { return m_shadows; }
			const SShadows& GetShadowParameters() const { return m_shadows; }

			virtual SAnimations& GetAnimationParameters() { return m_animations; }
			const SAnimations& GetAnimationParameters() const { return m_animations; }

		protected:
			bool m_bActive = true;
			Schematyc::Range<0, 32768> m_radius = 10.f;

			SOptions m_options;
			SColor m_color;
			SShadows m_shadows;
			SAnimations m_animations;
		};

		static void ReflectType(Schematyc::CTypeDesc<EMiniumSystemSpec>& desc)
		{
			desc.SetGUID("{9DDF8F33-CB8C-4BEE-A539-01BC8DAFED2E}"_cry_guid);
			desc.SetLabel("Minimum Graphical Setting");
			desc.SetDescription("Minimum graphical setting to enable an effect");
			desc.SetDefaultValue(EMiniumSystemSpec::Disabled);
			desc.AddConstant(EMiniumSystemSpec::Disabled, "None", "None");
			desc.AddConstant(EMiniumSystemSpec::Low, "Low", "Low");
			desc.AddConstant(EMiniumSystemSpec::Medium, "Medium", "Medium");
			desc.AddConstant(EMiniumSystemSpec::High, "High", "High");
			desc.AddConstant(EMiniumSystemSpec::VeryHigh, "VeryHigh", "VeryHigh");
		}

		static void ReflectType(Schematyc::CTypeDesc<CPointLightComponent::SOptions>& desc)
		{
			desc.SetGUID("{DB10AB64-7A5B-4B91-BC90-6D692D1D1222}"_cry_guid);
			desc.AddMember(&CPointLightComponent::SOptions::m_attenuationBulbSize, 'atte', "AttenuationBulbSize", "Attenuation Bulb Size", "Controls the fall-off exponentially from the origin, a value of 1 means that the light is at full intensity within a 1 meter ball before it begins to fall-off.", CDLight().m_fAttenuationBulbSize);
			desc.AddMember(&CPointLightComponent::SOptions::m_bIgnoreVisAreas, 'igvi', "IgnoreVisAreas", "Ignore VisAreas", nullptr, false);
			desc.AddMember(&CPointLightComponent::SOptions::m_bAffectsVolumetricFog, 'volf', "AffectVolumetricFog", "Affect Volumetric Fog", nullptr, true);
			desc.AddMember(&CPointLightComponent::SOptions::m_bVolumetricFogOnly, 'volo', "VolumetricFogOnly", "Only Affect Volumetric Fog", nullptr, false);
			desc.AddMember(&CPointLightComponent::SOptions::m_bAffectsOnlyThisArea, 'area', "OnlyAffectThisArea", "Only Affect This Area", nullptr, true);
			desc.AddMember(&CPointLightComponent::SOptions::m_bAmbient, 'ambi', "Ambient", "Ambient", nullptr, false);
			desc.AddMember(&CPointLightComponent::SOptions::m_fogRadialLobe, 'fogr', "FogRadialLobe", "Fog Radial Lobe", nullptr, CDLight().m_fFogRadialLobe);
		}

		static 	void ReflectType(Schematyc::CTypeDesc<CPointLightComponent::SColor>& desc)
		{
			desc.SetGUID("{B71C3414-F85A-4EAA-9CE0-5110A2E040AD}"_cry_guid);
			desc.AddMember(&CPointLightComponent::SColor::m_color, 'col', "Color", "Color", nullptr, ColorF(1.f));
			desc.AddMember(&CPointLightComponent::SColor::m_diffuseMultiplier, 'diff', "DiffMult", "Diffuse Multiplier", nullptr, 1.f);
			desc.AddMember(&CPointLightComponent::SColor::m_specularMultiplier, 'spec', "SpecMult", "Specular Multiplier", nullptr, 1.f);
		}

		static void ReflectType(Schematyc::CTypeDesc<CPointLightComponent::SShadows>& desc)
		{
			desc.SetGUID("{95F6EF06-2101-427C-9E55-481042117504}"_cry_guid);
			desc.AddMember(&CPointLightComponent::SShadows::m_castShadowSpec, 'shad', "ShadowSpec", "Minimum Shadow Graphics", "Minimum graphical setting to cast shadows from this light", EMiniumSystemSpec::Disabled);
		}

		static void ReflectType(Schematyc::CTypeDesc<CPointLightComponent::SAnimations>& desc)
		{
			desc.SetGUID("{95F6EF06-2101-427C-9E55-481042117504}"_cry_guid);
			desc.AddMember(&CPointLightComponent::SAnimations::m_style, 'styl', "Style", "Style", "Determines the light style to load, see Shaders/HWScripts/CryFX/Light.cfx for the full list", 0u);
			desc.AddMember(&CPointLightComponent::SAnimations::m_speed, 'sped', "Speed", "Speed", "Speed at which we animate", 1.f);
		}
	}
}