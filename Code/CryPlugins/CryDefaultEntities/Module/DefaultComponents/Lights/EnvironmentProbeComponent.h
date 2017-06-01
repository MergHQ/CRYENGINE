#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>
#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>

namespace Cry
{
	namespace DefaultComponents
	{
#if !defined(RELEASE) && defined(WIN32) && !defined(CRY_PLATFORM_CONSOLE)
#define SUPPORT_ENVIRONMENT_PROBE_GENERATION
#endif

		class CEnvironmentProbeComponent
			: public IEntityComponent
		{
		public:
			CEnvironmentProbeComponent()
				: m_generation(*this) {}

			virtual ~CEnvironmentProbeComponent() {}

			// IEntityComponent
			virtual void Initialize() override
			{
				if (m_generation.m_bAutoLoad && m_generation.m_generatedCubemapPath.value.size() > 0)
				{
					LoadFromDisk(m_generation.m_generatedCubemapPath);
				}
				else
				{
					FreeEntitySlot();
				}
			}

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "63297E89-EA00-4F56-AC23-2E93C68FE71D"_cry_guid;
				return id;
			}

			struct SOptions
			{
				inline bool operator==(const SOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				bool m_bIgnoreVisAreas = false;
				uint32 sortPriority = CDLight().m_nSortPriority;
				Schematyc::Range<0, 64000> m_attenuationFalloffMax = 1.f;
				bool m_bVolumetricFogOnly = false;
				bool m_bAffectsVolumetricFog = true;
				bool m_bAffectsOnlyThisArea = true;
				bool m_bBoxProjection = false;
			};

			struct SColor
			{
				inline bool operator==(const SColor &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				ColorF m_color = ColorF(1.f);
				Schematyc::Range<0, 100> m_diffuseMultiplier = 1.f;
				Schematyc::Range<0, 100> m_specularMultiplier = 1.f;
			};

