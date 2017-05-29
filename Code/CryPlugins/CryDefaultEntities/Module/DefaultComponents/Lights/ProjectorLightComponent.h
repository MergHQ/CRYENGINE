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
		public:
			CProjectorLightComponent() {}
			virtual ~CProjectorLightComponent() {}

			// IEntityComponent
			virtual void Initialize() override
			{
				if (!m_bActive)
				{
					FreeEntitySlot();

					return;
				}

				CDLight light;

				light.m_nLightStyle = m_animations.m_style;
				light.SetAnimSpeed(m_animations.m_speed);

				light.SetPosition(ZERO);
				light.m_Flags = DLF_DEFERRED_LIGHT | DLF_PROJECT;

				light.m_fRadius = m_radius;

				light.m_fLightFrustumAngle = m_angle.ToDegrees();
				light.m_fProjectorNearPlane = m_projectorOptions.m_nearPlane;

				light.SetLightColor(m_color.m_color * m_color.m_diffuseMultiplier);
				light.SetSpecularMult(m_color.m_specularMultiplier);

				light.m_fHDRDynamic = 0.f;

				if (m_options.m_bAffectsOnlyThisArea)
					light.m_Flags |= DLF_THIS_AREA_ONLY;

				if (m_options.m_bIgnoreVisAreas)
					light.m_Flags |= DLF_IGNORES_VISAREAS;

				if (m_options.m_bVolumetricFogOnly)
					light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;

				if (m_options.m_bAffectsVolumetricFog)
					light.m_Flags |= DLF_VOLUMETRIC_FOG;

				if (m_options.m_bAmbient)
					light.m_Flags |= DLF_AMBIENT;

				//TODO: Automatically add DLF_FAKE when using beams or flares

				if (m_shadows.m_castShadowSpec != EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_shadows.m_castShadowSpec)
				{
					light.m_Flags |= DLF_CASTSHADOW_MAPS;

					light.SetShadowBiasParams(1.f, 1.f);
					light.m_fShadowUpdateMinRadius = light.m_fRadius;

					float shadowUpdateRatio = 1.f;
					light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
				}
				else
					light.m_Flags &= ~DLF_CASTSHADOW_MAPS;

				light.m_fAttenuationBulbSize = m_options.m_attenuationBulbSize;

				light.m_fFogRadialLobe = m_options.m_fogRadialLobe;

				const char* szProjectorTexturePath = m_projectorOptions.GetTexturePath();
				if (szProjectorTexturePath[0] == '\0')
				{
					szProjectorTexturePath = "%ENGINE%/EngineAssets/Textures/lights/softedge.dds";
				}

				const char* pExt = PathUtil::GetExt(szProjectorTexturePath);
				if (!stricmp(pExt, "swf") || !stricmp(pExt, "gfx") || !stricmp(pExt, "usm") || !stricmp(pExt, "ui"))
				{
					light.m_pLightDynTexSource = gEnv->pRenderer->EF_LoadDynTexture(szProjectorTexturePath, false);
				}
				else
				{
					light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(szProjectorTexturePath, FT_DONT_STREAM);
				}

				if ((light.m_pLightImage == nullptr || !light.m_pLightImage->IsTextureLoaded()) && light.m_pLightDynTexSource == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Light projector texture %s not found, disabling projector component for entity %s", szProjectorTexturePath, m_pEntity->GetName());
					FreeEntitySlot();
					return;
				}

				if (m_flare.HasTexturePath())
				{
					int nLensOpticsId;

					if (gEnv->pOpticsManager->Load(m_flare.GetTexturePath(), nLensOpticsId))
					{
						IOpticsElementBase* pOptics = gEnv->pOpticsManager->GetOptics(nLensOpticsId);
						CRY_ASSERT(pOptics != nullptr);

						if (pOptics != nullptr)
						{
							light.SetLensOpticsElement(pOptics);

							float flareAngle = m_flare.m_angle.ToDegrees();

							if (flareAngle != 0)
							{
								int modularAngle = ((int)flareAngle) % 360;
								if (modularAngle == 0)
									light.m_LensOpticsFrustumAngle = 255;
								else
									light.m_LensOpticsFrustumAngle = (uint8)(flareAngle * (255.0f / 360.0f));
							}
							else
							{
								light.m_LensOpticsFrustumAngle = 0;
							}
						}
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Flare lens optics %s for projector component in entity %s doesn't exist!", m_flare.GetTexturePath(), GetEntity()->GetName());
						light.SetLensOpticsElement(nullptr);
					}
				}

				// Load the light source into the entity
				m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &light);

				if (m_projectorOptions.HasMaterialPath())
				{
					// Allow setting a specific material for the light in this slot, for example to set up beams
					if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_projectorOptions.GetMaterialPath(), false))
					{
						m_pEntity->SetSlotMaterial(GetEntitySlotId(), pMaterial);
					}
				}

				// Fix light orientation to point along the forward axis
				// This has to be done since lights in the engine currently emit from the right axis for some reason.
				m_pEntity->SetSlotLocalTM(GetEntitySlotId(), Matrix34::Create(Vec3(1.f), Quat::CreateRotationZ(gf_PI * 0.5f), ZERO));
			}

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

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

			void Enable(bool bEnable) { m_bActive = bEnable; }
			bool IsEnabled() const { return m_bActive; }

			void SetRadius(float radius) { m_radius = radius; }
			float GetRadius() const { return m_radius; }

			void SetFieldOfView(CryTransform::CAngle angle) { m_angle = angle; }
			CryTransform::CAngle GetFieldOfView() { return m_angle; }

			void SetNearPlane(float nearPlane) { m_projectorOptions.m_nearPlane = nearPlane; }
			float GetNearPlane() const { return m_projectorOptions.m_nearPlane; }

			CPointLightComponent::SOptions& GetOptions() { return m_options; }
			const CPointLightComponent::SOptions& GetOptions() const { return m_options; }

			CPointLightComponent::SColor& GetColorParameters() { return m_color; }
			const CPointLightComponent::SColor& GetColorParameters() const { return m_color; }

			CPointLightComponent::SShadows& GetShadowParameters() { return m_shadows; }
			const CPointLightComponent::SShadows& GetShadowParameters() const { return m_shadows; }

			CPointLightComponent::SAnimations& GetAnimationParameters() { return m_animations; }
			const CPointLightComponent::SAnimations& GetAnimationParameters() const { return m_animations; }

			void SetFlareAngle(CryTransform::CAngle angle) { m_flare.m_angle = angle; }
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