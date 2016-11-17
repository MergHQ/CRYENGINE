// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaEntity.h"

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
	}
};

CAudioAreaEntityRegistrator g_audioAreaEntityRegistrator;

void CAudioAreaEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	switch (event.event)
	{
	// Physicalize on level start for Launcher
	case ENTITY_EVENT_START_LEVEL:
	// Editor specific, physicalize on reset, property change or transform change
	case ENTITY_EVENT_RESET:
	case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
	case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
		Reset();
		break;

	case ENTITY_EVENT_ENTERNEARAREA:
		if (m_areaState == EAreaState::Outside)
		{
			const float distance = event.fParam[0];
			if (distance < m_fadeDistance)
			{
				m_bIsActive = true;
				m_fadeValue = 0.0f;
				// ACTIVATE_OUTPUT("OnFarToNear", true)
				// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)
			}
		}
		else if (m_areaState == EAreaState::Inside)
		{
			//m_fadeValue = fade;
			// ACTIVATE_OUTPUT("OnInsideToNear", true)
			// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)
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
				// ACTIVATE_OUTPUT("OnFarToNear", true)
				UpdateFadeValue(distance);
			}
			else if (m_bIsActive && distance > m_fadeDistance)
			{
				// ACTIVATE_OUTPUT("OnNearToFar", true)
				UpdateFadeValue(distance);
				m_bIsActive = false;
			}
			else if (distance < m_fadeDistance)
			{
				UpdateFadeValue(distance);
			}
		}
		break;

	case ENTITY_EVENT_ENTERAREA:
		if (m_areaState == EAreaState::Outside)
		{
			// possible if the player is teleported or gets spawned inside the area
			// technically, the listener enters the Near Area and the Inside Area at the same time
			m_bIsActive = true;
			// ACTIVATE_OUTPUT("OnFarToNear", true)
		}

		m_areaState = EAreaState::Inside;
		m_fadeValue = 1.0f;
		// ACTIVATE_OUTPUT("OnNearToInside", true)
		// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)
		UpdateObstruction();
		break;

	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			m_areaState = EAreaState::Inside;
			const float fade = event.fParam[0];
			if ((fabsf(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon) || ((fade == 0.0) && (m_fadeValue != fade)))
			{
				m_fadeValue = fade;
				// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)

				if (!m_bIsActive && (fade > 0.0))
				{
					m_bIsActive = true;
					// ACTIVATE_OUTPUT("OnFarToNear", true)
					// ACTIVATE_OUTPUT("OnNearToInside", true)

				}
				else if (m_bIsActive && fade == 0.0)
				{
					// ACTIVATE_OUTPUT("OnInsideToNear", true)
					// ACTIVATE_OUTPUT("OnNearToFar", true)
					m_bIsActive = false;
				}
			}
		}
		break;

	case ENTITY_EVENT_LEAVEAREA:
		m_areaState = EAreaState::Near;
		// ACTIVATE_OUTPUT("OnInsideToNear", true)
		UpdateObstruction();
		break;

	case ENTITY_EVENT_LEAVENEARAREA:
		m_areaState = EAreaState::Outside;
		m_fadeValue = 0.0f;
		if (m_bIsActive)
		{
			// ACTIVATE_OUTPUT("OnNearToFar", true)
			// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)
			m_bIsActive = false;
		}
		break;

	}
}

void CAudioAreaEntity::Reset()
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
		// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)

	}
	else if (m_fadeDistance > 0.0f)
	{
		float fade = (m_fadeDistance - distance) / m_fadeDistance;
		fade = (fade > 0.0) ? fade : 0.0f;

		if (fabsf(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon)
		{
			m_fadeValue = fade;
			// ACTIVATE_OUTPUT("FadeValue", m_fadeValue)
		}
	}
}
