#include "StdAfx.h"
#include "EnvironmentProbe.h"

class CProbeRegistrator : public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("EnvironmentLight") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class EnvironmentLight, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CEnvironmentProbeEntity>("EnvironmentLight", "Lights", "Light.bmp");
		auto* pPropertyHandler = gEnv->pEntitySystem->GetClassRegistry()->FindClass("EnvironmentLight")->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Active", "bActive", "1", "");

		RegisterEntityProperty<float>(pPropertyHandler, "BoxSizeX", "", "10", "", 0, 10000);
		RegisterEntityProperty<float>(pPropertyHandler, "BoxSizeY", "", "10", "", 0, 10000);
		RegisterEntityProperty<float>(pPropertyHandler, "BoxSizeZ", "", "10", "", 0, 10000);

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Projection");

			RegisterEntityProperty<bool>(pPropertyHandler, "BoxProject", "bBoxProject", "0", "");

			RegisterEntityProperty<float>(pPropertyHandler, "BoxWidth", "fBoxWidth", "10", "", 0, 10000);
			RegisterEntityProperty<float>(pPropertyHandler, "BoxLength", "fBoxLength", "10", "", 0, 10000);
			RegisterEntityProperty<float>(pPropertyHandler, "BoxHeight", "fBoxHeight", "10", "", 0, 10000);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Options");

			RegisterEntityProperty<bool>(pPropertyHandler, "IgnoreVisAreas", "bIgnoresVisAreas", "0", "");

			RegisterEntityProperty<int>(pPropertyHandler, "SortPriority", "", "0", "", 0, 255);
			RegisterEntityProperty<float>(pPropertyHandler, "AttentuationFalloffMax", "fAttenuationFalloffMax", "1", "", 0, 10000);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "OptionsAdvanced");

			RegisterEntityProperty<ITexture>(pPropertyHandler, "Cubemap", "texture_deferred_cubemap", "", "");
		}
	}
};

CProbeRegistrator g_probeRegistrator;

CEnvironmentProbeEntity::CEnvironmentProbeEntity()
// Start by setting the light slot to an invalid value by default
	: m_lightSlot(-1)
{
}

void CEnvironmentProbeEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	// Check if we have to unload first
	if (m_lightSlot != -1)
	{
		entity.FreeSlot(m_lightSlot);
		m_lightSlot = -1;
	}

	if (!GetPropertyBool(eProperty_Active))
		return;

	m_light.m_nLightStyle = 0;
	m_light.SetPosition(ZERO);
	m_light.m_fLightFrustumAngle = 45;
	m_light.m_Flags = DLF_POINT | DLF_THIS_AREA_ONLY | DLF_DEFERRED_CUBEMAPS;
	m_light.m_LensOpticsFrustumAngle = 255;

	m_light.m_ProbeExtents = Vec3(GetPropertyFloat(eProperty_BoxSizeX), GetPropertyFloat(eProperty_BoxSizeY), GetPropertyFloat(eProperty_BoxSizeZ)) * 0.5f;
	m_light.m_fRadius = pow(m_light.m_ProbeExtents.GetLengthSquared(), 0.5f);

	m_light.SetLightColor(ColorF(1.f, 1.f, 1.f, 1.f));
	m_light.SetSpecularMult(1.f);

	m_light.m_fHDRDynamic = 0.f;

	if (GetPropertyBool(eProperty_IgnoreVisAreas))
		m_light.m_Flags |= DLF_IGNORES_VISAREAS;

	if (GetPropertyBool(eProperty_BoxProject))
		m_light.m_Flags |= DLF_BOX_PROJECTED_CM;

	m_light.m_fBoxWidth = GetPropertyFloat(eProperty_BoxSizeX);
	m_light.m_fBoxLength = GetPropertyFloat(eProperty_BoxSizeY);
	m_light.m_fBoxHeight = GetPropertyFloat(eProperty_BoxSizeZ);

	m_light.m_nSortPriority = GetPropertyInt(eProperty_SortPriority);
	m_light.SetFalloffMax(GetPropertyFloat(eProperty_AttenuationFalloffMax));

	m_light.m_fFogRadialLobe = 0.f;

	m_light.SetAnimSpeed(0.f);
	m_light.m_fProjectorNearPlane = 0.f;

	ITexture* pSpecularTexture, * pDiffuseTexture;
	GetCubemapTextures(GetPropertyValue(eProperty_Cubemap), &pSpecularTexture, &pDiffuseTexture);

	m_light.SetSpecularCubemap(pSpecularTexture);
	m_light.SetDiffuseCubemap(pDiffuseTexture);

	if (!m_light.GetSpecularCubemap())
	{
		m_light.SetDiffuseCubemap(nullptr);
		m_light.SetSpecularCubemap(nullptr);

		m_light.m_Flags &= ~DLF_DEFERRED_CUBEMAPS;

		CRY_ASSERT_MESSAGE(false, "Failed to load specular cubemap for environment probe!");
		return;
	}
	else if (!m_light.GetDiffuseCubemap())
	{
		m_light.SetDiffuseCubemap(nullptr);
		m_light.SetSpecularCubemap(nullptr);

		m_light.m_Flags &= ~DLF_DEFERRED_CUBEMAPS;

		CRY_ASSERT_MESSAGE(false, "Failed to load diffuse cubemap for environment probe!");
		return;
	}

	// Acquire resources, as the cubemap will be releasing when it goes out of scope
	m_light.AcquireResources();

	// Load the light source into the entity
	m_lightSlot = entity.LoadLight(1, &m_light);
}

void CEnvironmentProbeEntity::GetCubemapTextures(const char* path, ITexture** pSpecular, ITexture** pDiffuse)
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
}
