// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaAmbienceEntity.h"
#include "AudioEntitiesUtils.h"

#include <CryAnimation/ICryAnimation.h>

class CAudioAreaAmbienceRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaAmbience") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class AudioAreaAmbience, overridden by game");
			return;
		}
		
		auto* pEntityClass = RegisterEntityWithDefaultComponent<CAudioAreaAmbienceEntity>("AudioAreaAmbience", "Audio", "AudioAreaAmbience.bmp");
		
		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE);
	}
};

CAudioAreaAmbienceRegistrator g_audioAreaAmbienceRegistrator;

CRYREGISTER_CLASS(CAudioAreaAmbienceEntity);

void CAudioAreaAmbienceEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	CDesignerEntityComponent::ProcessEvent(event);

	switch (event.event)
	{
	case ENTITY_EVENT_ENTERNEARAREA:
		{
			m_areaState = EAreaState::Near;

			const float distance = event.fParam[0];
			if (distance < m_rtpcDistance)
			{
				Play(m_playTriggerId);
				UpdateRtpc(0.f);
			}
		}
		break;
	case ENTITY_EVENT_MOVENEARAREA:
		{
			m_areaState = EAreaState::Near;

			const float distance = event.fParam[0];
			if (distance < m_rtpcDistance)
			{
				if (!IsPlaying())
				{
					Play(m_playTriggerId);
				}

				UpdateFadeValue(distance);
			}
			else if (IsPlaying() && distance > m_rtpcDistance)
			{
				Stop();
				UpdateFadeValue(distance);
			}
		}
		break;
	case ENTITY_EVENT_ENTERAREA:
		{
			if (m_areaState == EAreaState::Outside)
			{
				Play(m_playTriggerId);
			}

			m_areaState = EAreaState::Inside;

			UpdateRtpc(1.f);
			DisableObstruction();
		}
		break;
	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			const float distance = event.fParam[0];

			if ((abs(m_fadeValue - distance) > AudioEntitiesUtils::AreaFadeEpsilon) || (distance == 0.f && m_fadeValue != distance))
			{
				UpdateRtpc(distance);

				if (!IsPlaying() && distance > 0.f)
				{
					Play(m_playTriggerId);
				}
				else if (IsPlaying() && distance == 0.f)
				{
					Stop();
				}
			}

			m_areaState = EAreaState::Inside;
		}
		break;
	case ENTITY_EVENT_LEAVEAREA:
		{
			m_areaState = EAreaState::Outside;
			SetObstruction();
		}
		break;
	case ENTITY_EVENT_LEAVENEARAREA:
		{
			m_areaState = EAreaState::Outside;

			if (IsPlaying())
			{
				Stop();
			}

			UpdateRtpc(0.f);
		}
		break;
	}
}

void CAudioAreaAmbienceEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	if (!m_bEnabled)
	{
		if (auto pAudioProxy = entity.GetComponent<IEntityAudioComponent>())
		{
			pAudioProxy->SetEnvironmentId(INVALID_AUDIO_ENVIRONMENT_ID);

			Stop();
		}

		return;
	}

	auto& audioProxy = *(entity.GetOrCreateComponent<IEntityAudioComponent>());

	gEnv->pAudioSystem->GetAudioTriggerId(m_playTriggerName, m_playTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(m_stopTriggerName, m_stopTriggerId);
	gEnv->pAudioSystem->GetAudioRtpcId(m_rtpcName, m_rtpcId);
	gEnv->pAudioSystem->GetAudioRtpcId(m_globalRtpcName, m_globalRtpcId);

	gEnv->pAudioSystem->GetAudioEnvironmentId(m_environmentName, m_environmentId);

	m_obstructionSwitchId = AudioEntitiesUtils::GetObstructionOcclusionSwitch();

	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	audioProxy.SetSwitchState(m_obstructionSwitchId, stateIds[m_obstructionType]);

	audioProxy.SetFadeDistance(m_rtpcDistance);
	audioProxy.SetEnvironmentFadeDistance(m_environmentDistance);
	audioProxy.SetEnvironmentId(m_environmentId);

	auto entityFlags = entity.GetFlags() | ENTITY_FLAG_VOLUME_SOUND | ENTITY_FLAG_CLIENT_ONLY;

	if (m_bTriggerAreasOnMove)
	{
		entityFlags |= ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE;
		entity.SetFlagsExtended(entity.GetFlagsExtended() | ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE);
	}
	else
	{
		entityFlags &= ~ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE;
		entity.SetFlagsExtended(entity.GetFlagsExtended() & ~ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE);
	}

	entity.SetFlags(entityFlags);

	if (m_playingTriggerId != m_playTriggerId)
	{
		Play(m_playTriggerId);
	}
}

void CAudioAreaAmbienceEntity::Play(AudioControlId triggerId)
{
	if (auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>())
	{
		if (m_playingTriggerId != INVALID_AUDIO_CONTROL_ID)
		{
			pAudioProxy->StopTrigger(m_playingTriggerId);
		}

		pAudioProxy->SetCurrentEnvironments();
		pAudioProxy->ExecuteTrigger(triggerId);

		m_playingTriggerId = triggerId;
	}
}

void CAudioAreaAmbienceEntity::Stop()
{
	auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>();
	if (pAudioProxy == nullptr)
		return;

	if (m_stopTriggerId != INVALID_AUDIO_CONTROL_ID)
	{
		pAudioProxy->ExecuteTrigger(m_stopTriggerId);
	}
	else if (m_playTriggerId != INVALID_AUDIO_CONTROL_ID)
	{
		pAudioProxy->StopTrigger(m_playingTriggerId);
	}

	m_playingTriggerId = INVALID_AUDIO_CONTROL_ID;
}

void CAudioAreaAmbienceEntity::UpdateRtpc(float fadeValue)
{
	auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>();
	if (pAudioProxy == nullptr)
		return;

	if (m_rtpcId != INVALID_AUDIO_CONTROL_ID)
	{
		pAudioProxy->SetRtpcValue(m_rtpcId, fadeValue);
	}

	if (m_globalRtpcId != INVALID_AUDIO_CONTROL_ID)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData;
		requestData.audioRtpcId = m_globalRtpcId;
		requestData.value = fadeValue;
		request.pData = &requestData;
		gEnv->pAudioSystem->PushRequest(request);
	}

	m_fadeValue = fadeValue;
}

void CAudioAreaAmbienceEntity::UpdateFadeValue(float distance)
{
	if (m_rtpcDistance > 0.f)
	{
		float fade = max((m_rtpcDistance - distance) / m_rtpcDistance, 0.f);

		if (abs(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon)
		{
			UpdateRtpc(fade);
		}
	}
}

void CAudioAreaAmbienceEntity::SetObstruction()
{
	auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>();
	if (pAudioProxy == nullptr)
		return;

	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	pAudioProxy->SetSwitchState(m_obstructionSwitchId, stateIds[m_obstructionType]);
}

void CAudioAreaAmbienceEntity::DisableObstruction()
{
	auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>();
	if (pAudioProxy == nullptr)
		return;

	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	pAudioProxy->SetSwitchState(m_obstructionSwitchId, stateIds[0]);
}
