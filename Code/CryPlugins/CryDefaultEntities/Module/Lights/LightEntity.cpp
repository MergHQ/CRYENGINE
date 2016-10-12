#include "StdAfx.h"
#include "LightEntity.h"

#include "Helpers/EntityFlowNode.h"

class CLightRegistrator
	: public IEntityRegistrator
	, public IFlowNodeRegistrator
{
public:
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("Light") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class Light, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CDefaultLightEntity>("Light", "Lights", "Light.bmp");
		auto* pPropertyHandler = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Light")->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Active", "bActive", "1", "Turns the light on/off.");
		RegisterEntityProperty<float>(pPropertyHandler, "Radius", "", "10", "Specifies how far from the source the light affects the surrounding area.", 0, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "AttenuationBulbSize", "fAttenuationBulbSize", "0.05", "Specifies the radius of the area light bulb.", 0, 100.f);

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Color");

			RegisterEntityProperty<ColorF>(pPropertyHandler, "DiffuseColor", "clrDiffuse", "1,1,1", "");
			RegisterEntityProperty<float>(pPropertyHandler, "DiffuseMultiplier", "fDiffuseMultiplier", "1", "Control the strength of the diffuse color.", 0, 999.f);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Options");

			RegisterEntityProperty<bool>(pPropertyHandler, "IgnoreVisAreas", "bIgnoresVisAreas", "0", "Controls whether the light should respond to visareas.");
			RegisterEntityProperty<bool>(pPropertyHandler, "AffectsThisAreaOnly", "bAffectsThisAreaOnly", "1", "Set this parameter to false to make light cast in multiple visareas.");
			RegisterEntityProperty<bool>(pPropertyHandler, "Ambient", "bAmbient", "0", "Makes the light behave like an ambient light source, with no point of origin.");
			RegisterEntityProperty<bool>(pPropertyHandler, "FakeLight", "bFakeLight", "0", "Disables light projection, useful for lights which you only want to have Flare effects from.");
			RegisterEntityProperty<bool>(pPropertyHandler, "AffectVolumetricFog", "bVolumetricFog", "1", "Enables the light to affect volumetric fog.");
			RegisterEntityProperty<bool>(pPropertyHandler, "AffectVolumetricFogOnly", "bAffectsVolumetricFogOnly", "0", "Enables the light to affect only volumetric fog.");
			RegisterEntityProperty<float>(pPropertyHandler, "VolumetricFogRadialLobe", "fFogRadialLobe", "0", "Set the blend ratio of main and side radial lobe for volumetric fog.", 0, 1.f);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Shadows");

			RegisterEntityPropertyEnum(pPropertyHandler, "CastShadows", "eiCastShadows", "0", "", 0, 4);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Animation");

			RegisterEntityProperty<int>(pPropertyHandler, "Style", "nLightStyle", "0", "Specifies a preset animation for the light to play.", 0, 50);
			RegisterEntityProperty<float>(pPropertyHandler, "Speed", "fAnimationSpeed", "1", "Specifies the speed at which the light animation should play.", 0, 100);
			RegisterEntityProperty<float>(pPropertyHandler, "Phase", "nAnimationPhase", "0", "This will start the light style at a different point along the sequence.", 0, 100);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Projector");

			RegisterEntityProperty<string>(pPropertyHandler, "Texture", "texture_Texture", "", "Path to the projection texture we want to load.");
			RegisterEntityProperty<float>(pPropertyHandler, "FieldOfView", "fProjectorFov", "90", "Specifies the Angle on which the light texture is projected, in degrees.", 0, 180);
			RegisterEntityProperty<float>(pPropertyHandler, "NearPlane", "fProjectorNearPlane", "0", "Set the near plane for the projector, any surfaces closer to the light source than this value will not be projected on.", -100, 100);
		}

		{
			SEntityPropertyGroupHelper scopedGroup(pPropertyHandler, "Flare");

			RegisterEntityProperty<string>(pPropertyHandler, "Path", "flare_Flare", "", "Specified path to the flare effect we want to use");
			RegisterEntityProperty<float>(pPropertyHandler, "FieldOfView", "fFlareFOV", "360", "Defines the field of view of the flare in degrees", 0, 360.f);
		}

		// Register flow node
		m_pFlowNodeFactory = new CEntityFlowNodeFactory("entity:Light");

		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Active", ""));
		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Enable", ""));
		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Disable", ""));

		m_pFlowNodeFactory->m_activateCallback = CDefaultLightEntity::OnFlowgraphActivation;

		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("Active"));

		m_pFlowNodeFactory->Close();
	}
};

