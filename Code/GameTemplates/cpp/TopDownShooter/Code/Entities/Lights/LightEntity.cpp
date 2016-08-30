#include "StdAfx.h"
#include "LightEntity.h"

#include "Game/GameFactory.h"

#include "FlowNodes/Helpers/FlowGameEntityNode.h"

#include <CryAnimation/ICryAnimation.h>

class CLightRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		CLightEntity::Register();
	}
};

CLightRegistrator g_lightRegistrator;

CLightEntity::CLightEntity()
	// Start by setting the light slot to an invalid value by default
	: m_lightSlot(-1)
{
}

void CLightEntity::ProcessEvent(SEntityEvent& event)
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

void CLightEntity::Reset()
{
	IEntity &entity = *GetEntity();

	// Check if we have to unload first
	if(m_lightSlot != -1)
	{
		entity.FreeSlot(m_lightSlot);
		m_lightSlot = -1;
	}

	// Check if the light is active
	if(!GetPropertyBool(eProperty_Active))
		return;

	m_light.SetPosition(ZERO);
	m_light.m_fLightFrustumAngle = 45;
	m_light.m_fProjectorNearPlane = 0;
	m_light.m_Flags = DLF_DEFERRED_LIGHT | DLF_THIS_AREA_ONLY;

	m_light.m_fRadius = GetPropertyFloat(eProperty_Radius);
	m_light.m_ProbeExtents(m_light.m_fRadius, m_light.m_fRadius, m_light.m_fRadius);

	m_light.SetShadowBiasParams(1.f, 1.f);
	m_light.m_fShadowUpdateMinRadius = m_light.m_fRadius;

	float shadowUpdateRatio = 1.f;
	m_light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1<<DL_SHADOW_UPDATE_SHIFT)));

	int castShadowSpec = GetPropertyInt(eProperty_CastShadows);
	if(castShadowSpec != 0 && gEnv->pSystem->GetConfigSpec() >= castShadowSpec)
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

// Static function
void CLightEntity::Register()
{
	auto properties = new SNativeEntityPropertyInfo[eNumProperties];
	memset(properties, 0, sizeof(SNativeEntityPropertyInfo) * eNumProperties);

	RegisterEntityProperty<bool>(properties, eProperty_Active, "Active", "1", "", 0, 1);

	RegisterEntityProperty<float>(properties, eProperty_Radius, "Radius", "10", "", 0, 100000.f);

	{
		ENTITY_PROPERTY_GROUP("Color", ePropertyGroup_ColorBegin, ePropertyGroup_ColorEnd, properties);

		RegisterEntityProperty<ColorF>(properties, eProperty_DiffuseColor, "DiffuseColor", "1,1,1", "", 0, 1.f);
		RegisterEntityProperty<float>(properties, eProperty_DiffuseMultiplier, "DiffuseMultiplier", "1", "", 0, 100.f);
	}

	{
		ENTITY_PROPERTY_GROUP("Options", ePropertyGroup_OptionsBegin, ePropertyGroup_OptionsEnd, properties);

		RegisterEntityPropertyEnum(properties, eProperty_CastShadows, "CastShadows", "0", "", 0, 4);
		RegisterEntityProperty<float>(properties, eProperty_AttenuationBulbSize, "AttenuationBulbSize", "0.05", "", 0, 100.f);
	}

	{
		ENTITY_PROPERTY_GROUP("Animation", ePropertyGroup_AnimationBegin, ePropertyGroup_AnimationEnd, properties);

		RegisterEntityProperty<int>(properties, eProperty_Style, "Style", "0", "", 0, 1000);
		RegisterEntityProperty<float>(properties, eProperty_AnimationSpeed, "Speed", "1", "", 0, 1);
	}

	// Finally, register the entity class so that instances can be created later on either via Launcher or Editor
	CGameFactory::RegisterNativeEntity<CLightEntity>("Light", "Lights", "Light.bmp", 0u, properties, eNumProperties);

	// Create flownode
	CGameEntityNodeFactory &nodeFactory = CGameFactory::RegisterEntityFlowNode("Light");

	// Define input ports, and the callback function for when they are triggered
	std::vector<SInputPortConfig> inputs;
	inputs.push_back(InputPortConfig_Void("TurnOn", "Turns the light on"));
	inputs.push_back(InputPortConfig_Void("TurnOff", "Turns the light off"));
	nodeFactory.AddInputs(inputs, OnFlowgraphActivation);

	// Mark the factory as complete, indicating that there will be no additional ports
	nodeFactory.Close();
}

void CLightEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo *pActInfo, const class CFlowGameEntityNode *pNode)
{
	if (auto *pLightSource = static_cast<CLightEntity *>(QueryExtension(entityId)))
	{
		if (IsPortActive(pActInfo, eInputPort_TurnOn))
		{
			pLightSource->SetPropertyBool(eProperty_Active, true);
		}
		else if (IsPortActive(pActInfo, eInputPort_TurnOff))
		{
			pLightSource->SetPropertyBool(eProperty_Active, false);
		}
	}
}