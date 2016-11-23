#pragma once

#include "Helpers/DesignerEntityComponent.h"

////////////////////////////////////////////////////////
// Entity that sends out trigger events based on entering / leaving linked areas
////////////////////////////////////////////////////////
class CTriggerEntity final
	: public CDesignerEntityComponent
	, public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CTriggerEntity, "TriggerEntity", 0x717FCF92E0484479, 0xAB8D6AF20D94AA85);

	CTriggerEntity();
	virtual ~CTriggerEntity() {}

public:
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
	// ISimpleExtension
	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~ISimpleExtension

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "TriggerEntity Properties"; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override
	{
		archive(m_bActive, "Active", "Active");

		if (archive.isInput())
		{
			OnResetState();
		}
	}
	// ~IEntityPropertyGroup

	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

protected:
	bool m_bActive = true;
};