CLightRegistrator g_lightRegistrator;

CDefaultLightEntity::CDefaultLightEntity()
// Start by setting the light slot to an invalid value by default
	: m_lightSlot(-1)
	, m_bActive(true)
{
}

void CDefaultLightEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	// Check if we have to unload first
	if (m_lightSlot != -1)
	{
		entity.FreeSlot(m_lightSlot);
		m_lightSlot = -1;
	}

	// Check if the light is active
	if (!GetPropertyBool(eProperty_Active) || !m_bActive)
	{
		ActivateFlowNodeOutput(eOutputPorts_Active, TFlowInputData(true));
		return;
	}

	ActivateFlowNodeOutput(eOutputPorts_Active, TFlowInputData(true));

	m_light.SetPosition(ZERO);
	m_light.m_fLightFrustumAngle = 45;
	m_light.m_fProjectorNearPlane = 0;
	m_light.m_Flags = DLF_DEFERRED_LIGHT | DLF_THIS_AREA_ONLY;

	if (GetPropertyBool(eProperty_AffectThisAreaOnly))
	{
		m_light.m_Flags |= DLF_THIS_AREA_ONLY;
	}
	if (GetPropertyBool(eProperty_IgnoreVisAreas))
	{
		m_light.m_Flags |= DLF_IGNORES_VISAREAS;
	}
	if (GetPropertyBool(eProperty_Ambient))
	{
		m_light.m_Flags |= DLF_AMBIENT;
	}
	if (GetPropertyBool(eProperty_FakeLight))
	{
		m_light.m_Flags |= DLF_FAKE;
	}
	if (GetPropertyBool(eProperty_AffectVolumetricFog))
	{
		m_light.m_Flags |= DLF_VOLUMETRIC_FOG;
	}
	if (GetPropertyBool(eProperty_AffectVolumetricFogOnly))
	{
		m_light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;
	}

	m_light.m_fFogRadialLobe = GetPropertyFloat(eProperty_FogRadialLobe);

	m_light.m_fRadius = GetPropertyFloat(eProperty_Radius);
	m_light.m_ProbeExtents(m_light.m_fRadius, m_light.m_fRadius, m_light.m_fRadius);

	m_light.SetShadowBiasParams(1.f, 1.f);
	m_light.m_fShadowUpdateMinRadius = m_light.m_fRadius;

	float shadowUpdateRatio = 1.f;
	m_light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));

	int castShadowSpec = GetPropertyInt(eProperty_CastShadows);
	if (castShadowSpec != 0 && gEnv->pSystem->GetConfigSpec() >= castShadowSpec)
		m_light.m_Flags |= DLF_CASTSHADOW_MAPS;
	else
		m_light.m_Flags &= ~DLF_CASTSHADOW_MAPS;

	m_light.m_nLightStyle = GetPropertyInt(eProperty_Style);

	m_light.SetAnimSpeed(GetPropertyFloat(eProperty_AnimationSpeed));

	m_light.SetLightColor(GetPropertyColor(eProperty_DiffuseColor) * GetPropertyFloat(eProperty_DiffuseMultiplier));

	m_light.m_nSortPriority = 0;
	m_light.SetFalloffMax(1.0f);
	m_light.m_fProjectorNearPlane = 0.f;

	m_light.m_fAttenuationBulbSize = GetPropertyFloat(eProperty_AttenuationBulbSize);
	
	m_light.m_fFogRadialLobe = 0.f;

	SAFE_RELEASE(m_light.m_pLightImage);
	SAFE_RELEASE(m_light.m_pLightDynTexSource);

	const char* projectorTexture = GetPropertyValue(eProperty_ProjectionTexture);
	if (strlen(projectorTexture) > 0)
	{
		m_light.m_fLightFrustumAngle = GetPropertyFloat(eProperty_ProjectionFieldOfView);
		m_light.m_fProjectorNearPlane = GetPropertyFloat(eProperty_ProjectionNearPlane);

		const char* pExt = PathUtil::GetExt(projectorTexture);
		if (!stricmp(pExt, "swf") || !stricmp(pExt, "gfx") || !stricmp(pExt, "usm") || !stricmp(pExt, "ui"))
		{
			m_light.m_pLightDynTexSource = gEnv->pRenderer->EF_LoadDynTexture(projectorTexture, false);
		}
		else
		{
			m_light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(projectorTexture, FT_DONT_STREAM);
		}

		if ((!m_light.m_pLightImage || !m_light.m_pLightImage->IsTextureLoaded()) && !m_light.m_pLightDynTexSource)
		{
			CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, projectorTexture,
				"Light projector texture not found: %s", projectorTexture);
			m_light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture("Textures/defaults/red.dds", FT_DONT_STREAM);
		}
	}

	const char* flarePath = GetPropertyValue(eProperty_FlarePath);
	
	if (strlen(flarePath) > 0)
	{
		int nLensOpticsId;

		if (gEnv->pOpticsManager->Load(flarePath, nLensOpticsId))
		{
			if (IOpticsElementBase* pOptics = gEnv->pOpticsManager->GetOptics(nLensOpticsId))
			{
				m_light.SetLensOpticsElement(pOptics);

				float fLensOpticsFOV = GetPropertyFloat(eProperty_FlareFieldOfView);
				if (fLensOpticsFOV != 0)
				{
					int modularAngle = ((int)fLensOpticsFOV) % 360;
					if (modularAngle == 0)
						m_light.m_LensOpticsFrustumAngle = 255;
					else
						m_light.m_LensOpticsFrustumAngle = (uint8)(fLensOpticsFOV * (255.0f / 360.0f));
				}
				else
				{
					m_light.m_LensOpticsFrustumAngle = 0;
				}
			}
			else
			{
				m_light.SetLensOpticsElement(nullptr);
			}
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "Flare lens optics %s in light %s doesn't exist!", flarePath, GetEntity()->GetName());
			m_light.SetLensOpticsElement(nullptr);
		}
	}

	if (!(m_light.m_Flags & DLF_AREA_LIGHT) && m_light.m_fLightFrustumAngle && (m_light.m_pLightImage != NULL) && m_light.m_pLightImage->IsTextureLoaded() || m_light.m_pLightDynTexSource)
		m_light.m_Flags |= DLF_PROJECT;
	else
	{
		SAFE_RELEASE(m_light.m_pLightImage);
		SAFE_RELEASE(m_light.m_pLightDynTexSource);
		m_light.m_Flags |= DLF_POINT;
	}

	// Load the light source into the entity
	m_lightSlot = entity.LoadLight(1, &m_light);
}

void CDefaultLightEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	auto* pGameObject = gEnv->pGameFramework->GetGameObject(entityId);
	auto* pLightEntity = static_cast<CDefaultLightEntity*>(pGameObject->QueryExtension("Light"));

	if (IsPortActive(pActInfo, eInputPorts_Active))
	{
		pLightEntity->m_bActive = GetPortBool(pActInfo, eInputPorts_Active);

		pLightEntity->OnResetState();
	}
	else if (IsPortActive(pActInfo, eInputPorts_Enable))
	{
		pLightEntity->m_bActive = true;

		pLightEntity->OnResetState();
	}
	else if (IsPortActive(pActInfo, eInputPorts_Disable))
	{
		pLightEntity->m_bActive = false;

		pLightEntity->OnResetState();
	}
}