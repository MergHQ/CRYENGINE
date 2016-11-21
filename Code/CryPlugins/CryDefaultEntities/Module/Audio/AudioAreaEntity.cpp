// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaEntity.h"
#include "Helpers/EntityFlowNode.h"

class CAudioAreaEntityRegistrator : public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaEntity") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class AudioAreaEntity, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CAudioAreaEntity>("AudioAreaEntity", "Audio", "AudioAreaEntity.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaEntity");

		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE);
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Enabled", "bEnabled", "1", "");
		RegisterEntityProperty<bool>(pPropertyHandler, "Trigger Areas On Move", "bTriggerAreasOnMove", "0", "");
		RegisterEntityPropertyAudioEnvironment(pPropertyHandler, "Environment", "audioEnvironmentEnvironment", "", "");
		RegisterEntityPropertyEnum(pPropertyHandler, "Sound Obstruction Type", "eiSoundObstructionType", "1", "", 0, 1);
		RegisterEntityProperty<float>(pPropertyHandler, "Fade Distance", "fFadeDistance", "5.0", "", 0.f, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "Environment Distance", "fEnvironmentDistance", "5.0", "", 0.f, 100000.f);

		// Register flow node
		// Factory will be destroyed by flowsystem during shutdown
		CEntityFlowNodeFactory* pFlowNodeFactory = new CEntityFlowNodeFactory("entity:AudioAreaEntity");

		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Enable", ""));
		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Disable", ""));

		pFlowNodeFactory->m_activateCallback = CAudioAreaEntity::OnFlowgraphActivation;

		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<float>("FadeValue"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnFarToNear"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnNearToInside"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnInsideToNear"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnNearToFar"));

		pFlowNodeFactory->Close();

	}
};

CAudioAreaEntityRegistrator g_audioAreaEntityRegistrator;

void CAudioAreaEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	CNativeEntityBase::ProcessEvent(event);

	switch (event.event)
	{
	case ENTITY_EVENT_ENTERNEARAREA:
		if (m_areaState == EAreaState::Outside)
		{
			const float distance = event.fParam[0];
			if (distance < m_fadeDistance)
			{
				m_bIsActive = true;
				m_fadeValue = 0.0f;
				ActivateFlowNodeOutput(eOutputPorts_OnFarToNear, TFlowInputData(true));
				ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));
			}
		}
		else if (m_areaState == EAreaState::Inside)
		{
			ActivateFlowNodeOutput(eOutputPorts_OnInsideToNear, TFlowInputData(true));
			UpdateFadeValue(event.fParam[0]);
		}
		m_areaState = EAreaState::Near;
		break;

	case ENTITY_EVENT_MOVENEARAREA:
		{
			m_areaState = EAreaState::Near;
			const float distance = event.fParam[0];
			if (!m_bIsActive && distance < m_fadeDistance)
			{
				m_bIsActive = true;
				ActivateFlowNodeOutput(eOutputPorts_OnFarToNear, TFlowInputData(true));
			}
			else if (m_bIsActive && distance > m_fadeDistance)
			{
				m_bIsActive = false;
				ActivateFlowNodeOutput(eOutputPorts_OnNearToFar, TFlowInputData(true));
			}

			UpdateFadeValue(distance);

		}
		break;

	case ENTITY_EVENT_ENTERAREA:
		if (m_areaState == EAreaState::Outside)
		{
			// possible if the player is teleported or gets spawned inside the area
			// technically, the listener enters the Near Area and the Inside Area at the same time
			m_bIsActive = true;
			ActivateFlowNodeOutput(eOutputPorts_OnFarToNear, TFlowInputData(true));
		}

		m_areaState = EAreaState::Inside;
		m_fadeValue = 1.0f;
		ActivateFlowNodeOutput(eOutputPorts_OnNearToInside, TFlowInputData(true));
		ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));
		UpdateObstruction();
		break;

	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			m_areaState = EAreaState::Inside;
			const float fade = event.fParam[0];
			if ((fabsf(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon) || ((fade == 0.0) && (m_fadeValue != fade)))
			{
				m_fadeValue = fade;
				ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));

				if (!m_bIsActive && (fade > 0.0))
				{
					m_bIsActive = true;
					ActivateFlowNodeOutput(eOutputPorts_OnFarToNear, TFlowInputData(true));
					ActivateFlowNodeOutput(eOutputPorts_OnNearToInside, TFlowInputData(true));
				}
				else if (m_bIsActive && fade == 0.0)
				{
					ActivateFlowNodeOutput(eOutputPorts_OnInsideToNear, TFlowInputData(true));
					ActivateFlowNodeOutput(eOutputPorts_OnNearToFar, TFlowInputData(true));
					m_bIsActive = false;
				}
			}
		}
		break;

	case ENTITY_EVENT_LEAVEAREA:
		m_areaState = EAreaState::Near;
		ActivateFlowNodeOutput(eOutputPorts_OnInsideToNear, TFlowInputData(true));
		UpdateObstruction();
		break;

	case ENTITY_EVENT_LEAVENEARAREA:
		m_areaState = EAreaState::Outside;
		m_fadeValue = 0.0f;
		if (m_bIsActive)
		{
			ActivateFlowNodeOutput(eOutputPorts_OnNearToFar, TFlowInputData(true));
			ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));
			m_bIsActive = false;
		}
		break;

	}
}

void CAudioAreaEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	m_pProxy = crycomponent_cast<IEntityAudioProxyPtr>(entity.CreateProxy(ENTITY_PROXY_AUDIO));

	// Get properties
	m_bEnabled = GetPropertyBool(eProperty_Enabled);
	const bool bTriggerAreas = GetPropertyBool(eProperty_TriggerAreasOnMove);
	AudioControlId environmentId = INVALID_AUDIO_ENVIRONMENT_ID;
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_Environment), environmentId);
	m_obstructionType = static_cast<ESoundObstructionType>(GetPropertyInt(eProperty_SoundObstructionType));
	m_fadeDistance = GetPropertyFloat(eProperty_FadeDistance);
	m_environmentFadeDistance = GetPropertyFloat(eProperty_EnvironmentDistance);

	// Reset values
	m_pProxy->SetFadeDistance(m_fadeDistance);
	m_pProxy->SetEnvironmentFadeDistance(m_environmentFadeDistance);

	entity.SetFlags(entity.GetFlags() | ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_VOLUME_SOUND);
	if (bTriggerAreas)
	{
		entity.SetFlags(entity.GetFlags() | ENTITY_FLAG_TRIGGER_AREAS);
		entity.SetFlagsExtended(entity.GetFlagsExtended() | ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE);
	}
	else
	{
		entity.SetFlags(entity.GetFlags() & (~ENTITY_FLAG_TRIGGER_AREAS));
		entity.SetFlagsExtended(entity.GetFlagsExtended() & (~ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE));
	}

	if (m_bEnabled)
	{
		SetEnvironmentId(environmentId);
	}
	else
	{
		SetEnvironmentId(INVALID_AUDIO_ENVIRONMENT_ID);
	}

	UpdateObstruction();
}

void CAudioAreaEntity::SetEnvironmentId(const AudioControlId environmentId)
{
	const AudioEnvironmentId oldEnvironmentId = m_pProxy->GetEnvironmentId();
	m_pProxy->SetEnvironmentId(environmentId);

	//
	// TODO: The audio environment is being tampered with, we need to inform all entities affected by the area.
	//

}

void CAudioAreaEntity::UpdateObstruction()
{
	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	if (m_areaState == EAreaState::Near)
	{
		// Enable obstruction
		m_pProxy->SetSwitchState(AudioEntitiesUtils::GetObstructionOcclusionSwitch(), stateIds[m_obstructionType]);
	}
	else if (m_areaState == EAreaState::Inside)
	{
		// Disable obstruction
		m_pProxy->SetSwitchState(AudioEntitiesUtils::GetObstructionOcclusionSwitch(), stateIds[eSoundObstructionType_Ignore]);
	}
}

void CAudioAreaEntity::UpdateFadeValue(const float distance)
{
	if (!m_bEnabled)
	{
		m_fadeValue = 0.0f;
		ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));
	}
	else if (m_fadeDistance > 0.0f)
	{
		float fade = (m_fadeDistance - distance) / m_fadeDistance;
		fade = (fade > 0.0) ? fade : 0.0f;

		if (fabsf(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon)
		{
			m_fadeValue = fade;
			ActivateFlowNodeOutput(eOutputPorts_FadeValue, TFlowInputData(m_fadeValue));
		}
	}
}

void CAudioAreaEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	auto* pGameObject = gEnv->pGameFramework->GetGameObject(entityId);
	auto* pAudioAreaEntity = static_cast<CAudioAreaEntity*>(pGameObject->QueryExtension("AudioAreaEntity"));

	if (IsPortActive(pActInfo, eInputPorts_Enable))
	{
		pAudioAreaEntity->m_bEnabled = true;
	}
	else if (IsPortActive(pActInfo, eInputPorts_Disable))
	{
		pAudioAreaEntity->m_bEnabled = false;
	}

	pAudioAreaEntity->OnResetState();

}
