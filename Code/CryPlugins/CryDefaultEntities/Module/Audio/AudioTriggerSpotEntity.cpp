// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioTriggerSpotEntity.h"
#include "AudioEntitiesUtils.h"
#include <CryMath/Random.h>
#include <CryRenderer/IRenderAuxGeom.h>

#define DELAY_TIMER_ID 0

class CAudioTriggerSpotRegistrator final : public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioTriggerSpot") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class AudioTriggerSpot, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CAudioTriggerSpotEntity>("AudioTriggerSpot", "Audio", "Sound.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioTriggerSpot");

		pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE);
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Enabled", "bEnabled", "1", "");
		RegisterEntityPropertyAudioTrigger(pPropertyHandler, "Play Trigger", "audioTriggerPlayTriggerName", "", "");
		RegisterEntityPropertyAudioTrigger(pPropertyHandler, "Stop Trigger", "audioTriggerStopTriggerName", "", "");
		RegisterEntityProperty<bool>(pPropertyHandler, "Trigger Areas On Move", "bTriggerAreasOnMove", "0", "");
		RegisterEntityPropertyEnum(pPropertyHandler, "Sound Obstruction Type", "eiSoundObstructionType", "1", "", 0, 1);
		RegisterEntityPropertyEnum(pPropertyHandler, "Behaviour", "eiBehaviour", "0", "", 0, 1);
		RegisterEntityProperty<float>(pPropertyHandler, "Min Delay", "fMinDelay", "0", "", 0.f, 100000.f);
		RegisterEntityProperty<float>(pPropertyHandler, "Max Delay", "fMaxDelay", "1", "", 0.f, 100000.f);
		RegisterEntityProperty<Vec3>(pPropertyHandler, "Randomization Area", "vectorRandomizationArea", "", "");
		RegisterEntityPropertyEnum(pPropertyHandler, "Draw Activity Radius", "eiDrawActivityRadius", "0", "", 0, 1);
		RegisterEntityProperty<bool>(pPropertyHandler, "Draw Randomization Area", "bDrawRandomizationArea", "0", "");

	}
};

CAudioTriggerSpotRegistrator g_audioTriggerSpotRegistrator;

CAudioTriggerSpotEntity::CAudioTriggerSpotEntity()
{
	gEnv->pAudioSystem->AddRequestListener(&CAudioTriggerSpotEntity::OnAudioTriggerFinished, this, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance);
}

CAudioTriggerSpotEntity::~CAudioTriggerSpotEntity()
{
	gEnv->pAudioSystem->RemoveRequestListener(&CAudioTriggerSpotEntity::OnAudioTriggerFinished, this);
	Stop();
}

void CAudioTriggerSpotEntity::PostInit(IGameObject* pGameObject)
{
#if !defined(_RELEASE)
	pGameObject->EnableUpdateSlot(this, 0);
#endif

	CNativeEntityBase::PostInit(pGameObject);
}

void CAudioTriggerSpotEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	CNativeEntityBase::ProcessEvent(event);

	switch (event.event)
	{
	case ENTITY_EVENT_ENTERNEARAREA:
		{

		}
		break;
	case ENTITY_EVENT_MOVENEARAREA:
		{

		}
		break;
	case ENTITY_EVENT_ENTERAREA:
		{

		}
		break;
	case ENTITY_EVENT_MOVEINSIDEAREA:
		{

		}
		break;
	case ENTITY_EVENT_LEAVEAREA:
		{

		}
		break;
	case ENTITY_EVENT_LEAVENEARAREA:
		{

		}
	case ENTITY_EVENT_TIMER:
		{
			if (event.nParam[0] == DELAY_TIMER_ID)
			{
				Play();
				if (m_behaviour == ePlayBehaviour_TriggerRate)
				{
					GetEntity()->SetTimer(DELAY_TIMER_ID, static_cast<int>(cry_random(m_minDelay, m_maxDelay)));
				}
			}

		}
		break;
	}
}

void CAudioTriggerSpotEntity::TriggerFinished(const AudioControlId trigger)
{
	// If in delay mode, set a timer to play again. Note that the play trigger
	// could have been changed  and this event refers the previous one finishing
	// playing, that instance we need to ignore.
	if (m_bEnabled && trigger == m_playTriggerId && m_behaviour == ePlayBehaviour_Delay)
	{
		GetEntity()->SetTimer(DELAY_TIMER_ID, static_cast<int>(cry_random(m_minDelay, m_maxDelay)));
	}
}

void CAudioTriggerSpotEntity::OnAudioTriggerFinished(SAudioRequestInfo const* const pAudioRequestInfo)
{
	CAudioTriggerSpotEntity* pAudioTriggerSpot = static_cast<CAudioTriggerSpotEntity*>(pAudioRequestInfo->pOwner);
	pAudioTriggerSpot->TriggerFinished(pAudioRequestInfo->audioControlId);
}

void CAudioTriggerSpotEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	auto& audioProxy = *(entity.GetOrCreateComponent<IEntityAudioComponent>());

	// Get properties
	const bool bEnabled = GetPropertyBool(eProperty_Enabled);
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_PlayTrigger), m_playTriggerId);
	gEnv->pAudioSystem->GetAudioTriggerId(GetPropertyValue(eProperty_StopTrigger), m_stopTriggerId);
	const bool bTriggerAreas = GetPropertyBool(eProperty_TriggerAreasOnMove);
	const EPlayBehaviour behaviour = static_cast<EPlayBehaviour>(GetPropertyInt(eProperty_Behaviour));
	const ESoundObstructionType soundObstructionType = static_cast<ESoundObstructionType>(GetPropertyInt(eProperty_SoundObstructionType));
	m_minDelay = GetPropertyFloat(eProperty_MinDelay) * 1000.0f;
	m_maxDelay = GetPropertyFloat(eProperty_MaxDelay) * 1000.0f;
	m_randomizationArea = GetPropertyVec3(eProperty_RandomizationArea);
