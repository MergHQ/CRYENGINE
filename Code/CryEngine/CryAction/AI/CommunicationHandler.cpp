// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CommunicationHandler.h"
#include "AIProxy.h"

namespace ATLUtils
{
void SetSwitchState(const char* switchIdName, const char* switchValue, IEntityAudioProxyPtr pIEntityAudioProxy)
{
	assert(gEnv && gEnv->pAudioSystem);
	IAudioSystem* pAudioSystem = gEnv->pAudioSystem;
	AudioControlId switchControlId(INVALID_AUDIO_CONTROL_ID);
	pAudioSystem->GetAudioSwitchId(switchIdName, switchControlId);
	if (switchControlId)
	{
		AudioSwitchStateId switchStateId(INVALID_AUDIO_SWITCH_STATE_ID);
		pAudioSystem->GetAudioSwitchStateId(switchControlId, switchValue, switchStateId);
		IF_UNLIKELY(switchStateId == INVALID_AUDIO_SWITCH_STATE_ID)
		{
			CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_WARNING, "CommunicationHandler - You are trying to switch the state of the audio switch '%s' to the value '%s'. This switch state doesn't exist.", switchIdName, switchValue);
		}
		pIEntityAudioProxy->SetSwitchState(switchControlId, switchStateId);
	}
}
}

CommunicationHandler::CommunicationHandler(CAIProxy& proxy, IEntity* entity)
	: m_proxy(proxy)
	, m_entityId(entity->GetId())
	, m_agState(0)
	, m_currentQueryID(0)
	, m_currentPlaying(0)
	, m_signalInputID(0)
	, m_actionInputID(0)
{
	assert(entity);
	Reset();

	gEnv->pAudioSystem->AddRequestListener(&CommunicationHandler::TriggerFinishedCallback, this, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance);
}

CommunicationHandler::~CommunicationHandler()
{
	gEnv->pAudioSystem->RemoveRequestListener(&CommunicationHandler::TriggerFinishedCallback, this);

	if (m_agState)
		m_agState->RemoveListener(this);
}

void CommunicationHandler::Reset()
{
	// Notify sound/voice listeners that the sounds have stopped
	{
		IEntityAudioProxyPtr pIEntityAudioProxy;
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
		if (pEntity)
		{
			pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		}

		PlayingSounds::iterator end = m_playingSounds.end();

		for (PlayingSounds::iterator it(m_playingSounds.begin()); it != end; ++it)
		{
			PlayingSound& playingSound(it->second);

			if (playingSound.listener)
			{
				ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
				playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID, m_entityId);
			}

			if (pIEntityAudioProxy)
			{
				pIEntityAudioProxy->ExecuteTrigger(playingSound.correspondingStopControlId);
			}
		}

		m_playingSounds.clear();
	}

	// Notify animation listeners that the animations have stopped
	{
		PlayingAnimations::iterator end = m_playingAnimations.end();

		for (PlayingAnimations::iterator it(m_playingAnimations.begin()); it != end; ++it)
		{
			PlayingAnimation& playingAnim(it->second);

			if (playingAnim.listener)
			{
				ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodAction) ? ActionCancelled : SignalCancelled;
				playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
			}
		}

		m_playingAnimations.clear();
	}

	const ICVar* pCvar = gEnv->pConsole->GetCVar("ai_CommunicationForceTestVoicePack");
	const bool useTestVoicePack = pCvar ? pCvar->GetIVal() == 1 : false;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
	if (pEntity)
	{
		IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		if (pIEntityAudioProxy)
		{
			const ICommunicationManager::WWiseConfiguration& wiseConfigutation = gEnv->pAISystem->GetCommunicationManager()->GetWiseConfiguration();

			if (!wiseConfigutation.switchNameForCharacterVoice.empty())
			{
				const char* voiceLibraryName = m_proxy.GetVoiceLibraryName(useTestVoicePack);
				if (voiceLibraryName && voiceLibraryName[0])
				{
					ATLUtils::SetSwitchState(wiseConfigutation.switchNameForCharacterVoice.c_str(), voiceLibraryName, pIEntityAudioProxy);
				}
			}

			if (!wiseConfigutation.switchNameForCharacterType.empty())
			{
				if (IAIObject* pAIObject = pEntity->GetAI())
				{
					if (IAIActorProxy* pAIProxy = pAIObject->GetProxy())
					{
						stack_string characterType;
						characterType.Format("%f", pAIProxy->GetFmodCharacterTypeParam());
						if (!characterType.empty())
						{
							ATLUtils::SetSwitchState(wiseConfigutation.switchNameForCharacterType.c_str(), characterType.c_str(), pIEntityAudioProxy);
						}
					}
				}
			}
		}
	}
}

bool CommunicationHandler::IsInAGState(const char* name)
{
	if (GetAGState())
	{
		char inputValue[256] = "";
		m_agState->GetInput(m_signalInputID, inputValue);

		if (strcmp(inputValue, name) == 0)
			return true;

		m_agState->GetInput(m_actionInputID, inputValue);

		if (strcmp(inputValue, name) == 0)
			return true;

	}

	return false;
}

