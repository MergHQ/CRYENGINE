// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Helpers/NativeEntityBase.h"
#include "AudioEntitiesUtils.h"
#include <CryFlowGraph/IFlowSystem.h>

class CEntityFlowNode;

class CAudioAreaEntity final : public CNativeEntityBase
{

public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Enabled = 0,
		eProperty_TriggerAreasOnMove,
		eProperty_Environment,
		eProperty_SoundObstructionType,
		eProperty_FadeDistance,
		eProperty_EnvironmentDistance,
		eNumProperties
	};

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
	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual void OnResetState() override;
	// ~CNativeEntityBase

public:
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

private:
	void SetEnvironmentId(const AudioControlId environmentId);
	void UpdateObstruction();
	void UpdateFadeValue(const float distance);

	bool                  m_bEnabled = true;
	EAreaState            m_areaState = EAreaState::Outside;
	float                 m_fadeDistance = 5.0f;
	float                 m_environmentFadeDistance = 5.0f;
	ESoundObstructionType m_obstructionType = eSoundObstructionType_Ignore;

	IEntityAudioComponent*m_pProxy = nullptr;
	bool                  m_bIsActive = false;
	float                 m_fadeValue = 0.0f;

};
