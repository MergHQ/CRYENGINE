#include "StdAfx.h"
#include "TriggerEntity.h"

#include "Helpers/EntityFlowNode.h"

class CTriggerEntityRegistrator
	: public IEntityRegistrator
	, public IFlowNodeRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("AreaTrigger") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class AreaTrigger, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CTriggerEntity>("AreaTrigger", "Triggers");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AreaTrigger");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Enabled", "bEnabled", "1", "");

		// Register flow node
		m_pFlowNodeFactory = new CEntityFlowNodeFactory("entity:AreaTrigger");

		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Enable", ""));
		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig_Void("Disable", ""));

		m_pFlowNodeFactory->m_activateCallback = CTriggerEntity::OnFlowgraphActivation;

		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig_Void("Disabled"));
		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig_Void("Enabled"));
		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<EntityId>("Enter"));
		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<EntityId>("Leave"));

		m_pFlowNodeFactory->Close();
	}
};

CTriggerEntityRegistrator g_triggerEntityRegistrator;

CTriggerEntity::CTriggerEntity()
	: m_bActive(true)
{

}

void CTriggerEntity::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_ENTERAREA:
			{
				if (!m_bActive)
					return;

				ActivateFlowNodeOutput(eOutputPorts_Enter, TFlowInputData((EntityId)event.nParam[0]));
			}
			break;
		case ENTITY_EVENT_LEAVEAREA:
			{
				if (!m_bActive)
					return;

				ActivateFlowNodeOutput(eOutputPorts_Leave, TFlowInputData((EntityId)event.nParam[0]));
			}
			break;
	}
}

void CTriggerEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	auto* pGameObject = gEnv->pGameFramework->GetGameObject(entityId);
	auto* pTriggerEntity = static_cast<CTriggerEntity*>(pGameObject->QueryExtension("AreaTrigger"));

	if (IsPortActive(pActInfo, eInputPorts_Enable))
	{
		pTriggerEntity->m_bActive = GetPortBool(pActInfo, eInputPorts_Enable);

		if (pTriggerEntity->m_bActive)
		{
			pTriggerEntity->ActivateFlowNodeOutput(eOutputPorts_Enabled, TFlowInputData());
		}
		else
		{
			pTriggerEntity->ActivateFlowNodeOutput(eOutputPorts_Disabled, TFlowInputData());
		}
	}
	else if (IsPortActive(pActInfo, eInputPorts_Disable))
	{
		pTriggerEntity->m_bActive = false;

		pTriggerEntity->ActivateFlowNodeOutput(eOutputPorts_Disabled, TFlowInputData());
	}
}