			struct SGeneration
			{
				inline bool operator==(const SGeneration &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
				enum class EResolution
				{
					x256 = 256,
					x512 = 512,
					x1024 = 1024
				};
#endif

				SGeneration() {}

				SGeneration(CEnvironmentProbeComponent& component)
#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
					: m_generateButton(Serialization::ActionButton(std::function<void()>([&component]() { component.GenerateToDefaultPath(); })))
#endif
				{
				}

				bool m_bAutoLoad = true;

#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
				Serialization::FunctorActionButton<std::function<void()>> m_generateButton;
				EResolution m_resolution = EResolution::x256;
#endif

				Schematyc::TextureFileName m_generatedCubemapPath;
			};

#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
			void Generate(const Schematyc::TextureFileName& cubemapFileName)
			{
				IRenderNode* pProbeRenderNode = m_pEntity->GetRenderNode(GetEntitySlotId());

				// Hide the environment probe before generating, to prevent recursive dependency on itself
				if (pProbeRenderNode != nullptr)
				{
					pProbeRenderNode->SetRndFlags(ERF_HIDDEN, true);
				}

				string outputPath = cubemapFileName.value;

				// Fix up the texture path, just in case
				string texname = PathUtil::GetFileName(outputPath);
				string path = PathUtil::GetPathWithoutFilename(outputPath);

				// Add _CM suffix if missing
				int32 nCMSufixCheck = texname.Find("_cm");
				outputPath = PathUtil::Make(path, texname + ((nCMSufixCheck == -1) ? "_cm.tif" : ".tif"));
				outputPath = PathUtil::ToUnixPath(outputPath);

				// Now generate the cube map
				// Render 16x bigger cube map (4x4) - 16x SSAA
				int renderResolution = (int)m_generation.m_resolution * 4;

				TArray<unsigned short> vecData;
				vecData.Reserve(renderResolution * renderResolution * 6 * 4);
				vecData.SetUse(0);

				if (gEnv->pRenderer->EF_RenderEnvironmentCubeHDR(renderResolution, m_pEntity->GetWorldPos(), vecData))
				{
					TArray<unsigned short> downsampledImageData;

					int width = (int)m_generation.m_resolution * 4 * 6;
					int height = (int)m_generation.m_resolution;

					downsampledImageData.resize(width * height);

					size_t srcPitch = renderResolution * 4;
					size_t srcSlideSize = renderResolution * srcPitch;

					size_t dstPitch = (int)m_generation.m_resolution * 4;
					for (int side = 0; side < 6; ++side)
					{
						for (uint32 y = 0; y < (uint32)m_generation.m_resolution; ++y)
						{
							CryHalf4* pSrcSide = (CryHalf4*)&vecData[side * srcSlideSize];
							CryHalf4* pDst = (CryHalf4*)&downsampledImageData[side * dstPitch + y * width];
							for (uint32 x = 0; x < (uint32)m_generation.m_resolution; ++x)
							{
								Vec4 cResampledColor(0.f, 0.f, 0.f, 0.f);

								// re-sample the image at the original size
								for (uint32 yres = 0; yres < 4; ++yres)
								{
									for (uint32 xres = 0; xres < 4; ++xres)
									{
										const CryHalf4& pSrc = pSrcSide[(y * 4 + yres) * renderResolution + (x * 4 + xres)];
										cResampledColor += Vec4(CryConvertHalfToFloat(pSrc.x), CryConvertHalfToFloat(pSrc.y), CryConvertHalfToFloat(pSrc.z), CryConvertHalfToFloat(pSrc.w));
									}
								}

								cResampledColor /= 16.f;

								*pDst++ = CryHalf4(cResampledColor.x, cResampledColor.y, cResampledColor.z, cResampledColor.w);
							}
						}
					}

					outputPath = PathUtil::Make(gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryAbsolute(), outputPath);

					string outputDirectory = PathUtil::GetPathWithoutFilename(outputPath);
					gEnv->pCryPak->MakeDir(outputDirectory);

					gEnv->pRenderer->WriteTIFToDisk(downsampledImageData.Data(), (int)m_generation.m_resolution * 6, (int)m_generation.m_resolution, 2, 4, true, "HDRCubemap_highQ", outputPath);

					CResourceCompilerHelper::ERcCallResult result = CResourceCompilerHelper::CallResourceCompiler(outputPath, nullptr, nullptr, false, CResourceCompilerHelper::eRcExePath_editor, true, true);
					if (result == CResourceCompilerHelper::eRcCallResult_success)
					{
						LoadFromDisk(cubemapFileName);
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to generate cube map!");
				}

				// Un-hide the probe again
				if (pProbeRenderNode != nullptr)
				{
					pProbeRenderNode->SetRndFlags(ERF_HIDDEN, false);
				}
			}

			void GenerateToDefaultPath()
			{
				// Set the output path if nothing was specified by the user
				if (!HasOutputCubemapPath())
				{
					const char* szLevelName;

					if (ILevelInfo* pLevel = gEnv->pGameFramework->GetILevelSystem()->GetCurrentLevel())
					{
						szLevelName = pLevel->GetName();
					}
					else
					{
						szLevelName = "UnknownLevel";
					}

					string folder = string("Textures\\cubemaps\\") + szLevelName;
					m_generation.m_generatedCubemapPath.value = folder + "\\" + m_pEntity->GetName() + string("_cm.tif");
				}

				Generate(m_generation.m_generatedCubemapPath);
			}
#endif

			void LoadFromDisk(const Schematyc::TextureFileName& cubemapFileName)
			{
				ITexture* pSpecularTexture, *pDiffuseTexture;
				if (GetCubemapTextures(cubemapFileName.value, &pSpecularTexture, &pDiffuseTexture))
				{
					Load(pSpecularTexture, pDiffuseTexture);
				}
				else
				{
					FreeEntitySlot();
				}
			}

			void SetCubemap(const Schematyc::ExplicitTextureId specularTextureId, const Schematyc::ExplicitTextureId diffuseTextureId)
			{
				ITexture* pSpecularTexture = gEnv->pRenderer->EF_GetTextureByID((int)specularTextureId);
				ITexture *pDiffuseTexture = gEnv->pRenderer->EF_GetTextureByID((int)diffuseTextureId);

				if (pDiffuseTexture != nullptr && pSpecularTexture != nullptr)
				{
					Load(pSpecularTexture, pDiffuseTexture);
				}
				else
				{
					FreeEntitySlot();

					if (pDiffuseTexture == nullptr)
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load set cube map for environment probe component attached to entity %s, diffuse texture with id %i did not exist!!", m_pEntity->GetName(), (int)diffuseTextureId);
					}
					if (pSpecularTexture == nullptr)
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load set cube map for environment probe component attached to entity %s, specular texture with id %i did not exist!!", m_pEntity->GetName(), (int)specularTextureId);
					}
				}
			}

