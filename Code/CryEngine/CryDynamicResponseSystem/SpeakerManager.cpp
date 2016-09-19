// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SpeakerManager.h"
#include "ResponseSystem.h"

#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryEntitySystem/IEntityProxy.h>
#include <CryAnimation/IAttachment.h>
#include <CrySystem/IConsole.h>
#include <CryRenderer/IRenderer.h>
#include <CryGame/IGameFramework.h>
#include <CryGame/IGame.h>
#include "DialogLineDatabase.h"

namespace
{
static const uint32 s_attachmentPosName = CCrc32::ComputeLowercase("voice");
static const uint32 s_attachmentPosNameFallback = CCrc32::ComputeLowercase("eye_left");
static const CHashedString s_isTalkingVariableName = "IsTalking";
static const CHashedString s_lastLineId = "LastSpokenLine";
static const CHashedString s_lastLineFinishTime = "LastLineFinishTime";
static const CHashedString s_currentLinePriority = "CurrentLinePriority";
static const int s_characterSlot = 0;                        //todo: make this a property of the drs actor
static const float s_delayBeforeExecutingQueuedLines = 0.1f; // we do wait a short amount of time before actually starting any queued lines, to reduce the changes to interrupt a running dialog
}

using namespace CryDRS;

float CSpeakerManager::s_defaultPauseAfterLines = 0.2f;

//--------------------------------------------------------------------------------------------------
CSpeakerManager::CSpeakerManager() : m_listeners(2)
{
	//the audio callbacks we are interested in, all related to audio-asset (trigger or standaloneFile) finished or failed to start
	gEnv->pAudioSystem->AddRequestListener(&CSpeakerManager::OnAudioCallback, this, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance | eAudioCallbackManagerRequestType_ReportStoppedFile | eAudioCallbackManagerRequestType_ReportStartedFile);
	gEnv->pAudioSystem->AddRequestListener(&CSpeakerManager::OnAudioCallback, this, eAudioRequestType_AudioObjectRequest, eAudioObjectRequestType_ExecuteTrigger | eAudioObjectRequestType_PlayFile);

	m_audioRtpcIdGlobal = INVALID_AUDIO_CONTROL_ID;
	m_audioRtpcIdLocal = INVALID_AUDIO_CONTROL_ID;
	m_numActiveSpeaker = 0;

	REGISTER_CVAR2("drs_dialogSubtitles", &m_displaySubtitlesCVar, 0, VF_NULL, "Toggles use of subtitles for dialog lines on and off.\n");
	REGISTER_CVAR2("drs_dialogAudio", &m_playAudioCVar, 1, VF_NULL, "Toggles playback of audio files for dialog lines on and off.\n");
	REGISTER_CVAR2("drs_dialogsSamePriorityCancels", &m_samePrioCancelsLinesCVar, 1, VF_NULL, "If a new line is started with the same priority as a already running line, that line will be canceled.");
	REGISTER_CVAR2("drs_dialogsDefaultMaxQueueTime", &m_defaultMaxQueueTime, 3.0f, VF_NULL, "If a new line is queued (because of a already running line with higher priority) it will wait for this amount of seconds before simply being skipped.");
	REGISTER_CVAR2("drs_dialogsDefaultPauseAfterLines", &s_defaultPauseAfterLines, 0.2f, VF_NULL, "Artificial pause after a line is done, can be used to make dialog sound a bit more natural.");

	m_pDrsDialogDialogRunningEntityRtpcName = REGISTER_STRING("drs_dialogEntityRtpcName", "", 0, "name of the rtpc on the entity to set to 1 when someone is speaking");
	m_pDrsDialogDialogRunningGlobalRtpcName = REGISTER_STRING("drs_dialogGlobalRtpcName", "", 0, "name of the global rtpc to set to 1 when someone is speaking");

	m_pLipsyncProvider = nullptr;
	m_pDefaultLipsyncProvider = nullptr;
}