#if !defined(_RELEASE)
	m_bDrawRandomizationArea = GetPropertyBool(eProperty_DrawRandomizationArea);
	m_drawActivityRadius = static_cast<EDrawActivityRadius>(GetPropertyInt(eProperty_DrawActivityRadius));
#endif

	// Reset values to their default
	audioProxy.SetAuxAudioProxyOffset(Matrix34(IDENTITY));
	audioProxy.SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
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

	const auto& stateIds = AudioEntitiesUtils::GetObstructionOcclusionStateIds();
	audioProxy.SetSwitchState(AudioEntitiesUtils::GetObstructionOcclusionSwitch(), stateIds[soundObstructionType]);

	const bool bNewBehaviour = (behaviour != m_behaviour);
	m_behaviour = behaviour;

	if (bEnabled)
	{
		if (!m_bEnabled || (m_playTriggerId != m_currentlyPlayingTriggerId) || bNewBehaviour)
		{
			if (bNewBehaviour)
			{
				// Have to stop all running instances if the behaviour changes
				if (m_currentlyPlayingTriggerId != INVALID_AUDIO_CONTROL_ID)
				{
					audioProxy.StopTrigger(m_currentlyPlayingTriggerId);
				}
			}

			// Entity was enabled or an important property has changed
			StartPlayingBehaviour();
		}

	}
	else if (m_bEnabled)
	{
		// Entity was disabled
		Stop();
	}

	m_bEnabled = bEnabled;

}

void CAudioTriggerSpotEntity::StartPlayingBehaviour()
{
	IEntity& entity = *GetEntity();
	entity.KillTimer(DELAY_TIMER_ID);

	Play();

	if (m_behaviour == ePlayBehaviour_TriggerRate)
	{
		entity.SetTimer(DELAY_TIMER_ID, static_cast<int>(cry_random(m_minDelay, m_maxDelay)));
	}
}

void CAudioTriggerSpotEntity::Play()
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
	}
}

void CAudioTriggerSpotEntity::Stop()
{
	IEntity& entity = *GetEntity();
	entity.KillTimer(DELAY_TIMER_ID);

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
}

Vec3 CAudioTriggerSpotEntity::GenerateOffset()
{
	Vec3 offset = { 0, 0, 0 };
	offset.x = cry_random(-m_randomizationArea.x / 2.0f, m_randomizationArea.x / 2.0f);
	offset.y = cry_random(-m_randomizationArea.y / 2.0f, m_randomizationArea.y / 2.0f);
	offset.z = cry_random(-m_randomizationArea.z / 2.0f, m_randomizationArea.z / 2.0f);
	return offset;
}

#if !defined(_RELEASE)
void CAudioTriggerSpotEntity::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	if (m_drawActivityRadius > eDrawActivityRadius_Disabled || m_bDrawRandomizationArea)
	{
		IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		if (pRenderAuxGeom)
		{
			const SAuxGeomRenderFlags oldFlags = pRenderAuxGeom->GetRenderFlags();
			SAuxGeomRenderFlags newFlags = oldFlags;
			newFlags.SetCullMode(e_CullModeNone);
			newFlags.SetFillMode(e_FillModeWireframe);
			newFlags.SetAlphaBlendMode(e_AlphaBlended);
			pRenderAuxGeom->SetRenderFlags(newFlags);

			const Vec3 pos = GetEntity()->GetWorldPos();

			// Activity Radius
			if (m_drawActivityRadius > eDrawActivityRadius_Disabled)
			{
				const AudioControlId triggerId = m_drawActivityRadius == eDrawActivityRadius_PlayTrigger ? m_playTriggerId : m_stopTriggerId;
				SAudioTriggerData audioTriggerData;
				gEnv->pAudioSystem->GetAudioTriggerData(triggerId, audioTriggerData);

				pRenderAuxGeom->DrawSphere(pos, audioTriggerData.radius, ColorB(250, 100, 100, 100), false);
				if (audioTriggerData.occlusionFadeOutDistance > 0.0f)
				{
					pRenderAuxGeom->DrawSphere(pos, audioTriggerData.radius - audioTriggerData.occlusionFadeOutDistance, ColorB(200, 200, 255, 100), false);
				}
			}

			// Randomization Area
			if (m_bDrawRandomizationArea)
			{
				newFlags.SetFillMode(e_FillModeSolid);
				pRenderAuxGeom->SetRenderFlags(newFlags);

				const AABB bbox(-m_randomizationArea * 0.5f, m_randomizationArea * 0.5f);
				const OBB obb = OBB::CreateOBBfromAABB(Matrix33::CreateRotationXYZ(GetEntity()->GetWorldAngles()), bbox);
				pRenderAuxGeom->DrawOBB(obb, pos, true, ColorB(255, 128, 128, 128), eBBD_Faceted);
			}

			pRenderAuxGeom->SetRenderFlags(oldFlags);
		}
	}
}
#endif