void CommunicationHandler::ResetAnimationState()
{
	if (GetAGState())
	{
		m_agState->SetInput(m_signalInputID, "none");
		m_agState->SetInput(m_actionInputID, "idle");

		m_agState->Update();
		m_agState->ForceTeleportToQueriedState();
	}
}

void CommunicationHandler::OnReused(IEntity* entity)
{
	assert(entity);
	m_entityId = entity->GetId();
	Reset();
}

SCommunicationSound CommunicationHandler::PlaySound(CommPlayID playID, const char* name, IEventListener* listener)
{
	return PlaySound(playID, name, Sound, listener);
}

void CommunicationHandler::StopSound(const SCommunicationSound& soundToStop)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
	if (pEntity)
	{
		IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		if (pIEntityAudioProxy)
		{
			if (soundToStop.stopSoundControlId != INVALID_AUDIO_CONTROL_ID)
			{
				pIEntityAudioProxy->ExecuteTrigger(soundToStop.stopSoundControlId);
			}
			else
			{
				pIEntityAudioProxy->StopTrigger(soundToStop.playSoundControlId);
			}

			PlayingSounds::iterator it = m_playingSounds.find(soundToStop.playSoundControlId);
			if (it != m_playingSounds.end())
			{
				PlayingSound& playingSound = it->second;
				if (playingSound.listener)
				{
					ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
					playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID, m_entityId);
				}

				m_playingSounds.erase(it);
			}
		}
	}
}

SCommunicationSound CommunicationHandler::PlayVoice(CommPlayID playID, const char* variationName, IEventListener* listener)
{
	return PlaySound(playID, variationName, Voice, listener);
}

void CommunicationHandler::StopVoice(const SCommunicationSound& voiceToStop)
{
	StopSound(voiceToStop);
}

void CommunicationHandler::PlayAnimation(CommPlayID playID, const char* name, EAnimationMethod method, IEventListener* listener)
{
	bool ok = false;

	if (GetAGState())
	{
		AnimationGraphInputID inputID = method == AnimationMethodAction ? m_actionInputID : m_signalInputID;

		if (ok = m_agState->SetInput(inputID, name, &m_currentQueryID))
		{
			//Force the animation graph to update to the new signal, to provide quicker communication responsiveness
			m_agState->Update();
			m_agState->ForceTeleportToQueriedState();
			std::pair<PlayingAnimations::iterator, bool> iresult = m_playingAnimations.insert(
			  PlayingAnimations::value_type(m_currentQueryID, PlayingAnimation()));

			PlayingAnimation& playingAnimation = iresult.first->second;
			playingAnimation.listener = listener;
			playingAnimation.name = name;
			playingAnimation.method = method;
			playingAnimation.playing = m_currentPlaying;
			playingAnimation.playID = playID;
		}
		m_currentPlaying = false;
		m_currentQueryID = 0;
	}

	if (!ok && listener)
		listener->OnCommunicationHandlerEvent(
		  (method == AnimationMethodAction) ? ActionFailed : SignalFailed, playID, m_entityId);
}

void CommunicationHandler::StopAnimation(CommPlayID playID, const char* name, EAnimationMethod method)
{
	if (GetAGState())
	{
		if (method == AnimationMethodSignal)
		{
			m_agState->SetInput(m_signalInputID, "none");
		}
		else
		{
			m_agState->SetInput(m_actionInputID, "idle");
		}
		m_agState->Update();
		m_agState->ForceTeleportToQueriedState();
	}

	PlayingAnimations::iterator it = m_playingAnimations.begin();
	PlayingAnimations::iterator end = m_playingAnimations.end();

	for (; it != end; )
	{
		PlayingAnimation& playingAnim = it->second;

		bool animMatch = playID ? playingAnim.playID == playID : strcmp(playingAnim.name, name) == 0;

		if (animMatch)
		{
			if (playingAnim.listener)
			{
				ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodSignal) ? SignalCancelled : ActionCancelled;
				playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
			}

			m_playingAnimations.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

SCommunicationSound CommunicationHandler::PlaySound(CommPlayID playID, const char* name, ESoundType type, IEventListener* listener)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
	if (pEntity)
	{
		IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		if (pIEntityAudioProxy)
		{
			const ICommunicationManager::WWiseConfiguration& wiseConfigutation = gEnv->pAISystem->GetCommunicationManager()->GetWiseConfiguration();

			assert(gEnv && gEnv->pAudioSystem);
			IAudioSystem* pAudioSystem = gEnv->pAudioSystem;
			AudioControlId playCommunicationControlId(INVALID_AUDIO_CONTROL_ID);
			AudioControlId stopCommunicationControlId(INVALID_AUDIO_CONTROL_ID);
			stack_string playTriggerName;
			playTriggerName.Format("%s%s", wiseConfigutation.prefixForPlayTrigger.c_str(), name);
			stack_string stopTriggerName;
			stopTriggerName.Format("%s%s", wiseConfigutation.prefixForStopTrigger.c_str(), name);

			pAudioSystem->GetAudioTriggerId(playTriggerName, playCommunicationControlId);
			pAudioSystem->GetAudioTriggerId(stopTriggerName, stopCommunicationControlId);
			if (playCommunicationControlId != INVALID_AUDIO_CONTROL_ID)
			{
				if (listener)
				{
					std::pair<PlayingSounds::iterator, bool> iresult = m_playingSounds.insert(
					  PlayingSounds::value_type(playCommunicationControlId, PlayingSound()));

					PlayingSound& playingSound = iresult.first->second;
					playingSound.listener = listener;
					playingSound.type = type;
					playingSound.playID = playID;
				}

				SAudioCallBackInfo const callbackInfo(this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_entityId)), this, eAudioRequestFlags_PriorityNormal | eAudioRequestFlags_SyncFinishedCallback);
				pIEntityAudioProxy->ExecuteTrigger(playCommunicationControlId, DEFAULT_AUDIO_PROXY_ID, callbackInfo);

				SCommunicationSound soundInfo;
				soundInfo.playSoundControlId = playCommunicationControlId;
				soundInfo.stopSoundControlId = stopCommunicationControlId;
				return soundInfo;
			}
			else
			{
				CryLogAlways("The audio trigger to play communication '%s' is not defined.", name);
			}
		}
	}

	return SCommunicationSound();
}