//--------------------------------------------------------------------------------------------------
CSpeakerManager::~CSpeakerManager()
{
	delete(m_pDefaultLipsyncProvider);

	gEnv->pAudioSystem->RemoveRequestListener(nullptr, this);  //remove all listener-callback-functions from this object

	gEnv->pConsole->UnregisterVariable("drs_dialogSubtitles", true);
	gEnv->pConsole->UnregisterVariable("drs_dialogAudio", true);
	gEnv->pConsole->UnregisterVariable("drs_dialogsLinesWithSamePriorityCancel", true);

	if (m_pDrsDialogDialogRunningEntityRtpcName)
	{
		m_pDrsDialogDialogRunningEntityRtpcName->Release();
		m_pDrsDialogDialogRunningEntityRtpcName = nullptr;
	}
	if (m_pDrsDialogDialogRunningGlobalRtpcName)
	{
		m_pDrsDialogDialogRunningGlobalRtpcName->Release();
		m_pDrsDialogDialogRunningGlobalRtpcName = nullptr;
	}
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::Init()
{
	if (!m_pLipsyncProvider)
	{
		m_pDefaultLipsyncProvider = new CDefaultLipsyncProvider;
		m_pLipsyncProvider = m_pDefaultLipsyncProvider;
	}

	if (m_pDrsDialogDialogRunningEntityRtpcName)
	{
		gEnv->pAudioSystem->GetAudioRtpcId(m_pDrsDialogDialogRunningEntityRtpcName->GetString(), m_audioRtpcIdLocal);
	}
	if (m_pDrsDialogDialogRunningGlobalRtpcName)
	{
		gEnv->pAudioSystem->GetAudioRtpcId(m_pDrsDialogDialogRunningGlobalRtpcName->GetString(), m_audioRtpcIdGlobal);
	}
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::Update()
{
	SET_DRS_USER_SCOPED("SpeakerManager Update");
	if (m_activeSpeakers.empty())
	{
		SetNumActiveSpeaker(0);
		if (m_recentlyFinishedSpeakers.empty())
		{
			return;
		}
	}

	const float currentTime = CResponseSystem::GetInstance()->GetCurrentDrsTime();
	const float fColorBlue[4] = { 0.4f, 0.4f, 1.0f, 1.0f };
	float currentPos2Y = 10.0f;

	//draw text and flag finished speakers
	for (SpeakerList::iterator it = m_activeSpeakers.begin(), itEnd = m_activeSpeakers.end(); it != itEnd; ++it)
	{
		if (m_pLipsyncProvider && it->endingConditions & eEC_WaitingForLipsync)
		{
			if (!m_pLipsyncProvider->Update(it->lipsyncId, it->pActor, it->pPickedLine))
			{
				it->endingConditions &= ~eEC_WaitingForLipsync;
			}
		}

		if (currentTime - it->finishTime >= 0)
		{
			it->endingConditions &= ~eEC_WaitingForTimer;
		}
		else if (gEnv->pEntitySystem && gEnv->pRenderer)
		{
			CResponseActor* pActor = it->pActor;
			if (pActor)
			{
				IEntity* pEntity = it->pEntity;
				if (pEntity)
				{
					if (it->speechAuxProxy != DEFAULT_AUDIO_PROXY_ID)
					{
						UpdateAudioProxyPosition(pEntity, *it);
					}

					if (m_displaySubtitlesCVar != 0)
					{
						if (gEnv->pGame && gEnv->pGame->GetIGameFramework() && gEnv->pGame->GetIGameFramework()->GetClientActorId() == pEntity->GetId())
						{
							IRenderAuxText::Draw2dLabel(8.0f, currentPos2Y, 2.0f, fColorBlue, false, "%s", it->text.c_str());
							currentPos2Y += 10.0f;
						}
						else
						{
							IRenderAuxText::DrawLabelEx(pEntity->GetWorldPos() + Vec3(0.0f, 0.0f, 2.0f), 2.0f, fColorBlue, true, true, "%s", it->text.c_str());
						}
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "An DRS-actor without an Entity detected: LineText: '%s', Actor: '%s'\n", (!it->text.empty()) ? it->text.c_str() : "no text", pActor->GetName().GetText().c_str());
				}
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "An active speaker without a DRS-Actor detected: LineText: '%s' \n", (!it->text.empty()) ? it->text.c_str() : "no text");
			}
		}
	}

	//remove finished speakers
	for (SpeakerList::iterator it = m_activeSpeakers.begin(); it != m_activeSpeakers.end(); )
	{
		if (it->endingConditions == eEC_Done)
		{
			if (m_pLipsyncProvider)
			{
				m_pLipsyncProvider->OnLineEnded(it->lipsyncId, it->pActor, it->pPickedLine);
			}

			if (!it->bWasCanceled)  //if the line was canceled we do not start any follow-up-lines
			{
				InformListener(it->pActor, it->lineID, IListener::eLineEvent_Finished, it->pPickedLine);

				CDialogLineDatabase* pLineDatabase = static_cast<CDialogLineDatabase*>(CResponseSystem::GetInstance()->GetDialogLineDatabase());
				CDialogLineSet* pLineSet = pLineDatabase->GetLineSetById(it->lineID);
				const CDialogLine* pFollowUpLine = (pLineSet) ? pLineSet->GetFollowUpLine(it->pPickedLine) : nullptr;
				if (pFollowUpLine) //so we are NOT done, because there is a follow-up-line for our just finished line
				{
					it->pPickedLine = pFollowUpLine;
					ExecuteStartSpeaking(&*it);
					continue;
				}
			}

			ReleaseSpeakerAudioProxy(*it, true);
			CVariableCollection* pLocalCollection = it->pActor->GetLocalVariables();
			pLocalCollection->SetVariableValue(s_isTalkingVariableName, false);
			pLocalCollection->SetVariableValue(s_lastLineFinishTime, CResponseSystem::GetInstance()->GetCurrentDrsTime());
			pLocalCollection->SetVariableValue(s_currentLinePriority, 0);
			bool bWasAlreeadyExisting = false;
			if (!m_queuedSpeakers.empty())
			{
				//(unique) add the just-finished speaker to our list of recently finished speakers, so that we can start queued lines a bit later on (first we give follow-up responses the change to start the next line of a sequential dialog
				for (std::pair<CResponseActor*, float> actorAndFinishTime : m_recentlyFinishedSpeakers)
				{
					if (actorAndFinishTime.first == it->pActor)
					{
						actorAndFinishTime.second = currentTime;
						bWasAlreeadyExisting = true;
						break;
					}
				}
				if (!bWasAlreeadyExisting)
				{
					m_recentlyFinishedSpeakers.push_back(std::pair<CResponseActor*, float>(it->pActor, currentTime));
				}
			}
			it = m_activeSpeakers.erase(it);
		}
		else
		{
			++it;
		}
	}

	//now we start all queued lines, that are ready to be started + we remove all queuedLines that reached their max queue-time
	if (!m_recentlyFinishedSpeakers.empty())
	{
		if (!m_queuedSpeakers.empty())
		{
			QueuedSpeakerList m_queuedSpeakersCopy = m_queuedSpeakers;
			for (auto itActorAndFinishTime = m_recentlyFinishedSpeakers.begin(); itActorAndFinishTime != m_recentlyFinishedSpeakers.end(); )
			{
				if (itActorAndFinishTime->second + s_delayBeforeExecutingQueuedLines < currentTime)
				{
					CResponseActor* pFinishedActor = itActorAndFinishTime->first;
					itActorAndFinishTime = m_recentlyFinishedSpeakers.erase(itActorAndFinishTime);

					for (auto itQueued = m_queuedSpeakersCopy.begin(); itQueued != m_queuedSpeakersCopy.end(); ++itQueued)
					{
						const SWaitingInfo& currentQueuedInfo = *itQueued;
						stl::find_and_erase(m_queuedSpeakers, *itQueued);  //we will now handle the queued-line, so we can already remove it from the list

						if (currentQueuedInfo.pActor == pFinishedActor)
						{
							if (currentTime > currentQueuedInfo.waitEndTime)
							{
								InformListener(currentQueuedInfo.pActor, currentQueuedInfo.lineID, IListener::eLineEvent_SkippedBecauseOfTimeOut, nullptr);
							}
							else
							{
								if (StartSpeaking(currentQueuedInfo.pActor, currentQueuedInfo.lineID))
								{
									break;  //we found and started a queued line, so we are done for this actor
								}
							}
						}
					}
				}
				else
				{
					++itActorAndFinishTime;
				}
			}
		}
		else
		{
			m_recentlyFinishedSpeakers.clear();
		}
	}
}

//--------------------------------------------------------------------------------------------------
bool CSpeakerManager::IsSpeaking(const DRS::IResponseActor* pActor, const CHashedString& lineID /* = CHashedString::GetEmpty() */, bool bCheckQueuedLinesAsWell /* = false */) const
{
	for (SpeakerList::const_iterator it = m_activeSpeakers.cbegin(), itEnd = m_activeSpeakers.cend(); it != itEnd; ++it)
	{
		if (it->pActor == pActor && (!lineID.IsValid() || it->lineID == lineID))
		{
			return true;
		}
	}

	if (bCheckQueuedLinesAsWell)
	{
		for (QueuedSpeakerList::const_iterator it = m_queuedSpeakers.cbegin(), itEnd = m_queuedSpeakers.cend(); it != itEnd; ++it)
		{
			if (it->pActor == pActor && (!lineID.IsValid() || it->lineID == lineID))
			{
				return true;
			}
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
DRS::ISpeakerManager::IListener::eLineEvent CSpeakerManager::StartSpeaking(DRS::IResponseActor* pIActor, const CHashedString& lineID)
{
	if (!pIActor)
	{
		InformListener(pIActor, lineID, IListener::eLineEvent_SkippedBecauseOfFaultyData, nullptr);
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "StartSpeaking was called without a valid DRS Actor.");
		return IListener::eLineEvent_SkippedBecauseOfFaultyData;
	}

	CResponseActor* pActor = static_cast<CResponseActor*>(pIActor);
	CDialogLineDatabase* pLineDatabase = static_cast<CDialogLineDatabase*>(CResponseSystem::GetInstance()->GetDialogLineDatabase());
	CDialogLineSet* pLineSet = pLineDatabase->GetLineSetById(lineID);
	if (pLineSet && !pLineSet->HasAvailableLines())
	{
		InformListener(pActor, lineID, IListener::eLineEvent_SkippedBecauseOfNoValidLineVariations, nullptr);
		return IListener::eLineEvent_SkippedBecauseOfNoValidLineVariations;
	}

	IEntity* pEntity = pActor->GetLinkedEntity();
	if (!pEntity)
	{
		InformListener(pActor, lineID, IListener::eLineEvent_SkippedBecauseOfFaultyData, nullptr);
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "StartSpeaking was called with a DRS Actor that has no Entity assigned to it.");
		return IListener::eLineEvent_SkippedBecauseOfFaultyData;
	}

	if (!OnLineAboutToStart(pIActor, lineID))
	{
		InformListener(pIActor, lineID, IListener::eLineEvent_SkippedBecauseOfExternalCode, nullptr);
		return IListener::eLineEvent_SkippedBecauseOfExternalCode;
	}

	const CDialogLine* pLine = nullptr;                             //we will check the priority first, before picking a line
	const int priority = (pLineSet) ? pLineSet->GetPriority() : 50; // 50 = default priority
	SSpeakInfo* pSpeakerInfoToUse = nullptr;

	//Special handling of just-finished-talking actors: if the actor just finished talking and there are queued lines waiting to start, we queue the new line as well, so that 'best' line can continue
	for (std::pair<CResponseActor*, float> actorAndFinishTime : m_recentlyFinishedSpeakers)
	{
		if (actorAndFinishTime.first == pActor)
		{
			for (QueuedSpeakerList::iterator itQueued = m_queuedSpeakers.begin(); itQueued != m_queuedSpeakers.end(); ++itQueued)
			{
				if (itQueued->pActor == pActor)
				{
					float maxQueueDuration = (pLineSet) ? pLineSet->GetMaxQueuingDuration() : m_defaultMaxQueueTime;
					if (maxQueueDuration < 0.0f)
						maxQueueDuration = m_defaultMaxQueueTime + s_delayBeforeExecutingQueuedLines;
					QueueLine(pActor, lineID, maxQueueDuration, priority + 1);  //+1 prio, to give this line a small boost, because it was started just after the previous line was done, so changes are good, that it belongs to it
					return IListener::eLineEvent_Queued;
				}
			}
		}
	}

	//check if the actor is already talking
	for (SSpeakInfo& activateSpeaker : m_activeSpeakers)
	{
		if (activateSpeaker.pActor == pActor)
		{
			if (activateSpeaker.bWasCanceled
			    || priority > activateSpeaker.priority
			    || (m_samePrioCancelsLinesCVar != 0 && priority >= activateSpeaker.priority && lineID != activateSpeaker.lineID)) //if the new line is not less important, stop the old one
			{
				const IEntityAudioProxyPtr pEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

				if (activateSpeaker.stopTriggerID != INVALID_AUDIO_CONTROL_ID && m_playAudioCVar != 0)
				{
					//soft interruption: we execute the stop trigger on the old line. That trigger should cause the old line to end after a while. And only then, do we start the playback of the next line
					pLine = (pLineSet) ? pLineSet->PickLine() : nullptr;
					SAudioCallBackInfo callbackInfo(this, (void* const)(pLine), (void* const)(activateSpeaker.pActor), eAudioRequestFlags_PriorityNormal | eAudioRequestFlags_SyncFinishedCallback);
					if (!pEntityAudioProxy->ExecuteTrigger(activateSpeaker.stopTriggerID, activateSpeaker.speechAuxProxy, callbackInfo))
					{
						//failed to start the stop trigger, therefore we fallback to hard-interruption by stopping the start trigger
						pEntityAudioProxy->StopTrigger(activateSpeaker.startTriggerID, activateSpeaker.speechAuxProxy);
					}
					else
					{
						float maxQueueDuration = (pLineSet) ? pLineSet->GetMaxQueuingDuration() : m_defaultMaxQueueTime;
						if (maxQueueDuration < 0.0f)
							maxQueueDuration = m_defaultMaxQueueTime;
						//so we have started the stop-trigger, now we have to wait for it to finish before we can start the new line.
						activateSpeaker.endingConditions |= eEC_WaitingForStopTrigger;
						activateSpeaker.priority = priority; //we set the priority of the ending-line to the new priority, so that the stopping wont be canceled by someone else.
						QueueLine(pActor, lineID, maxQueueDuration, priority);
						return IListener::eLineEvent_Queued;
					}
				}
				else
				{
					if (activateSpeaker.startTriggerID != INVALID_AUDIO_CONTROL_ID)
					{
						pEntityAudioProxy->StopTrigger(activateSpeaker.startTriggerID, activateSpeaker.speechAuxProxy);
					}
					if (!activateSpeaker.standaloneFile.empty())
					{
						pEntityAudioProxy->StopFile(activateSpeaker.standaloneFile.c_str(), activateSpeaker.speechAuxProxy);
					}
					InformListener(pActor, activateSpeaker.lineID, IListener::eLineEvent_Canceled, activateSpeaker.pPickedLine);
				}
				pSpeakerInfoToUse = &activateSpeaker;
				break;
			}
			else
			{
				float maxQueueDuration = (pLineSet) ? pLineSet->GetMaxQueuingDuration() : m_defaultMaxQueueTime;
				if (maxQueueDuration < 0.0f)
					maxQueueDuration = m_defaultMaxQueueTime;

				if (maxQueueDuration > 0.0f && lineID != activateSpeaker.lineID)
				{
					//so we wait for some time for the currently playing (and more important line) to finish
					QueueLine(pActor, lineID, maxQueueDuration, priority);
					return IListener::eLineEvent_Queued;
				}
				else
				{
					InformListener(pActor, lineID, IListener::eLineEvent_SkippedBecauseOfPriority, activateSpeaker.pPickedLine);
					return IListener::eLineEvent_SkippedBecauseOfPriority;
				}
			}
		}
	}

	if (!pSpeakerInfoToUse)  //are we reusing an existing speaker
	{
		m_activeSpeakers.push_back(SSpeakInfo());
		pSpeakerInfoToUse = &m_activeSpeakers.back();
		pSpeakerInfoToUse->speechAuxProxy = DEFAULT_AUDIO_PROXY_ID;
		pSpeakerInfoToUse->voiceAttachmentIndex = -1;  // -1 means invalid ID;
	}

	if (pLine)
	{
		pSpeakerInfoToUse->pPickedLine = pLine;
	}
	else
	{
		pSpeakerInfoToUse->pPickedLine = (pLineSet) ? pLineSet->PickLine() : nullptr;
	}
	pSpeakerInfoToUse->pActor = pActor;
	pSpeakerInfoToUse->pEntity = pEntity;
	pSpeakerInfoToUse->lineID = lineID;
	pSpeakerInfoToUse->priority = priority;
	pSpeakerInfoToUse->bWasCanceled = false;

	ExecuteStartSpeaking(pSpeakerInfoToUse);  //now we have all data we need in order to actual start speaking

	return ISpeakerManager::IListener::eLineEvent_Started;
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::UpdateAudioProxyPosition(IEntity* pEntity, const SSpeakInfo& speakerInfo)
{
	//todo: check if we can use a 'head' proxy created and defined in schematyc
	IEntityAudioProxy* const pEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
	if (pEntityAudioProxy)
	{
		const ICharacterInstance* pCharacter = pEntity->GetCharacter(s_characterSlot);
		if (pCharacter)
		{
			const IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
			if (pAttachmentManager)
			{
				const IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(speakerInfo.voiceAttachmentIndex);
				if (pAttachment)
				{
					pEntityAudioProxy->SetAuxAudioProxyOffset(Matrix34(pAttachment->GetAttModelRelative()), speakerInfo.speechAuxProxy);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
bool CSpeakerManager::CancelSpeaking(const DRS::IResponseActor* pActor, int maxPrioToCancel /* = -1 */, const CHashedString& lineID /* = CHashedString::GetEmpty() */, bool bCancelQueuedLines /* = true */)
{
	bool bSomethingWasCanceled = false;

	for (SSpeakInfo& speakerInfo : m_activeSpeakers)
	{
		if ((speakerInfo.pActor == pActor || pActor == nullptr)
		    && (speakerInfo.priority <= maxPrioToCancel || maxPrioToCancel < 0)
		    && (lineID == speakerInfo.lineID || !lineID.IsValid()))
		{
			//to-check: currently, cancel speaking will not execute the stop trigger
			InformListener(speakerInfo.pActor, speakerInfo.lineID, IListener::eLineEvent_Canceled, speakerInfo.pPickedLine);
			speakerInfo.endingConditions = eEC_Done;
			speakerInfo.finishTime = 0.0f;  //trigger finished, will be removed in the next update then
			speakerInfo.bWasCanceled = true;
			bSomethingWasCanceled = true;
			if (speakerInfo.lineID)
			{
				CDialogLineDatabase* pLineDatabase = static_cast<CDialogLineDatabase*>(CResponseSystem::GetInstance()->GetDialogLineDatabase());
				CDialogLineSet* pLineSet = pLineDatabase->GetLineSetById(speakerInfo.lineID);
				if (pLineSet)
				{
					pLineSet->OnLineCanceled(speakerInfo.pPickedLine); //we inform the lineSet, that the line was canceled, so it might decide to reset the was-spoken flag, that is used for the say-only-once option
				}
			}
		}
	}

	if (bCancelQueuedLines)
	{
		for (QueuedSpeakerList::iterator itQueued = m_queuedSpeakers.begin(); itQueued != m_queuedSpeakers.end(); )
		{
			if ((itQueued->pActor == pActor || pActor == nullptr)
			    && (itQueued->linePriority <= maxPrioToCancel || maxPrioToCancel < 0)
			    && (lineID == itQueued->lineID || !lineID.IsValid()))
			{
				InformListener(itQueued->pActor, itQueued->lineID, IListener::eLineEvent_Canceled, nullptr);
				itQueued = m_queuedSpeakers.erase(itQueued);
				bSomethingWasCanceled = true;
			}
			else
			{
				++itQueued;
			}
		}
	}

	return bSomethingWasCanceled;
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::Reset()
{
	m_queuedSpeakers.clear();

	for (SSpeakInfo& speakerInfo : m_activeSpeakers)
	{
		SET_DRS_USER_SCOPED("SpeakerManager Reset");
		InformListener(speakerInfo.pActor, speakerInfo.lineID, IListener::eLineEvent_Canceled, speakerInfo.pPickedLine);
		CVariableCollection* pLocalCollection = speakerInfo.pActor->GetLocalVariables();
		pLocalCollection->SetVariableValue(s_isTalkingVariableName, false);
		pLocalCollection->SetVariableValue(s_lastLineFinishTime, CResponseSystem::GetInstance()->GetCurrentDrsTime());
		pLocalCollection->SetVariableValue(s_currentLinePriority, 0);
		ReleaseSpeakerAudioProxy(speakerInfo, true);
	}
	m_activeSpeakers.clear();
	m_recentlyFinishedSpeakers.clear();
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::ReleaseSpeakerAudioProxy(SSpeakInfo& speakerInfo, bool stopTrigger)
{
	if (speakerInfo.speechAuxProxy != DEFAULT_AUDIO_PROXY_ID || stopTrigger)
	{
		IEntity* pEntity = speakerInfo.pActor->GetLinkedEntity();
		if (pEntity)
		{
			IEntityAudioProxy* const pEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
			if (pEntityAudioProxy)
			{
				if (stopTrigger)
				{
					if (speakerInfo.startTriggerID != INVALID_AUDIO_CONTROL_ID)
					{
						pEntityAudioProxy->StopTrigger(speakerInfo.startTriggerID, speakerInfo.speechAuxProxy);
					}
					if (!speakerInfo.standaloneFile.empty())
					{
						pEntityAudioProxy->StopFile(speakerInfo.standaloneFile, speakerInfo.speechAuxProxy);
					}
				}
				if (speakerInfo.speechAuxProxy != DEFAULT_AUDIO_PROXY_ID)
				{
					pEntityAudioProxy->RemoveAuxAudioProxy(speakerInfo.speechAuxProxy);
					speakerInfo.speechAuxProxy = DEFAULT_AUDIO_PROXY_ID;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::OnAudioCallback(const SAudioRequestInfo* const pAudioRequestInfo)
{
	CSpeakerManager* pSpeakerManager = CResponseSystem::GetInstance()->GetSpeakerManager();
	const CDialogLine* pDialogLine = reinterpret_cast<const CDialogLine*>(pAudioRequestInfo->pUserData);
	const CResponseActor* pActor = reinterpret_cast<const CResponseActor*>(pAudioRequestInfo->pUserDataOwner);

	if (pAudioRequestInfo->requestResult == eAudioRequestResult_Failure &&
	    (pAudioRequestInfo->audioRequestType == eAudioRequestType_AudioObjectRequest && (pAudioRequestInfo->specificAudioRequest == eAudioObjectRequestType_ExecuteTrigger || pAudioRequestInfo->specificAudioRequest == eAudioObjectRequestType_PlayFile)
	     || pAudioRequestInfo->audioRequestType == eAudioRequestType_AudioCallbackManagerRequest && pAudioRequestInfo->specificAudioRequest == eAudioCallbackManagerRequestType_ReportStartedFile))
	{
		//handling of failure executing the start / stop trigger or the standalone file
		for (SSpeakInfo& speakerInfo : pSpeakerManager->m_activeSpeakers)
		{
			if (speakerInfo.pActor == pActor && pDialogLine == speakerInfo.pPickedLine)
			{
				if (pAudioRequestInfo->audioControlId == speakerInfo.startTriggerID)  //if the start trigger fails, we still want to display the subtitle for some time, //will also be met for StandaloneFiles, because startTriggerID == 0
				{
					int textLength = (pDialogLine) ? strlen(pDialogLine->GetText()) : 16;
					float pauseLength = (speakerInfo.pPickedLine->GetPauseLength() < 0.0f) ? s_defaultPauseAfterLines : speakerInfo.pPickedLine->GetPauseLength();
					speakerInfo.finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + (2.0f + (textLength / 16.0f)) + pauseLength;
					speakerInfo.endingConditions |= eEC_WaitingForTimer;
					speakerInfo.endingConditions &= ~eEC_WaitingForStartTrigger;
				}
				else if (pAudioRequestInfo->audioControlId == speakerInfo.stopTriggerID) //if the stop trigger fails, we simply stop the start-trigger directly (in the next update)
				{
					speakerInfo.finishTime = 0.0f;
					speakerInfo.endingConditions = eEC_Done;
				}
				return;
			}
		}
	}
	else if (pAudioRequestInfo->audioRequestType == eAudioRequestType_AudioCallbackManagerRequest
	         && (pAudioRequestInfo->specificAudioRequest == eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance
	             || pAudioRequestInfo->specificAudioRequest == eAudioCallbackManagerRequestType_ReportStoppedFile))
	{
		for (SSpeakInfo& speakerInfo : pSpeakerManager->m_activeSpeakers)
		{
			if (speakerInfo.pActor == pActor && pDialogLine == speakerInfo.pPickedLine)
			{
				if (pAudioRequestInfo->audioControlId == speakerInfo.startTriggerID)  //will also be met for StandaloneFiles, because startTriggerID == 0
				{
					float pauseLength = (speakerInfo.pPickedLine->GetPauseLength() < 0.0f) ? s_defaultPauseAfterLines : speakerInfo.pPickedLine->GetPauseLength();
					speakerInfo.endingConditions &= ~eEC_WaitingForStartTrigger;
					if (pauseLength > 0.0f && (speakerInfo.endingConditions & eEC_WaitingForTimer) == 0)
					{
						//audio playback is done, time for the artificial pause, specified by the 'pause' property of the line
						speakerInfo.finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + pauseLength;
						speakerInfo.endingConditions |= eEC_WaitingForTimer;
					}
				}
				if (pAudioRequestInfo->audioControlId == speakerInfo.stopTriggerID)
				{
					speakerInfo.endingConditions &= ~eEC_WaitingForStopTrigger;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::OnActorRemoved(const CResponseActor* pActor)
{
	for (SpeakerList::iterator it = m_activeSpeakers.begin(); it != m_activeSpeakers.end(); )
	{
		if (it->pActor == pActor)
		{
			InformListener(pActor, it->lineID, IListener::eLineEvent_Canceled, it->pPickedLine);
			it = m_activeSpeakers.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (QueuedSpeakerList::iterator itQueued = m_queuedSpeakers.begin(); itQueued != m_queuedSpeakers.end(); )
	{
		if (itQueued->pActor == pActor)
		{
			InformListener(itQueued->pActor, itQueued->lineID, IListener::eLineEvent_Canceled, nullptr);
			itQueued = m_queuedSpeakers.erase(itQueued);
		}
		else
		{
			++itQueued;
		}
	}

	for (std::vector<std::pair<CResponseActor*, float>>::iterator itRecent = m_recentlyFinishedSpeakers.begin(); itRecent != m_recentlyFinishedSpeakers.end(); )
	{
		if (itRecent->first == pActor)
		{
			itRecent = m_recentlyFinishedSpeakers.erase(itRecent);
		}
		else
		{
			++itRecent;
		}
	}
}

//--------------------------------------------------------------------------------------------------
bool CSpeakerManager::AddListener(IListener* pListener)
{
	return m_listeners.Add(pListener);
}

//--------------------------------------------------------------------------------------------------
bool CSpeakerManager::RemoveListener(IListener* pListener)
{
	m_listeners.Remove(pListener);
	return true;
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::InformListener(const DRS::IResponseActor* pSpeaker, const CHashedString& lineID, IListener::eLineEvent event, const CDialogLine* pLine)
{
	const DRS::IDialogLine* pILine = static_cast<const DRS::IDialogLine*>(pLine);
	m_listeners.ForEachListener([=](IListener* pListener)
	{
		pListener->OnLineEvent(pSpeaker, lineID, event, pILine);
	});
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::SetCustomLipsyncProvider(ILipsyncProvider* pProvider)
{
	delete(m_pDefaultLipsyncProvider);  //we dont need the default one anymore, once a custom one is set.
	m_pDefaultLipsyncProvider = nullptr;

	m_pLipsyncProvider = pProvider;
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::SetNumActiveSpeaker(int newAmountOfSpeaker)
{
	if (m_numActiveSpeaker != newAmountOfSpeaker)
	{
		m_numActiveSpeaker = newAmountOfSpeaker;
		CVariableCollection* pGlobalVariableCollection = CResponseSystem::GetInstance()->GetCollection("Global");
		CVariable* pActiveSpeakerVariable = pGlobalVariableCollection->CreateOrGetVariable("ActiveSpeakers");
		pActiveSpeakerVariable->SetValue(newAmountOfSpeaker);

		if (m_audioRtpcIdGlobal != INVALID_AUDIO_CONTROL_ID)
		{
			SAudioRequest request;
			SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(m_audioRtpcIdGlobal, (float)newAmountOfSpeaker);
			request.flags = eAudioRequestFlags_PriorityNormal;
			request.pData = &requestData;
			gEnv->pAudioSystem->PushRequest(request);
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CSpeakerManager::ExecuteStartSpeaking(SSpeakInfo* pSpeakerInfoToUse)
{
	pSpeakerInfoToUse->startTriggerID = INVALID_AUDIO_CONTROL_ID;
	pSpeakerInfoToUse->stopTriggerID = INVALID_AUDIO_CONTROL_ID;
	pSpeakerInfoToUse->endingConditions = eEC_Done;
	pSpeakerInfoToUse->lipsyncId = DRS::s_InvalidLipSyncId;
	pSpeakerInfoToUse->standaloneFile.clear();

	if (pSpeakerInfoToUse->pPickedLine)
	{
		pSpeakerInfoToUse->text = pSpeakerInfoToUse->pPickedLine->GetText();
		if (!pSpeakerInfoToUse->pPickedLine->GetStartAudioTrigger().empty())
		{
			gEnv->pAudioSystem->GetAudioTriggerId(pSpeakerInfoToUse->pPickedLine->GetStartAudioTrigger().c_str(), pSpeakerInfoToUse->startTriggerID);
		}

		pSpeakerInfoToUse->standaloneFile = pSpeakerInfoToUse->pPickedLine->GetStandaloneFile();
		if (!pSpeakerInfoToUse->pPickedLine->GetEndAudioTrigger().empty())
		{
			gEnv->pAudioSystem->GetAudioTriggerId(pSpeakerInfoToUse->pPickedLine->GetEndAudioTrigger().c_str(), pSpeakerInfoToUse->stopTriggerID);
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Triggered an SpeakLineAction, with a line that did not exist in the database! LineID was: '%s'", pSpeakerInfoToUse->lineID.GetText().c_str());
		pSpeakerInfoToUse->text.Format("Missing: %s", pSpeakerInfoToUse->lineID.GetText().c_str());
	}

	bool bHasAudioAsset = (pSpeakerInfoToUse->startTriggerID != INVALID_AUDIO_CONTROL_ID || !pSpeakerInfoToUse->standaloneFile.empty()) && m_playAudioCVar != 0;

	if (bHasAudioAsset)
	{
		const IEntityAudioProxyPtr pEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pSpeakerInfoToUse->pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

		if (m_audioRtpcIdLocal != INVALID_AUDIO_CONTROL_ID)
		{
			pEntityAudioProxy->SetRtpcValue(m_audioRtpcIdLocal, 1.0f, INVALID_AUDIO_PROXY_ID);
		}

		if (pSpeakerInfoToUse->speechAuxProxy == DEFAULT_AUDIO_PROXY_ID)
		{
			const ICharacterInstance* pCharacter = pSpeakerInfoToUse->pEntity->GetCharacter(s_characterSlot);
			if (pCharacter)
			{
				const IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
				if (pAttachmentManager)
				{
					pSpeakerInfoToUse->voiceAttachmentIndex = pAttachmentManager->GetIndexByNameCRC(s_attachmentPosName);
					if (pSpeakerInfoToUse->voiceAttachmentIndex == -1)
						pSpeakerInfoToUse->voiceAttachmentIndex = pAttachmentManager->GetIndexByNameCRC(s_attachmentPosNameFallback);
					if (pSpeakerInfoToUse->voiceAttachmentIndex != -1)
					{
						pSpeakerInfoToUse->speechAuxProxy = pEntityAudioProxy->CreateAuxAudioProxy();
						UpdateAudioProxyPosition(pSpeakerInfoToUse->pEntity, *pSpeakerInfoToUse);
					}
				}
			}
		}

		SAudioCallBackInfo callbackInfo(this, (void* const)(pSpeakerInfoToUse->pPickedLine), (void* const)(pSpeakerInfoToUse->pActor), eAudioRequestFlags_PriorityNormal | eAudioRequestFlags_SyncFinishedCallback);

		bool bAudioPlaybackStarted = true;

		if (pSpeakerInfoToUse->startTriggerID)
		{
			bAudioPlaybackStarted = pEntityAudioProxy->ExecuteTrigger(pSpeakerInfoToUse->startTriggerID, pSpeakerInfoToUse->speechAuxProxy, callbackInfo);
		}
		else
		{
			bAudioPlaybackStarted = pEntityAudioProxy->PlayFile(pSpeakerInfoToUse->standaloneFile.c_str(), pSpeakerInfoToUse->speechAuxProxy, callbackInfo);
		}
		if (bAudioPlaybackStarted)
		{
			pSpeakerInfoToUse->finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + 100.0f;
			pSpeakerInfoToUse->endingConditions = eEC_WaitingForStartTrigger;
		}
		else
		{
			ReleaseSpeakerAudioProxy(*pSpeakerInfoToUse, false);
			pSpeakerInfoToUse->startTriggerID = INVALID_AUDIO_CONTROL_ID;
			pSpeakerInfoToUse->stopTriggerID = INVALID_AUDIO_CONTROL_ID;
			pSpeakerInfoToUse->speechAuxProxy = DEFAULT_AUDIO_PROXY_ID;
			pSpeakerInfoToUse->standaloneFile.clear();
		}
	}

	if (m_pLipsyncProvider)
	{
		pSpeakerInfoToUse->lipsyncId = m_pLipsyncProvider->OnLineStarted(pSpeakerInfoToUse->pActor, pSpeakerInfoToUse->pPickedLine);
		if (pSpeakerInfoToUse->lipsyncId != DRS::s_InvalidLipSyncId)
		{
			pSpeakerInfoToUse->endingConditions |= eEC_WaitingForLipsync;
			pSpeakerInfoToUse->finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + 100.0f;
		}
	}

	if (pSpeakerInfoToUse->endingConditions == eEC_Done)  //fallback, if no other ending conditions were applied, display subtitles for some time
	{
		pSpeakerInfoToUse->finishTime = CResponseSystem::GetInstance()->GetCurrentDrsTime();
		pSpeakerInfoToUse->finishTime += (2.0f + pSpeakerInfoToUse->text.length() / 16.0f);
		if (pSpeakerInfoToUse->pPickedLine)
		{
			float pauseLength = (pSpeakerInfoToUse->pPickedLine->GetPauseLength() < 0.0f) ? s_defaultPauseAfterLines : pSpeakerInfoToUse->pPickedLine->GetPauseLength();
			pSpeakerInfoToUse->finishTime += pauseLength;
		}
		pSpeakerInfoToUse->endingConditions = eEC_WaitingForTimer;
	}

	InformListener(pSpeakerInfoToUse->pActor, pSpeakerInfoToUse->lineID, IListener::eLineEvent_Started, pSpeakerInfoToUse->pPickedLine);

	SET_DRS_USER_SCOPED("SpeakerManager Start Speaking");
	CVariableCollection* pLocalCollection = pSpeakerInfoToUse->pActor->GetLocalVariables();
	pLocalCollection->SetVariableValue(s_isTalkingVariableName, true);
	pLocalCollection->SetVariableValue(s_lastLineId, pSpeakerInfoToUse->lineID);
	float pointInFarFuture = (((int)CResponseSystem::GetInstance()->GetCurrentDrsTime() % 5000) + 1) * 5000.0f; //we set the 'LastFinishTime' to a point in the far future, so that conditions like 'if TimeSince LastFinishTime > 3' (to detect longer pauses) will savely work.
	pLocalCollection->SetVariableValue(s_lastLineFinishTime, pointInFarFuture);
	pLocalCollection->SetVariableValue(s_currentLinePriority, pSpeakerInfoToUse->priority);
	SetNumActiveSpeaker((int)m_activeSpeakers.size());
}

//--------------------------------------------------------------------------------------------------
bool CSpeakerManager::OnLineAboutToStart(const DRS::IResponseActor* pSpeaker, const CHashedString& lineID)
{
	bool bLineCanBeStarted = true;

	m_listeners.ForEachListener([pSpeaker, lineID, &bLineCanBeStarted](IListener* pListener)
	{
		bLineCanBeStarted = bLineCanBeStarted & pListener->OnLineAboutToStart(pSpeaker, lineID);
	});

	return bLineCanBeStarted;
}

//--------------------------------------------------------------------------------------------------
void CryDRS::CSpeakerManager::QueueLine(CResponseActor* pActor, const CHashedString& lineID, float maxQueueDuration, const int priority)
{
	SWaitingInfo newWaitInfo;
	newWaitInfo.pActor = pActor;
	newWaitInfo.lineID = lineID;
	newWaitInfo.waitEndTime = CResponseSystem::GetInstance()->GetCurrentDrsTime() + maxQueueDuration;

	auto existingPos = std::find(m_queuedSpeakers.begin(), m_queuedSpeakers.end(), newWaitInfo);
	if (existingPos != m_queuedSpeakers.end())
	{
		existingPos->waitEndTime = newWaitInfo.waitEndTime;
	}
	else
	{
		newWaitInfo.linePriority = priority;
		m_queuedSpeakers.push_back(newWaitInfo);
	}

	std::sort(m_queuedSpeakers.begin(), m_queuedSpeakers.end(), [](const SWaitingInfo& a, const SWaitingInfo& b)
	{
		if (a.linePriority == b.linePriority)
		{
		  return a.waitEndTime < b.waitEndTime; //if the priority is the same we prefer the line that can not be queued not as long
		}
		return a.linePriority > b.linePriority;
	});

	InformListener(pActor, lineID, IListener::eLineEvent_Queued, nullptr);
}

//--------------------------------------------------------------------------------------------------
DRS::LipSyncID CDefaultLipsyncProvider::OnLineStarted(DRS::IResponseActor* pActor, const DRS::IDialogLine* pDialogLine)
{
	//Start lipsync animation
	int animationID = -1;
	if (pDialogLine)
	{
		if (!pDialogLine->GetLipsyncAnimation().empty())
		{
			IEntity* pEntity = pActor->GetLinkedEntity();
			if (pEntity)
			{
				ICharacterInstance* pCharacter = pEntity->GetCharacter(s_characterSlot);
				if (pCharacter)
				{
					const IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
					if (pAnimSet)
					{
						CryCharAnimationParams params;
						params.m_fTransTime = m_lipsyncTransitionTime;
						params.m_nLayerID = m_lipsyncAnimationLayer;
						params.m_nUserToken = pActor->GetName().GetHash();
						params.m_nFlags = CA_ALLOW_ANIM_RESTART;

						animationID = pAnimSet->GetAnimIDByName(pDialogLine->GetLipsyncAnimation().c_str());

						if (animationID < 0)                                                                    //fallback 1: use audio trigger name as animation name
						{
							animationID = pAnimSet->GetAnimIDByName(pDialogLine->GetStartAudioTrigger().c_str()); //default: if no special line is provided, we look for a animation with the same name as the audio trigger
						}
						if (animationID < 0)                                                                    //fallback 2: use default animation
						{
							animationID = pAnimSet->GetAnimIDByName(m_defaultLipsyncAnimationName);
							if (animationID >= 0)
							{
								params.m_nFlags |= CA_LOOP_ANIMATION;
								params.m_fPlaybackSpeed = 1.25f;
							}
						}

						if (animationID >= 0)
						{
							ISkeletonAnim* pSkeletonAnimation = pCharacter->GetISkeletonAnim();
							if (!pSkeletonAnimation->StartAnimationById(animationID, params))
							{
								animationID = -1;
							}
						}
					}
				}
			}
		}
	}
	return (animationID < 0) ? DRS::s_InvalidLipSyncId : static_cast<DRS::LipSyncID>(animationID);
}

//--------------------------------------------------------------------------------------------------
void CDefaultLipsyncProvider::OnLineEnded(DRS::LipSyncID lipsyncId, DRS::IResponseActor* pSpeaker, const DRS::IDialogLine* pLine)
{
	//stop lipsync animation
	if (lipsyncId != DRS::s_InvalidLipSyncId)
	{
		IEntity* pEntity = pSpeaker->GetLinkedEntity();
		if (pEntity)
		{
			ICharacterInstance* pCharacter = pEntity->GetCharacter(s_characterSlot);
			if (pCharacter)
			{
				ISkeletonAnim* pSkeletonAnimation = pCharacter->GetISkeletonAnim();
				if (pSkeletonAnimation)
				{
					// NOTE: there is no simple way to just stop the exact animation we started, but this should do too:
					pSkeletonAnimation->StopAnimationInLayer(m_lipsyncAnimationLayer, m_lipsyncTransitionTime);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
CDefaultLipsyncProvider::CDefaultLipsyncProvider()
{
	ICVar* pVariable = REGISTER_STRING("drs_dialogsDefaultTalkAnimation", "relaxed_tac_leaningOverRailEnd_nw_3p_01", 0, "The fallback talk animation, if no specialized talk animation is set for a line");
	if (pVariable)
		m_defaultLipsyncAnimationName = pVariable->GetString();

	REGISTER_CVAR2("drs_dialogsLipsyncAnimationLayer", &m_lipsyncAnimationLayer, 11, VF_NULL, "The animation layer used for dialog lipsync animations");
	REGISTER_CVAR2("drs_dialogsLipsyncTransitionTime", &m_lipsyncTransitionTime, 0.25, VF_NULL, "The blend in/out times for lipsync animations");
}

//--------------------------------------------------------------------------------------------------
CDefaultLipsyncProvider::~CDefaultLipsyncProvider()
{
	gEnv->pConsole->UnregisterVariable("drs_dialogsLipsyncAnimationLayer", true);
	gEnv->pConsole->UnregisterVariable("drs_dialogsLipsyncTransitionTime", true);
}

//--------------------------------------------------------------------------------------------------
bool CDefaultLipsyncProvider::Update(DRS::LipSyncID lipsyncId, DRS::IResponseActor* pSpeaker, const DRS::IDialogLine* pLine)
{
	//check if animation has finished, or when its a looping animation always returns false (we dont want the speakermanager wait for it to finish)
	if (lipsyncId != DRS::s_InvalidLipSyncId)
	{
		IEntity* pEntity = pSpeaker->GetLinkedEntity();
		if (pEntity)
		{
			ICharacterInstance* pCharacter = pEntity->GetCharacter(s_characterSlot);
			if (pCharacter)
			{
				ISkeletonAnim* pSkeletonAnimation = pCharacter->GetISkeletonAnim();
				if (pSkeletonAnimation)
				{
					const CAnimation* pLipSyncAni = pSkeletonAnimation->FindAnimInFIFO(pSpeaker->GetName().GetHash(), m_lipsyncAnimationLayer);
					return pLipSyncAni && !pLipSyncAni->HasStaticFlag(CA_LOOP_ANIMATION);
				}
			}
		}
	}
	return false;
}
