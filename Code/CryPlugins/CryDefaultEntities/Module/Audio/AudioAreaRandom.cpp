// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaRandom.h"
#include "AudioEntitiesUtils.h"

class CAudioAreaRandomRegistrator : public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaRandom") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class AudioAreaRandom, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CAudioAreaRandom>("AudioAreaRandom", "Audio", "AudioAreaRandom.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaRandom");

		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE);
	}
};

CAudioAreaRandomRegistrator g_audioAreaRandomRegistrator;

CRYREGISTER_CLASS(CAudioAreaRandom);

void CAudioAreaRandom::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	switch (event.event)
	{
	case ENTITY_EVENT_ENTERNEARAREA:
		{
			if (m_areaState == EAreaState::Outside)
			{
				m_areaState = EAreaState::Near;

				const float distance = event.fParam[0];
				if (distance < m_rtpcDistance)
				{
					Play();
					m_fadeValue = 0.0;
					UpdateRtpc();
				}
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
					Play();
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
				Play();
			}

			m_areaState = EAreaState::Inside;

			m_fadeValue = 1.0f;
			UpdateRtpc();
		}
		break;

	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			const float distance = event.fParam[0];

			if ((abs(m_fadeValue - distance) > AudioEntitiesUtils::AreaFadeEpsilon) || (distance == 0.f && m_fadeValue != distance))
			{
				m_fadeValue = distance;
				UpdateRtpc();

				if (!IsPlaying() && distance > 0.f)
				{
					Play();
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
			m_areaState = EAreaState::Near;
		}
		break;

	case ENTITY_EVENT_LEAVENEARAREA:
		{
			m_areaState = EAreaState::Outside;

			if (IsPlaying())
			{
				Stop();
			}

			m_fadeValue = 0.0f;
			UpdateRtpc();
		}
		break;

	case ENTITY_EVENT_TIMER:
		{
			if (event.nParam[0] == m_timerId)
			{
				Play();
			}

		}
		break;

	}
}

void CAudioAreaRandom::OnResetState()
{
	IEntity& entity = *GetEntity();

	auto& audioProxy = *(entity.GetOrCreateComponent<IEntityAudioComponent>());

	// Get properties
	gEnv->pAudioSystem->GetAudioTriggerId(m_playTriggerName, m_playTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(m_stopTriggerName, m_stopTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(m_rtpcName, m_rtpcId);

	// Update values
	audioProxy.SetFadeDistance(m_rtpcDistance);
	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	audioProxy.SetSwitchState(AudioEntitiesUtils::GetObstructionOcclusionSwitch(), stateIds[m_obstructionType]);

	audioProxy.SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
	audioProxy.SetAuxAudioProxyOffset(Matrix34(IDENTITY));
	audioProxy.AuxAudioProxiesMoveWithEntity(m_bMoveWithEntity);

	entity.SetFlags(entity.GetFlags() | ENTITY_FLAG_CLIENT_ONLY);
	if (m_bTriggerAreasOnMove)
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
		if (!m_bPlaying)
		{
			// Entity was enabled
			entity.KillTimer(m_timerId);
			m_timerId = 0;

			Play();
		}
	}
	else if (m_bPlaying)
	{
		// Entity was disabled
		Stop();
	}
}

Vec3 CAudioAreaRandom::GenerateOffset()
{
	Vec3 offset = { 0, 0, 0 };
	offset.x = cry_random(-1.0f, 1.0f);
	offset.y = cry_random(-1.0f, 1.0f);
	offset *= m_radius;
	return offset;
}

void CAudioAreaRandom::Play()
{
	if (auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>())
	{
		if (m_currentlyPlayingTriggerId != INVALID_AUDIO_CONTROL_ID && m_playTriggerId != m_currentlyPlayingTriggerId)
		{
			pAudioProxy->StopTrigger(m_currentlyPlayingTriggerId);
		}

		if (m_playTriggerId != INVALID_AUDIO_CONTROL_ID)
		{
			pAudioProxy->SetCurrentEnvironments();
			pAudioProxy->SetAuxAudioProxyOffset(Matrix34(IDENTITY, GenerateOffset()));

			SAudioCallBackInfo const callbackInfo(this);
			pAudioProxy->ExecuteTrigger(m_playTriggerId, DEFAULT_AUDIO_PROXY_ID, callbackInfo);
		}

		m_currentlyPlayingTriggerId = m_playTriggerId;

		GetEntity()->SetTimer(m_timerId, static_cast<int>(cry_random(m_minDelay, m_maxDelay)));
		m_bPlaying = true;
	}
}

void CAudioAreaRandom::Stop()
{
	IEntity& entity = *GetEntity();
	entity.KillTimer(m_timerId);

	if (auto pAudioProxy = entity.GetComponent<IEntityAudioComponent>())
	{
		if (m_stopTriggerId != INVALID_AUDIO_CONTROL_ID)
		{
			pAudioProxy->ExecuteTrigger(m_stopTriggerId);
		}
		else if (m_currentlyPlayingTriggerId != INVALID_AUDIO_CONTROL_ID)
		{
			pAudioProxy->StopTrigger(m_currentlyPlayingTriggerId);
		}

		m_currentlyPlayingTriggerId = INVALID_AUDIO_CONTROL_ID;
	}

	m_bPlaying = false;
}

void CAudioAreaRandom::UpdateRtpc()
{
	if (m_rtpcId != INVALID_AUDIO_CONTROL_ID)
	{
		if (auto pAudioProxy = GetEntity()->GetComponent<IEntityAudioComponent>())
		{
			pAudioProxy->SetRtpcValue(m_rtpcId, m_fadeValue);
		}
	}
}

void CAudioAreaRandom::UpdateFadeValue(float distance)
{
	if (!m_bEnabled)
	{
		m_fadeValue = 0.0f;
		UpdateRtpc();
	}
	else if (m_rtpcDistance > 0.0f)
	{
		float fade = std::max((m_rtpcDistance - distance) / m_rtpcDistance, 0.f);
		fade = (fade > 0.0f) ? fade : 0.0f;
		if (abs(m_fadeValue - fade) > AudioEntitiesUtils::AreaFadeEpsilon)
		{
			m_fadeValue = fade;
			UpdateRtpc();
		}
	}
}