void CommunicationHandler::TriggerFinishedCallback(SAudioRequestInfo const* const pAudioRequestInfo)
{
	EntityId entityId = static_cast<EntityId>(reinterpret_cast<UINT_PTR>(pAudioRequestInfo->pUserData));

	if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId))
	{
		if (IAIObject* pAIObject = pEntity->GetAI())
		{
			if (IAIActorProxy* pAiProxy = pAIObject->GetProxy())
			{
				if (IAICommunicationHandler* pCommunicationHandler = pAiProxy->GetCommunicationHandler())
				{
					pCommunicationHandler->OnSoundTriggerFinishedToPlay(pAudioRequestInfo->audioControlId);
				}
			}
		}
	}
}

void CommunicationHandler::OnSoundTriggerFinishedToPlay(const AudioControlId nTriggerID)
{
	PlayingSounds::iterator it = m_playingSounds.find(nTriggerID);
	if (it != m_playingSounds.end())
	{
		PlayingSound& playing = it->second;
		if (playing.listener)
		{
			ECommunicationHandlerEvent outEvent = (playing.type == Sound) ? SoundFinished : VoiceFinished;

			// Let the CommunicationPlayer know this sound/voice has finished
			playing.listener->OnCommunicationHandlerEvent(outEvent, playing.playID, m_entityId);
		}

		m_playingSounds.erase(it);
	}
}

IAnimationGraphState* CommunicationHandler::GetAGState()
{
	if (m_agState)
		return m_agState;

	if (IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_entityId))
	{
		if (m_agState = actor->GetAnimationGraphState())
		{
			m_agState->AddListener("AICommunicationHandler", this);
			m_signalInputID = m_agState->GetInputId("Signal");
			m_actionInputID = m_agState->GetInputId("Action");
		}
	}

	return m_agState;
}

void CommunicationHandler::QueryComplete(TAnimationGraphQueryID queryID, bool succeeded)
{
	if (queryID == m_currentQueryID) // this call happened during SetInput
	{
		m_currentPlaying = true;
		m_agState->QueryLeaveState(&m_currentQueryID);

		return;
	}

	PlayingAnimations::iterator animationIt = m_playingAnimations.find(queryID);
	if (animationIt != m_playingAnimations.end())
	{
		PlayingAnimation& playingAnimation = animationIt->second;
		if (!playingAnimation.playing)
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = succeeded ? ActionStarted : ActionFailed;
			else
				event = succeeded ? SignalStarted : SignalFailed;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);

			if (succeeded)
			{
				playingAnimation.playing = true;

				TAnimationGraphQueryID leaveQueryID;
				m_agState->QueryLeaveState(&leaveQueryID);

				m_playingAnimations.insert(PlayingAnimations::value_type(leaveQueryID, playingAnimation));
			}
		}
		else
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = ActionCancelled;
			else
				event = succeeded ? SignalFinished : SignalCancelled;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);
		}

		m_playingAnimations.erase(animationIt);
	}
}

void CommunicationHandler::DestroyedState(IAnimationGraphState* agState)
{
	if (agState == m_agState)
		m_agState = 0;
}

bool CommunicationHandler::IsPlayingAnimation() const
{
	return !m_playingAnimations.empty();
}

bool CommunicationHandler::IsPlayingSound() const
{
	return true;//!m_playingSounds.empty();
}
