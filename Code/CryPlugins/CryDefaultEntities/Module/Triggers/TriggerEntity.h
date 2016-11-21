#pragma once

#include "Helpers/NativeEntityBase.h"

////////////////////////////////////////////////////////
// Entity that sends out trigger events based on entering / leaving linked areas
////////////////////////////////////////////////////////
class CTriggerEntity : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Enabled = 0,

		eNumProperties,
	};

	enum EFlowgraphInputPorts
	{
		eInputPorts_Enable = 0,
		eInputPorts_Disable
	};

	enum EFlowgraphOutputPorts
	{
		eOutputPorts_Disabled = 0,
		eOutputPorts_Enabled,
		eOutputPorts_Enter,
		eOutputPorts_Leave,
	};

public:
	CTriggerEntity();
	virtual ~CTriggerEntity() {}

	// ISimpleExtension
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~ISimpleExtension

	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

protected:
	bool m_bActive;
};
