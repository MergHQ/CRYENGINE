#include "StdAfx.h"
#include "EnvironmentProbe.h"

#include "Game/GameFactory.h"

#include <CryAnimation/ICryAnimation.h>

#include "Game/GameFactory.h"

class CProbeRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		auto properties = new SNativeEntityPropertyInfo[CEnvironmentProbeEntity::eNumProperties];
		memset(properties, 0, sizeof(SNativeEntityPropertyInfo) * CEnvironmentProbeEntity::eNumProperties);

		RegisterEntityProperty<bool>(properties, CEnvironmentProbeEntity::eProperty_Active, "Active", "1", "", 0, 1);

		RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_BoxSizeX, "BoxSizeX", "10", "", 0, 10000);
		RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_BoxSizeY, "BoxSizeY", "10", "", 0, 10000);
		RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_BoxSizeZ, "BoxSizeZ", "10", "", 0, 10000);

		{
			ENTITY_PROPERTY_GROUP("Projection", CEnvironmentProbeEntity::ePropertyGroup_ProjectionBegin, CEnvironmentProbeEntity::ePropertyGroup_ProjectionEnd, properties);

			RegisterEntityProperty<bool>(properties, CEnvironmentProbeEntity::eProperty_BoxProject, "BoxProject", "0", "", 0, 1);

			RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_fBoxWidth, "BoxWidth", "10", "", 0, 10000);
			RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_fBoxLength, "BoxLength", "10", "", 0, 10000);
			RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_fBoxHeight, "BoxHeight", "10", "", 0, 10000);
		}

		{
			ENTITY_PROPERTY_GROUP("Options", CEnvironmentProbeEntity::ePropertyGroup_OptionsBegin, CEnvironmentProbeEntity::ePropertyGroup_OptionsEnd, properties);

			RegisterEntityProperty<bool>(properties, CEnvironmentProbeEntity::eProperty_IgnoreVisAreas, "IgnoreVisAreas", "0", "", 0, 1);

			RegisterEntityProperty<int>(properties, CEnvironmentProbeEntity::eProperty_SortPriority, "SortPriority", "0", "", 0, 255);
			RegisterEntityProperty<float>(properties, CEnvironmentProbeEntity::eProperty_AttenuationFalloffMax, "AttentuationFalloffMax", "1", "", 0, 10000);
		}

		{
			ENTITY_PROPERTY_GROUP("Advanced", CEnvironmentProbeEntity::ePropertyGroup_AdvancedBegin, CEnvironmentProbeEntity::ePropertyGroup_AdvancedEnd, properties);

			RegisterEntityPropertyTexture(properties, CEnvironmentProbeEntity::eProperty_Cubemap, "Cubemap", "", "");
		}

		// Finally, register the entity class so that instances can be created later on either via Launcher or Editor
		CGameFactory::RegisterNativeEntity<CEnvironmentProbeEntity>("EnvironmentLight", "Lights", "Light.bmp", 0u, properties, CEnvironmentProbeEntity::eNumProperties);
	}
};

CProbeRegistrator g_probeRegistrator;

CEnvironmentProbeEntity::CEnvironmentProbeEntity()
	// Start by setting the light slot to an invalid value by default
	: m_lightSlot(-1)
{
}

void CEnvironmentProbeEntity::ProcessEvent(SEntityEvent& event)
{
	if(gEnv->IsDedicated())
		return;

	switch(event.event)
	{
		// Physicalize on level start for Launcher
	case ENTITY_EVENT_START_LEVEL:
		// Editor specific, physicalize on reset, property change or transform change
	case ENTITY_EVENT_RESET:
	case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
	case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
		Reset();
		break;
	}
}

void CEnvironmentProbeEntity::Reset()
{
	IEntity &entity = *GetEntity();

	// Check if we have to unload first
	if(m_lightSlot != -1)
	{
		entity.FreeSlot(m_lightSlot);
		m_lightSlot = -1;
	}

	if(!GetPropertyBool(eProperty_Active))
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

	if(GetPropertyBool(eProperty_IgnoreVisAreas))
		m_light.m_Flags |= DLF_IGNORES_VISAREAS;

	if(GetPropertyBool(eProperty_BoxProject))
		m_light.m_Flags |= DLF_BOX_PROJECTED_CM;

	m_light.m_fBoxWidth = GetPropertyFloat(eProperty_BoxSizeX);
	m_light.m_fBoxLength  = GetPropertyFloat(eProperty_BoxSizeY);
	m_light.m_fBoxHeight = GetPropertyFloat(eProperty_BoxSizeZ);

	m_light.m_nSortPriority = GetPropertyInt(eProperty_SortPriority);
	m_light.SetFalloffMax(GetPropertyFloat(eProperty_AttenuationFalloffMax));

	m_light.m_fFogRadialLobe = 0.f;

	m_light.SetAnimSpeed(0.f);
	m_light.m_fProjectorNearPlane = 0.f;

	ITexture *pSpecularTexture, *pDiffuseTexture;
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
	else if(!m_light.GetDiffuseCubemap())
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

void CEnvironmentProbeEntity::GetCubemapTextures(const char *path, ITexture **pSpecular, ITexture **pDiffuse)
{
	stack_string specularCubemap = path;

	stack_string sSpecularName(specularCubemap);
	int strIndex = sSpecularName.find("_diff");
	if(strIndex >= 0)
	{
		sSpecularName = sSpecularName.substr(0, strIndex) + sSpecularName.substr(strIndex + 5, sSpecularName.length());
		specularCubemap = sSpecularName.c_str();
	}

	char diffuseCubemap[ICryPak::g_nMaxPath];
	_snprintf(diffuseCubemap, sizeof(diffuseCubemap), "%s%s%s.%s", PathUtil::AddSlash(PathUtil::GetPathWithoutFilename(specularCubemap)).c_str(), 
																							PathUtil::GetFileName(specularCubemap).c_str(), "_diff", PathUtil::GetExt(specularCubemap));

	// '\\' in filename causing texture duplication
	stack_string specularCubemapUnix = PathUtil::ToUnixPath(specularCubemap.c_str());
	stack_string diffuseCubemapUnix = PathUtil::ToUnixPath(diffuseCubemap);

	*pSpecular = gEnv->pRenderer->EF_LoadTexture(specularCubemapUnix, FT_DONT_STREAM);
	*pDiffuse = gEnv->pRenderer->EF_LoadTexture(diffuseCubemapUnix, FT_DONT_STREAM);
}