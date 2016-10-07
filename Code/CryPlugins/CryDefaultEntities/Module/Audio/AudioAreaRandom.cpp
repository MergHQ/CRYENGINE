// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaRandom.h"
#include "AudioEntitiesUtils.h"

const int timerId = 0;

class CAudioAreaRandomRegistrator : public IEntityRegistrator
{
	virtual void Register() override
	{
		RegisterEntityWithDefaultComponent<CAudioAreaRandom>("AudioAreaRandom", "Audio", "AudioAreaRandom.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioAreaRandom");

		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE);
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Enabled", "bEnabled", "1", "");
		RegisterEntityPropertyAudioTrigger(pPropertyHandler, "Play Trigger", "audioTriggerPlayTriggerName", "", "");
		RegisterEntityPropertyAudioTrigger(pPropertyHandler, "Stop Trigger", "audioTriggerStopTriggerName", "", "");
		RegisterEntityPropertyAudioRtpc(pPropertyHandler, "RTPC", "audioRTPCRtpc", "", "");
		RegisterEntityProperty<bool>(pPropertyHandler, "Move with Entity", "bMoveWithEntity", "0", "");
		RegisterEntityProperty<bool>(pPropertyHandler, "Trigger Areas On Move", "bTriggerAreasOnMove", "0", "");
		RegisterEntityPropertyEnum(pPropertyHandler, "Sound Obstruction Type", "eiSoundObstructionType", "1", "", 0, 1);
		RegisterEntityProperty<float>(pPropertyHandler, "RTPC Distance", "fRtpcDistance", "5.0", "", 0.f, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "Radius Random", "fRadiusRandom", "10.0", "", 0.f, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "Min Delay", "fMinDelay", "0", "", 0.f, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "Max Delay", "fMaxDelay", "1", "", 0.f, 100000.f);
	}
};

CAudioAreaRandomRegistrator g_audioAreaRandomRegistrator;

void CAudioAreaRandom::ProcessEvent(SEntityEvent& event)
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
			if (event.nParam[0] == timerId)
			{
				Play();
			}

		}
		break;

	}
}

void CAudioAreaRandom::Reset()
{
	IEntity& entity = *GetEntity();

	auto& audioProxy = static_cast<IEntityAudioProxy&>(*entity.CreateProxy(ENTITY_PROXY_AUDIO));

	// Get properties
	const bool bEnabled = GetPropertyBool(eProperty_Enabled);
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_PlayTrigger), m_playTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_StopTrigger), m_stopTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_RTPC), m_rtpcId);
	const bool bMoveWithEntity = GetPropertyBool(eProperty_MoveWithEntity);
	const bool bTriggerAreas = GetPropertyBool(eProperty_TriggerAreasOnMove);
	const ESoundObstructionType soundObstructionType = static_cast<ESoundObstructionType>(GetPropertyInt(eProperty_SoundObstructionType));
	m_rtpcDistance = GetPropertyFloat(eProperty_RTPCDistance);
	m_radius = GetPropertyFloat(eProperty_RadiusRandom);
	m_minDelay = GetPropertyFloat(eProperty_MinDelay) * 1000.0f;
	m_maxDelay = GetPropertyFloat(eProperty_MaxDelay) * 1000.0f;

	// Update values
	audioProxy.SetFadeDistance(m_rtpcDistance);
	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	audioProxy.SetSwitchState(AudioEntitiesUtils::GetObstructionOcclusionSwitch(), stateIds[soundObstructionType]);

	audioProxy.SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
	audioProxy.SetAuxAudioProxyOffset(Matrix34(IDENTITY));
	audioProxy.AuxAudioProxiesMoveWithEntity(bMoveWithEntity);

	entity.SetFlags(entity.GetFlags() | ENTITY_FLAG_CLIENT_ONLY);
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

	if (bEnabled)
	{
		if (!m_bEnabled)
		{
			// Entity was enabled
			entity.KillTimer(timerId);
			Play();
		}
	}
	else if (m_bEnabled)
	{
		// Entity was disabled
		Stop();
	}

	m_bEnabled = bEnabled;
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
	if (auto* pAudioProxy = static_cast<IEntityAudioProxy*>(GetEntity()->GetProxy(ENTITY_PROXY_AUDIO)))
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

		GetEntity()->SetTimer(timerId, static_cast<int>(cry_random(m_minDelay, m_maxDelay)));
	}
}

void CAudioAreaRandom::Stop()
{
	IEntity& entity = *GetEntity();
	entity.KillTimer(timerId);

	if (auto* pAudioProxy = static_cast<IEntityAudioProxy*>(entity.GetProxy(ENTITY_PROXY_AUDIO)))
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
}

void CAudioAreaRandom::UpdateRtpc()
{
	if (m_rtpcId != INVALID_AUDIO_CONTROL_ID)
	{
		if (auto* pAudioProxy = static_cast<IEntityAudioProxy*>(GetEntity()->GetProxy(ENTITY_PROXY_AUDIO)))
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
