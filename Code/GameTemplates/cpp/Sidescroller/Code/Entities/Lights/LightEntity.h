#pragma once

#include "Entities/Helpers/NativeEntityBase.h"

class CGameEntityNodeFactory;

////////////////////////////////////////////////////////
// Sample entity for creating a light source
////////////////////////////////////////////////////////
class CLightEntity
	: public CGameObjectExtensionHelper<CLightEntity, CNativeEntityBase>
{
	enum EInputPorts
	{
		eInputPort_TurnOn = 0,
		eInputPort_TurnOff
	};

	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Active = 0,
		eProperty_Radius,

		ePropertyGroup_ColorBegin,
		eProperty_DiffuseColor,
		eProperty_DiffuseMultiplier,
		ePropertyGroup_ColorEnd,

		ePropertyGroup_OptionsBegin,
		eProperty_CastShadows,
		eProperty_AttenuationBulbSize,
		ePropertyGroup_OptionsEnd,

		ePropertyGroup_AnimationBegin,
		eProperty_Style,
		eProperty_AnimationSpeed,
		ePropertyGroup_AnimationEnd,

		eNumProperties
	};

public:
	CLightEntity();
	virtual ~CLightEntity() {}

	// CNativeEntityBase
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

public:
	// Called to register the entity class and its properties
	static void Register();

	// Called when one of the input Flowgraph ports are activated in one of the entity instances tied to this class
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CFlowGameEntityNode *pNode);

protected:
	// Called on entity spawn, or when the state of the entity changes in Editor
	void Reset();

	// Specifies the entity geometry slot in which the light is loaded, -1 if not currently loaded
	// We default to using slot 1 for this light sample, in case the user would want to put geometry into slot 0.
	int m_lightSlot;

	// Light parameters, updated in the Reset function
	CDLight m_light;
};

