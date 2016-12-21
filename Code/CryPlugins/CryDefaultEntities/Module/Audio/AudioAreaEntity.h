// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioEntitiesUtils.h"
#include "Helpers/DesignerEntityComponent.h"
#include <CryFlowGraph/IFlowSystem.h>
#include <CrySerialization/Decorators/ResourcesAudio.h>

class CEntityFlowNode;

class CAudioAreaEntity final
	: public CDesignerEntityComponent<>
	  , public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CAudioAreaEntity, "AudioAreaEntity", 0xA8EC43DAC8B046A8, 0x94182C1C8C56F2D1);

public:
	enum EFlowgraphInputPorts
	{
		eInputPorts_Enable = 0,
		eInputPorts_Disable,
	};

	enum EFlowgraphOutputPorts
	{
		eOutputPorts_FadeValue,
		eOutputPorts_OnFarToNear,
		eOutputPorts_OnNearToInside,
		eOutputPorts_OnInsideToNear,
		eOutputPorts_OnNearToFar,
	};

public:
	CAudioAreaEntity() = default;
	virtual ~CAudioAreaEntity() {}

	// CNativeEntityBase
	virtual uint64                GetEventMask() const override { return CDesignerEntityComponent::GetEventMask() | BIT64(ENTITY_EVENT_ENTERNEARAREA) | BIT64(ENTITY_EVENT_MOVENEARAREA) | BIT64(ENTITY_EVENT_ENTERAREA) | BIT64(ENTITY_EVENT_MOVEINSIDEAREA) | BIT64(ENTITY_EVENT_LEAVEAREA) | BIT64(ENTITY_EVENT_LEAVENEARAREA) | BIT64(ENTITY_EVENT_TIMER); }
	virtual void                  ProcessEvent(SEntityEvent& event) override;

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void                  OnResetState() override;
	// ~CNativeEntityBase

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "AudioAreaEntity Properties"; }

	virtual void        SerializeProperties(Serialization::IArchive& archive) override;
	// ~IEntityPropertyGroup

public:
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

private:
	void SetEnvironmentId(const CryAudio::ControlId environmentId);
	void UpdateFadeValue(const float distance);

	bool                     m_bEnabled = true;
	bool                     m_bTriggerAreasOnMove = false;

	EAreaState               m_areaState = EAreaState::Outside;
	float                    m_fadeDistance = 5.0f;
	float                    m_environmentFadeDistance = 5.0f;
	CryAudio::EOcclusionType m_obstructionType = CryAudio::eOcclusionType_Ignore;

	string                   m_environmentName;

	IEntityAudioComponent*   m_pProxy = nullptr;
	bool                     m_bIsActive = false;
	float                    m_fadeValue = 0.0f;

};