			bool GetCubemapTextures(const char* path, ITexture** pSpecular, ITexture** pDiffuse) const
			{
				stack_string specularCubemap = path;

				stack_string sSpecularName(specularCubemap);
				int strIndex = sSpecularName.find("_diff");
				if (strIndex >= 0)
				{
					sSpecularName = sSpecularName.substr(0, strIndex) + sSpecularName.substr(strIndex + 5, sSpecularName.length());
					specularCubemap = sSpecularName.c_str();
				}

				char diffuseCubemap[ICryPak::g_nMaxPath];
				cry_sprintf(diffuseCubemap, "%s%s%s.%s", PathUtil::AddSlash(PathUtil::GetPathWithoutFilename(specularCubemap)).c_str(),
					PathUtil::GetFileName(specularCubemap).c_str(), "_diff", PathUtil::GetExt(specularCubemap));

				// '\\' in filename causing texture duplication
				stack_string specularCubemapUnix = PathUtil::ToUnixPath(specularCubemap.c_str());
				stack_string diffuseCubemapUnix = PathUtil::ToUnixPath(diffuseCubemap);

				*pSpecular = gEnv->pRenderer->EF_LoadTexture(specularCubemapUnix, FT_DONT_STREAM);
				*pDiffuse = gEnv->pRenderer->EF_LoadTexture(diffuseCubemapUnix, FT_DONT_STREAM);

				if (*pSpecular == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load specular cube map for environment probe component attached to entity %s!", m_pEntity->GetName());
					return false;
				}

				if (*pDiffuse == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load diffuse cube map for environment probe component attached to entity %s!", m_pEntity->GetName());
					return false;
				}

				return true;
			}

			void Load(ITexture* pSpecular, ITexture* pDiffuse)
			{
				if (!m_bActive)
				{
					FreeEntitySlot();

					return;
				}

				CDLight light;

				light.m_nLightStyle = 0;
				light.SetPosition(ZERO);
				light.m_fLightFrustumAngle = 45;
				light.m_Flags = DLF_POINT | DLF_DEFERRED_CUBEMAPS;
				light.m_LensOpticsFrustumAngle = 255;

				light.m_ProbeExtents = m_extents;

				light.m_fBoxWidth = light.m_ProbeExtents.x;
				light.m_fBoxLength = light.m_ProbeExtents.y;
				light.m_fBoxHeight = light.m_ProbeExtents.z;

				light.m_fRadius = pow(light.m_ProbeExtents.GetLengthSquared(), 0.5f);

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

				light.SetFalloffMax(m_options.m_attenuationFalloffMax);

				if (m_options.m_bBoxProjection)
					light.m_Flags |= DLF_BOX_PROJECTED_CM;

				light.m_fFogRadialLobe = 0.f;

				light.SetAnimSpeed(0.f);
				light.m_fProjectorNearPlane = 0.f;

				// When the textures remains the same, reference count of their resources should be bigger than 1 now,
				// so light resources can be dropped without releasing textures
				light.DropResources();

				light.SetSpecularCubemap(pSpecular);
				light.SetDiffuseCubemap(pDiffuse);

				// Load the light source into the entity
				m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &light);
			}

			void Enable(bool bEnable) { m_bActive = bEnable; }
			bool IsEnabled() const { return m_bActive; }

			void EnableAutomaticLoad(bool bEnable) { m_generation.m_bAutoLoad = bEnable; }
			bool IsAutomaticLoadEnabled() const { return m_generation.m_bAutoLoad; }

			void SetExtents(const Vec3& extents) { m_extents = extents; }
			const Vec3& GetExtents() const { return m_extents; }

			SOptions& GetOptions() { return m_options; }
			const SOptions& GetOptions() const { return m_options; }

			SColor& GetColorParameters() { return m_color; }
			const SColor& GetColorParameters() const { return m_color; }

			virtual void SetOutputCubemapPath(const char* szPath);
			const char* GetOutputCubemapPath() const { return m_generation.m_generatedCubemapPath.value.c_str(); }
			bool HasOutputCubemapPath() const { return m_generation.m_generatedCubemapPath.value.size() > 0; }

		protected:
			bool m_bActive = true;

			Vec3 m_extents = Vec3(10.f);

			SOptions m_options;
			SColor m_color;
			SGeneration m_generation;
		};
	}
}