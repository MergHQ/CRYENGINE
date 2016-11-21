#pragma once

#include "Helpers/NativeEntityBase.h"

#include <CryRenderer/IShader.h>

////////////////////////////////////////////////////////
// Sample entity for creating a light source
////////////////////////////////////////////////////////
class CDefaultLightEntity : public CNativeEntityBase
{
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Active = 0,
		eProperty_Radius,
		eProperty_AttenuationBulbSize,

		ePropertyGroup_ColorBegin,
		eProperty_DiffuseColor,
		eProperty_DiffuseMultiplier,
		ePropertyGroup_ColorEnd,

		ePropertyGroup_OptionsBegin,
		eProperty_IgnoreVisAreas,
		eProperty_AffectThisAreaOnly,
		eProperty_Ambient,
		eProperty_FakeLight,
		eProperty_AffectVolumetricFog,
		eProperty_AffectVolumetricFogOnly,
		eProperty_FogRadialLobe,
		ePropertyGroup_OptionsEnd,

		ePropertyGroup_ShadowsBegin,
		eProperty_CastShadows,
		ePropertyGroup_ShadowsEnd,

		ePropertyGroup_AnimationBegin,
		eProperty_Style,
		eProperty_AnimationSpeed,
		eProperty_AnimationPhase,
		ePropertyGroup_AnimationEnd,

		ePropertyGroup_ProjectionBegin,
		eProperty_ProjectionTexture,
		eProperty_ProjectionFieldOfView,
		eProperty_ProjectionNearPlane,
		ePropertyGroup_ProjectionEnd,

		ePropertyGroup_FlaresBegin,
		eProperty_FlarePath,
		eProperty_FlareFieldOfView,
		ePropertyGroup_FlaresEnd,

		eNumProperties
	};

	enum EFlowgraphInputPorts
	{
		eInputPorts_Active = 0,
		eInputPorts_Enable,
		eInputPorts_Disable,
	};

	enum EFlowgraphOutputPorts
	{
		eOutputPorts_Active,
	};

public:
	CDefaultLightEntity();
	virtual ~CDefaultLightEntity() {}

	// CNativeEntityBase
	virtual void OnResetState() override;
	// ~CNativeEntityBase

public:
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

protected:
	// Specifies the entity geometry slot in which the light is loaded, -1 if not currently loaded
	// We default to using slot 1 for this light sample, in case the user would want to put geometry into slot 0.
	int m_lightSlot;

	// Light parameters, updated in the OnResetState function
	CDLight m_light;

	// Additional active boolean coming from Flowgraph
	bool m_bActive;
};
