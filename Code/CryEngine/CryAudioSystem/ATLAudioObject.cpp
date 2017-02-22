// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "AudioEventManager.h"
#include "AudioSystem.h"
#include "AudioStandaloneFileManager.h"
#include <CryString/HashedString.h>
#include <CryEntitySystem/IEntitySystem.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

using namespace CryAudio;
using namespace CryAudio::Impl;

TriggerInstanceId CryAudio::CATLAudioObject::s_triggerInstanceIdCounter = 1;
CSystem* CryAudio::CATLAudioObject::s_pAudioSystem = nullptr;
CAudioEventManager* CryAudio::CATLAudioObject::s_pEventManager = nullptr;
CAudioStandaloneFileManager* CryAudio::CATLAudioObject::s_pStandaloneFileManager = nullptr;

ControlId CryAudio::CATLAudioObject::s_occlusionTypeSwitchId = InvalidControlId;
SwitchStateId CryAudio::CATLAudioObject::s_occlusionTypeStateIds[eOcclusionType_Count] = {
	InvalidSwitchStateId,
	InvalidSwitchStateId,
	InvalidSwitchStateId,
	InvalidSwitchStateId,
	InvalidSwitchStateId,
	InvalidSwitchStateId
};

//////////////////////////////////////////////////////////////////////////
CATLAudioObject::CATLAudioObject(Impl::IAudioObject* const pImplData, Vec3 const& audioListenerPosition)
	: m_pImplData(pImplData)
	, m_propagationProcessor(m_attributes.transformation, audioListenerPosition)
{
	m_propagationProcessor.Init(this);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Release()
{
	m_activeEvents.clear();
	m_triggerImplStates.clear();
	m_activeStandaloneFiles.clear();

	for (auto& triggerStatesPair : m_triggerStates)
	{
		triggerStatesPair.second.numLoadingEvents = 0;
		triggerStatesPair.second.numPlayingEvents = 0;
	}

	m_pImplData = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartingTriggerInstance(TriggerInstanceId const audioTriggerInstanceId, ControlId const audioTriggerId)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = m_triggerStates.emplace(audioTriggerInstanceId, SAudioTriggerInstanceState()).first->second;
	audioTriggerInstanceState.audioTriggerId = audioTriggerId;
	audioTriggerInstanceState.flags |= eAudioTriggerStatus_Starting;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedTriggerInstance(
  TriggerInstanceId const audioTriggerInstanceId,
  void* const pOwnerOverride,
  void* const pUserData,
  void* const pUserDataOwner,
  EnumFlagsType const flags)
{
	ObjectTriggerStates::iterator iter(m_triggerStates.find(audioTriggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;

		if (audioTriggerInstanceState.numPlayingEvents > 0 || audioTriggerInstanceState.numLoadingEvents > 0)
		{
			audioTriggerInstanceState.pOwnerOverride = pOwnerOverride;
			audioTriggerInstanceState.pUserData = pUserData;
			audioTriggerInstanceState.pUserDataOwner = pUserDataOwner;
			audioTriggerInstanceState.flags &= ~eAudioTriggerStatus_Starting;
			audioTriggerInstanceState.flags |= eAudioTriggerStatus_Playing;

			if ((flags & eRequestFlags_SyncFinishedCallback) == 0)
			{
				audioTriggerInstanceState.flags |= eAudioTriggerStatus_CallbackOnAudioThread;
			}
		}
		else
		{
			// All of the events have either finished before we got here or never started.
			// So we report this trigger as finished immediately.
			ReportFinishedTriggerInstance(iter);
		}
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Reported a started instance %u that couldn't be found on an object", audioTriggerInstanceId);
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedEvent(CATLEvent* const _pEvent)
{
	m_activeEvents.insert(_pEvent);
	m_triggerImplStates.insert(std::make_pair(_pEvent->m_audioTriggerImplId, SAudioTriggerImplState()));

	// Update the radius where events playing in this audio object are audible
	if (_pEvent->m_pTrigger)
	{
		m_maxRadius = std::max(_pEvent->m_pTrigger->m_maxRadius, m_maxRadius);
		m_occlusionFadeOutDistance = std::max(_pEvent->m_pTrigger->m_occlusionFadeOutDistance, m_occlusionFadeOutDistance);
	}

	ObjectTriggerStates::iterator const iter(m_triggerStates.find(_pEvent->m_audioTriggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;

		switch (_pEvent->m_audioEventState)
		{
		case eAudioEventState_Playing:
			{
				++(audioTriggerInstanceState.numPlayingEvents);

				break;
			}
		case eAudioEventState_PlayingDelayed:
			{
				CRY_ASSERT(audioTriggerInstanceState.numLoadingEvents > 0);
				--(audioTriggerInstanceState.numLoadingEvents);
				++(audioTriggerInstanceState.numPlayingEvents);
				_pEvent->m_audioEventState = eAudioEventState_Playing;

				break;
			}
		case eAudioEventState_Loading:
			{
				++(audioTriggerInstanceState.numLoadingEvents);

				break;
			}
		case eAudioEventState_Unloading:
			{
				// not handled currently
				break;
			}
		default:
			{
				// unknown event state!
				CRY_ASSERT(false);

				break;
			}
		}
	}
	else
	{
		// must exist!
		CRY_ASSERT(false);
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedEvent(CATLEvent* const _pEvent, bool const _bSuccess)
{
	m_activeEvents.erase(_pEvent);
	m_triggerImplStates.erase(_pEvent->m_audioTriggerImplId);

	// recalculate the max radius of the audio object
	m_maxRadius = 0.0f;
	m_occlusionFadeOutDistance = 0.0f;
	for (auto const pEvent : m_activeEvents)
	{
		if (pEvent->m_pTrigger)
		{
			m_maxRadius = std::max(pEvent->m_pTrigger->m_maxRadius, m_maxRadius);
			m_occlusionFadeOutDistance = std::max(pEvent->m_pTrigger->m_occlusionFadeOutDistance, m_occlusionFadeOutDistance);
		}
	}

	ObjectTriggerStates::iterator iter(m_triggerStates.begin());

	if (FindPlace(m_triggerStates, _pEvent->m_audioTriggerInstanceId, iter))
	{
		switch (_pEvent->m_audioEventState)
		{
		case eAudioEventState_Playing:
			{
				SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
				CRY_ASSERT(audioTriggerInstanceState.numPlayingEvents > 0);

				if (--(audioTriggerInstanceState.numPlayingEvents) == 0 &&
				    audioTriggerInstanceState.numLoadingEvents == 0 &&
				    (audioTriggerInstanceState.flags & eAudioTriggerStatus_Starting) == 0)
				{
					ReportFinishedTriggerInstance(iter);
				}

				break;
			}
		case eAudioEventState_Loading:
			{
				if (_bSuccess)
				{
					ReportFinishedLoadingTriggerImpl(_pEvent->m_audioTriggerImplId, true);
				}

				break;
			}
		case eAudioEventState_Unloading:
			{
				if (_bSuccess)
				{
					ReportFinishedLoadingTriggerImpl(_pEvent->m_audioTriggerImplId, false);
				}

				break;
			}
		default:
			{
				// unknown event state!
				CRY_ASSERT(false);

				break;
			}
		}
	}
	else
	{
		if (_pEvent->m_pTrigger != nullptr)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			g_audioLogger.Log(eAudioLogType_Warning, "Reported finished event on an inactive trigger %s", _pEvent->m_pTrigger->m_name.c_str());
#else
			g_audioLogger.Log(eAudioLogType_Warning, "Reported finished event on an inactive trigger %u", _pEvent->m_pTrigger->GetId());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
		}
		else
		{
			g_audioLogger.Log(eAudioLogType_Warning, "Reported finished event on a trigger that does not exist anymore");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request)
{
	ObjectStandaloneFileMap::const_iterator const iter(m_activeStandaloneFiles.find(pStandaloneFile));

	if (iter != m_activeStandaloneFiles.end())
	{
		SUserDataBase const& audioStandaloneFileData = iter->second;
		request.pOwner = audioStandaloneFileData.pOwnerOverride;
		request.pUserData = audioStandaloneFileData.pUserData;
		request.pUserDataOwner = audioStandaloneFileData.pUserDataOwner;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedStandaloneFile(
  CATLStandaloneFile* const pStandaloneFile,
  void* const pOwner,
  void* const pUserData,
  void* const pUserDataOwner)
{
	m_activeStandaloneFiles.emplace(pStandaloneFile, SUserDataBase(pOwner, pUserData, pUserDataOwner));
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	m_activeStandaloneFiles.erase(pStandaloneFile);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedLoadingTriggerImpl(TriggerImplId const audioTriggerImplId, bool const bLoad)
{
	if (bLoad)
	{
		m_triggerImplStates[audioTriggerImplId].flags |= eAudioTriggerStatus_Loaded;
	}
	else
	{
		m_triggerImplStates[audioTriggerImplId].flags &= ~eAudioTriggerStatus_Loaded;
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleExecuteTrigger(
  CATLTrigger const* const pTrigger,
  void* const pOwner,
  void* const pUserData,
  void* const pUserDataOwner,
  EnumFlagsType const flags)
{
	ERequestStatus result = eRequestStatus_Failure;

	// Sets eAudioTriggerStatus_Starting on this TriggerInstance to avoid
	// reporting TriggerFinished while the events are being started.
	ReportStartingTriggerInstance(s_triggerInstanceIdCounter, pTrigger->GetId());

	for (auto const pTriggerImpl : pTrigger->m_implPtrs)
	{
		CATLEvent* const pEvent = s_pEventManager->ConstructAudioEvent();
		ERequestStatus activateResult = m_pImplData->ExecuteTrigger(pTriggerImpl->m_pImplData, pEvent->m_pImplData);

		if (activateResult == eRequestStatus_Success || activateResult == eRequestStatus_Pending)
		{
			pEvent->m_pAudioObject = this;
			pEvent->m_pTrigger = pTrigger;
			pEvent->m_audioTriggerImplId = pTriggerImpl->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = s_triggerInstanceIdCounter;
			pEvent->SetDataScope(pTrigger->GetDataScope());

			if (activateResult == eRequestStatus_Success)
			{
				pEvent->m_audioEventState = eAudioEventState_Playing;
			}
			else if (activateResult == eRequestStatus_Pending)
			{
				pEvent->m_audioEventState = eAudioEventState_Loading;
			}

			ReportStartedEvent(pEvent);

			// If at least one event fires, it is a success: the trigger has been activated.
			result = eRequestStatus_Success;
		}
		else
		{
			s_pEventManager->ReleaseAudioEvent(pEvent);
		}
	}

	// Either removes the eAudioTriggerStatus_Starting flag on this trigger instance or removes it if no event was started.
	ReportStartedTriggerInstance(s_triggerInstanceIdCounter++, pOwner, pUserData, pUserDataOwner, flags);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (result != eRequestStatus_Success)
	{
		// No TriggerImpl generated an active event.
		g_audioLogger.Log(eAudioLogType_Warning, "Trigger \"%s\" failed on AudioObject \"%s\"", pTrigger->m_name.c_str(), m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleStopTrigger(CATLTrigger const* const pTrigger)
{
	ERequestStatus result = eRequestStatus_Failure;
	for (auto const pEvent : m_activeEvents)
	{
		if ((pEvent != nullptr) && pEvent->IsPlaying() && (pEvent->m_pTrigger == pTrigger))
		{
			result = pEvent->Stop();
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleSetSwitchState(CATLSwitch const* const pSwitch, CATLSwitchState const* const pState)
{
	ERequestStatus result = eRequestStatus_Failure;

	for (auto const pSwitchStateImpl : pState->m_implPtrs)
	{
		result = pSwitchStateImpl->Set(*this);
	}

	if (result == eRequestStatus_Success)
	{
		m_switchStates[pSwitch->GetId()] = pState->GetId();
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to set the ATLSwitch \"%s\" to ATLSwitchState \"%s\" on AudioObject \"%s\"", pSwitch->m_name.c_str(), pState->m_name.c_str(), m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;

}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleSetParameter(CParameter const* const pParameter, float const value)
{
	ERequestStatus result = eRequestStatus_Failure;
	for (auto const pParameterImpl : pParameter->m_implPtrs)
	{
		result = pParameterImpl->Set(*this, value);
	}

	if (result == eRequestStatus_Success)
	{
		m_parameters[pParameter->GetId()] = value;
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to set the Audio Parameter \"%s\" to %f on Audio Object \"%s\"", pParameter->m_name.c_str(), value, m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleSetEnvironment(CATLAudioEnvironment const* const pEnvironment, float const amount)
{
	ERequestStatus result = eRequestStatus_Failure;

	for (auto const pEnvImpl : pEnvironment->m_implPtrs)
	{
		if (m_pImplData->SetEnvironment(pEnvImpl->m_pImplData, amount) == eRequestStatus_Success)
		{
			result = eRequestStatus_Success;
		}
	}
	if (result == eRequestStatus_Success)
	{
		if (amount > 0.0f)
		{
			m_environments[pEnvironment->GetId()] = amount;
		}
		else
		{
			m_environments.erase(pEnvironment->GetId());
		}
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to set the ATLAudioEnvironment \"%s\" to %f on AudioObject \"%s\"", pEnvironment->m_name.c_str(), amount, m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::StopAllTriggers()
{
	return m_pImplData->StopAllTriggers();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	m_pImplData->SetObstructionOcclusion(obstruction, occlusion);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::LoadTriggerAsync(CATLTrigger const* const pTrigger, bool const bLoad)
{
	ERequestStatus result = eRequestStatus_Failure;
	ControlId const audioTriggerId = pTrigger->GetId();

	for (auto const pTriggerImpl : pTrigger->m_implPtrs)
	{
		EnumFlagsType nTriggerImplFlags = InvalidEnumFlagType;
		ObjectTriggerImplStates::const_iterator iPlace = m_triggerImplStates.end();
		if (FindPlaceConst(m_triggerImplStates, pTriggerImpl->m_audioTriggerImplId, iPlace))
		{
			nTriggerImplFlags = iPlace->second.flags;
		}
		CATLEvent* const pEvent = s_pEventManager->ConstructAudioEvent();
		ERequestStatus prepUnprepResult = eRequestStatus_Failure;

		if (bLoad)
		{
			if (((nTriggerImplFlags & eAudioTriggerStatus_Loaded) == 0) && ((nTriggerImplFlags & eAudioTriggerStatus_Loading) == 0))
			{
				prepUnprepResult = pTriggerImpl->m_pImplData->LoadAsync(pEvent->m_pImplData);
			}
		}
		else
		{
			if (((nTriggerImplFlags & eAudioTriggerStatus_Loaded) != 0) && ((nTriggerImplFlags & eAudioTriggerStatus_Unloading) == 0))
			{
				prepUnprepResult = pTriggerImpl->m_pImplData->UnloadAsync(pEvent->m_pImplData);
			}
		}

		if (prepUnprepResult == eRequestStatus_Success)
		{
			pEvent->m_pAudioObject = this;
			pEvent->m_pTrigger = pTrigger;
			pEvent->m_audioTriggerImplId = pTriggerImpl->m_audioTriggerImplId;

			pEvent->m_audioEventState = bLoad ? eAudioEventState_Loading : eAudioEventState_Unloading;
		}

		if (prepUnprepResult == eRequestStatus_Success)
		{
			pEvent->SetDataScope(pTrigger->GetDataScope());
			ReportStartedEvent(pEvent);
			result = eRequestStatus_Success;// if at least one event fires, it is a success
		}
		else
		{
			s_pEventManager->ReleaseAudioEvent(pEvent);
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (result != eRequestStatus_Success)
	{
		// No TriggerImpl produced an active event.
		g_audioLogger.Log(eAudioLogType_Warning, "LoadTriggerAsync failed on AudioObject \"%s\"", m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleResetEnvironments(AudioEnvironmentLookup const& environmentsLookup)
{
	// Needs to be a copy as SetEnvironment removes from the map that we are iterating over.
	ObjectEnvironmentMap const environments = m_environments;
	ERequestStatus result = eRequestStatus_Success;

	for (auto const& environmentPair : environments)
	{
		CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(environmentsLookup, environmentPair.first, nullptr);

		if (pEnvironment != nullptr)
		{
			if (HandleSetEnvironment(pEnvironment, 0.0f) != eRequestStatus_Success)
			{
				// If setting at least one Environment fails, we consider this a failure.
				result = eRequestStatus_Failure;
			}
		}
	}

	if (result == eRequestStatus_Success)
	{
		m_environments.clear();
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to Reset AudioEnvironments on AudioObject \"%s\"", m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedTriggerInstance(ObjectTriggerStates::iterator& iter)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> requestData(audioTriggerInstanceState.audioTriggerId);
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_SyncCallback;
	request.pObject = this;
	request.pOwner = audioTriggerInstanceState.pOwnerOverride;
	request.pUserData = audioTriggerInstanceState.pUserData;
	request.pUserDataOwner = audioTriggerInstanceState.pUserDataOwner;

	if ((audioTriggerInstanceState.flags & eAudioTriggerStatus_CallbackOnAudioThread) > 0)
	{
		request.flags &= ~eRequestFlags_SyncCallback;
	}

	s_pAudioSystem->PushRequest(request);

	if ((audioTriggerInstanceState.flags & eAudioTriggerStatus_Loaded) > 0)
	{
		// if the trigger instance was manually loaded -- keep it
		audioTriggerInstanceState.flags &= ~eAudioTriggerStatus_Playing;
	}
	else
	{
		//if the trigger instance wasn't loaded -- kill it
		m_triggerStates.erase(iter);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::PushRequest(SAudioRequestData const& requestData, SRequestUserData const& userData)
{
	CAudioRequest request(userData.flags, this, userData.pOwner, userData.pUserData, userData.pUserDataOwner, &requestData);
	s_pAudioSystem->PushRequest(request);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Update(
  float const deltaTime,
  float const distance,
  Vec3 const& audioListenerPosition)
{
	m_propagationProcessor.Update(deltaTime, distance, audioListenerPosition);

	if (m_maxRadius > 0.0f)
	{
		float occlusionFadeOut = 0.0f;
		if (distance < m_maxRadius)
		{
			float const fadeOutStart = m_maxRadius - m_occlusionFadeOutDistance;
			if (fadeOutStart < distance)
			{
				occlusionFadeOut = 1.0f - ((distance - fadeOutStart) / m_occlusionFadeOutDistance);
			}
			else
			{
				occlusionFadeOut = 1.0f;
			}
		}

		m_propagationProcessor.SetOcclusionMultiplier(occlusionFadeOut);
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo)
{
	m_propagationProcessor.ProcessPhysicsRay(pAudioRayInfo);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleSetTransformation(CObjectTransformation const& transformation, float const distanceToListener)
{
	ERequestStatus status = eRequestStatus_Success;
	float const threshold = distanceToListener * g_audioCVars.m_positionUpdateThresholdMultiplier;

	if (!m_attributes.transformation.IsEquivalent(transformation, threshold))
	{
		float const deltaTime = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();

		if (deltaTime > 0.0f)
		{
			m_attributes.transformation = transformation;
			m_attributes.velocity = (m_attributes.transformation.GetPosition() - m_previousAttributes.transformation.GetPosition()) / deltaTime;
			m_flags |= eAudioObjectFlags_NeedsVelocityUpdate;
			m_previousTime = g_lastMainThreadFrameStartTime;
			m_previousAttributes = m_attributes;
		}
		else if (deltaTime < 0.0f) // delta time can get negative after loading a savegame...
		{
			m_previousTime = 0.0f;  // ...in that case we force an update to the new position
			HandleSetTransformation(transformation, 0.0f);
		}

		status = m_pImplData->Set3DAttributes(m_attributes);
	}

	return status;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetOcclusionType(EOcclusionType const calcType, Vec3 const& audioListenerPosition)
{
	CRY_ASSERT(calcType != eOcclusionType_None);
	m_propagationProcessor.SetOcclusionType(calcType, audioListenerPosition);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::GetPropagationData(SATLSoundPropagationData& propagationData) const
{
	m_propagationProcessor.GetPropagationData(propagationData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReleasePendingRays()
{
	m_propagationProcessor.ReleasePendingRays();
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandlePlayFile(CATLStandaloneFile* const pFile, void* const pOwner, void* const pUserData, void* const pUserDataOwner)
{
	ERequestStatus status = eRequestStatus_Failure;

	ERequestStatus const tempStatus = m_pImplData->PlayFile(pFile->m_pImplData);

	if (tempStatus == eRequestStatus_Success || tempStatus == eRequestStatus_Pending)
	{
		if (tempStatus == eRequestStatus_Success)
		{
			pFile->m_state = eAudioStandaloneFileState_Playing;
		}
		else if (tempStatus == eRequestStatus_Pending)
		{
			pFile->m_state = eAudioStandaloneFileState_Loading;
		}

		pFile->m_pAudioObject = this;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pFile->m_pOwner = pOwner;
		pFile->m_pUserData = pUserData;
		pFile->m_pUserDataOwner = pUserDataOwner;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

		ReportStartedStandaloneFile(pFile, pOwner, pUserData, pUserDataOwner);

		// It's a success in both cases.
		status = eRequestStatus_Success;
	}
	else
	{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		g_audioLogger.Log(eAudioLogType_Warning, "PlayFile failed with \"%s\" on AudioObject \"%s\"", pFile->m_hashedFilename.GetText().c_str(), m_name.c_str());
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

		s_pStandaloneFileManager->ReleaseStandaloneFile(pFile);
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleStopFile(char const* const szFile)
{
	ERequestStatus status = eRequestStatus_Failure;
	CHashedString const hashedFilename(szFile);
	auto iter = m_activeStandaloneFiles.cbegin();
	auto iterEnd = m_activeStandaloneFiles.cend();

	while (iter != iterEnd)
	{
		CATLStandaloneFile* const pStandaloneFile = iter->first;

		if (pStandaloneFile != nullptr && pStandaloneFile->m_hashedFilename == hashedFilename)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (pStandaloneFile->m_state != eAudioStandaloneFileState_Playing)
			{
				char const* szState = "unknown";

				switch (pStandaloneFile->m_state)
				{
				case eAudioStandaloneFileState_Playing:
					szState = "playing";
					break;
				case eAudioStandaloneFileState_Loading:
					szState = "loading";
					break;
				case eAudioStandaloneFileState_Stopping:
					szState = "stopping";
					break;
				default:
					break;
				}
				g_audioLogger.Log(eAudioLogType_Warning, "Request to stop a standalone audio file that is not playing! State: \"%s\"", szState);
			}
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

			ERequestStatus const tempStatus = m_pImplData->StopFile(pStandaloneFile->m_pImplData);

			status = eRequestStatus_Success;

			if (tempStatus != eRequestStatus_Pending)
			{
				ReportFinishedStandaloneFile(pStandaloneFile);
				s_pStandaloneFileManager->ReleaseStandaloneFile(pStandaloneFile);

				iter = m_activeStandaloneFiles.begin();
				iterEnd = m_activeStandaloneFiles.end();
				continue;
			}
			else
			{
				pStandaloneFile->m_state = eAudioStandaloneFileState_Stopping;
			}
		}

		++iter;
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetDopplerTracking(bool const bEnable)
{
	if (bEnable)
	{
		m_previousAttributes = m_attributes;
		m_flags |= eAudioObjectFlags_TrackDoppler;
	}
	else
	{
		m_flags &= ~eAudioObjectFlags_TrackDoppler;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetVelocityTracking(bool const bEnable)
{
	if (bEnable)
	{
		m_previousAttributes = m_attributes;
		m_flags |= eAudioObjectFlags_TrackVelocity;
	}
	else
	{
		m_flags &= ~eAudioObjectFlags_TrackVelocity;
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::UpdateControls(float const deltaTime, SObject3DAttributes const& listenerAttributes)
{
	if ((m_flags & eAudioObjectFlags_TrackDoppler) > 0)
	{
		// Approaching positive, departing negative value.
		if (m_attributes.velocity.GetLengthSquared() > 0.0f || listenerAttributes.velocity.GetLengthSquared() > 0.0f)
		{
			Vec3 const relativeVelocityVec(m_attributes.velocity - listenerAttributes.velocity);
			float const relativeVelocity = -relativeVelocityVec.Dot((m_attributes.transformation.GetPosition() - listenerAttributes.transformation.GetPosition()).GetNormalized());

			SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(SATLInternalControlIDs::objectDopplerParameterId, relativeVelocity);
			CAudioRequest request(&requestData);
			request.pObject = this;
			s_pAudioSystem->PushRequest(request);

			m_flags |= eAudioObjectFlags_NeedsDopplerUpdate;
		}
		else if ((m_flags & eAudioObjectFlags_NeedsDopplerUpdate) > 0)
		{
			m_attributes.velocity = ZERO;

			SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(SATLInternalControlIDs::objectDopplerParameterId, 0.0f);
			CAudioRequest request(&requestData);
			request.pObject = this;
			s_pAudioSystem->PushRequest(request);

			m_flags &= ~eAudioObjectFlags_NeedsDopplerUpdate;
			m_pImplData->Set3DAttributes(m_attributes);
		}
	}

	if ((m_flags & eAudioObjectFlags_TrackVelocity) > 0)
	{
		if (m_attributes.velocity.GetLengthSquared() > 0.0f)
		{
			float const currentVelocity = m_attributes.velocity.GetLength();

			if (fabs(currentVelocity - m_previousVelocity) > g_audioCVars.m_velocityTrackingThreshold)
			{
				m_previousVelocity = currentVelocity;

				SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(SATLInternalControlIDs::objectVelocityParameterId, currentVelocity);
				CAudioRequest request(&requestData);
				request.pObject = this;
				s_pAudioSystem->PushRequest(request);
			}
		}
		else if ((m_flags & eAudioObjectFlags_NeedsVelocityUpdate) > 0)
		{
			m_attributes.velocity = ZERO;
			m_previousVelocity = 0.0f;

			SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(SATLInternalControlIDs::objectVelocityParameterId, 0.0f);
			CAudioRequest request(&requestData);
			request.pObject = this;
			s_pAudioSystem->PushRequest(request);

			m_flags &= ~eAudioObjectFlags_NeedsVelocityUpdate;
			m_pImplData->Set3DAttributes(m_attributes);
		}
	}

	// Exponential decay towards zero.
	if (m_attributes.velocity.GetLengthSquared() > 0.0f)
	{
		float const deltaTime2 = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();
		float const decay = std::max(1.0f - deltaTime2 / 0.125f, 0.0f);
		m_attributes.velocity *= decay;
		m_pImplData->Set3DAttributes(m_attributes);
	}
}

///////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::CanBeReleased() const
{
	return (m_flags & eAudioObjectFlags_DoNotRelease) == 0 &&
	       m_activeEvents.empty() &&
	       m_activeStandaloneFiles.empty() &&
	       !m_propagationProcessor.HasPendingRays();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetFlag(EAudioObjectFlags const flag)
{
	m_flags |= flag;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::RemoveFlag(EAudioObjectFlags const flag)
{
	m_flags &= ~flag;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
float const CATLAudioObject::CStateDebugDrawData::s_minAlpha = 0.5f;
float const CATLAudioObject::CStateDebugDrawData::s_maxAlpha = 1.0f;
int const CATLAudioObject::CStateDebugDrawData::s_maxToMinUpdates = 100;
static char const* const s_szOcclusionTypes[] = { "None", "Ignore", "Adaptive", "Low", "Medium", "High" };

///////////////////////////////////////////////////////////////////////////
CATLAudioObject::CStateDebugDrawData::CStateDebugDrawData(SwitchStateId const audioSwitchState)
	: m_currentState(audioSwitchState)
	, m_currentAlpha(s_maxAlpha)
{
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::CStateDebugDrawData::Update(SwitchStateId const audioSwitchState)
{
	if ((audioSwitchState == m_currentState) && (m_currentAlpha > s_minAlpha))
	{
		m_currentAlpha -= (s_maxAlpha - s_minAlpha) / s_maxToMinUpdates;
	}
	else if (audioSwitchState != m_currentState)
	{
		m_currentState = audioSwitchState;
		m_currentAlpha = s_maxAlpha;
	}
}

typedef std::map<ControlId const, size_t> TriggerCountMap;

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::DrawDebugInfo(
  IRenderAuxGeom& auxGeom,
  Vec3 const& listenerPosition,
  AudioTriggerLookup const& triggers,
  AudioParameterLookup const& parameters,
  AudioSwitchLookup const& switches,
  AudioPreloadRequestLookup const& preloadRequests,
  AudioEnvironmentLookup const& environments) const
{
	m_propagationProcessor.DrawObstructionRays(auxGeom);

	if (g_audioCVars.m_drawAudioDebug > 0)
	{
		Vec3 const& position = m_attributes.transformation.GetPosition();
		Vec3 screenPos(ZERO);

		if (IRenderer* const pRenderer = gEnv->pRenderer)
		{
			pRenderer->ProjectToScreen(position.x, position.y, position.z, &screenPos.x, &screenPos.y, &screenPos.z);

			screenPos.x = screenPos.x * 0.01f * pRenderer->GetWidth();
			screenPos.y = screenPos.y * 0.01f * pRenderer->GetHeight();
		}
		else
		{
			screenPos.z = -1.0f;
		}

		if ((screenPos.z >= 0.0f) && (screenPos.z <= 1.0f))
		{
			float const distance = position.GetDistance(listenerPosition);

			if ((g_audioCVars.m_drawAudioDebug & eADDF_DRAW_SPHERES) > 0)
			{
				SAuxGeomRenderFlags const previousRenderFlags = auxGeom.GetRenderFlags();
				SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
				newRenderFlags.SetCullMode(e_CullModeNone);
				auxGeom.SetRenderFlags(newRenderFlags);
				float const radius = 0.15f;
				auxGeom.DrawSphere(position, radius, ColorB(255, 1, 1, 255));
				auxGeom.SetRenderFlags(previousRenderFlags);
			}
			float const fontSize = 1.3f;
			float const lineHeight = 12.0f;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_STATES) > 0 && !m_switchStates.empty())
			{
				Vec3 switchPos(screenPos);

				for (auto const& switchStatePair : m_switchStates)
				{

					CATLSwitch const* const pSwitch = stl::find_in_map(switches, switchStatePair.first, nullptr);

					if (pSwitch != nullptr)
					{
						CATLSwitchState const* const pSwitchState = stl::find_in_map(pSwitch->audioSwitchStates, switchStatePair.second, nullptr);

						if (pSwitchState != nullptr)
						{
							if (!pSwitch->m_name.empty() && !pSwitchState->m_name.empty())
							{
								CStateDebugDrawData& drawData = m_stateDrawInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(pSwitch->GetId()), std::forward_as_tuple(pSwitchState->GetId())).first->second;
								drawData.Update(pSwitchState->GetId());
								float const switchTextColor[4] = { 0.8f, 0.8f, 0.8f, drawData.m_currentAlpha };

								switchPos.y -= lineHeight;
								auxGeom.Draw2dLabel(
								  switchPos.x,
								  switchPos.y,
								  fontSize,
								  switchTextColor,
								  false,
								  "%s: %s\n",
								  pSwitch->m_name.c_str(),
								  pSwitchState->m_name.c_str());
							}
						}
					}
				}
			}

			CryFixedStringT<MaxMiscStringLength> temp;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_LABEL) > 0)
			{
				static float const objectTextColor[4] = { 0.90f, 0.90f, 0.90f, 1.0f };
				static float const objectGrayTextColor[4] = { 0.50f, 0.50f, 0.50f, 1.0f };

				EOcclusionType const occlusionType = m_propagationProcessor.GetOcclusionType();
				SATLSoundPropagationData propagationData;
				m_propagationProcessor.GetPropagationData(propagationData);

				CATLAudioObject* const pAudioObject = const_cast<CATLAudioObject*>(this);

				auxGeom.Draw2dLabel(
				  screenPos.x,
				  screenPos.y,
				  fontSize,
				  objectTextColor,
				  false,
				  "%s Dist:%4.1fm",
				  pAudioObject->m_name.c_str(),
				  distance);

				if (distance < g_audioCVars.m_occlusionMaxDistance)
				{
					if (occlusionType == eOcclusionType_Adaptive)
					{

						temp.Format(
						  "%s(%s)",
						  s_szOcclusionTypes[occlusionType],
						  s_szOcclusionTypes[m_propagationProcessor.GetOcclusionTypeWhenAdaptive()]);
					}
					else
					{
						temp.Format("%s", s_szOcclusionTypes[occlusionType]);
					}
				}
				else
				{
					temp.Format("Ignore (exceeded activity range)");
				}

				auxGeom.Draw2dLabel(
				  screenPos.x,
				  screenPos.y + lineHeight,
				  fontSize,
				  (occlusionType != eOcclusionType_None && occlusionType != eOcclusionType_Ignore) ? objectTextColor : objectGrayTextColor,
				  false,
				  "Obst: %3.2f Occl: %3.2f Type: %s",
				  propagationData.obstruction,
				  propagationData.occlusion,
				  temp.c_str());
			}

			float const textColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_PARAMETERS) > 0 && !m_parameters.empty())
			{
				Vec3 parameterPos(screenPos);

				for (auto const& parameterPair : m_parameters)
				{
					CParameter const* const pParameter = stl::find_in_map(parameters, parameterPair.first, nullptr);

					if (pParameter != nullptr)
					{
						float const offsetOnX = (static_cast<float>(pParameter->m_name.size()) + 5.6f) * 5.4f * fontSize;
						parameterPos.y -= lineHeight;
						auxGeom.Draw2dLabel(
						  parameterPos.x - offsetOnX,
						  parameterPos.y,
						  fontSize,
						  textColor, false,
						  "%s: %2.2f\n",
						  pParameter->m_name.c_str(), parameterPair.second);
					}
				}
			}

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_ENVIRONMENTS) > 0 && !m_environments.empty())
			{
				Vec3 envPos(screenPos);

				for (auto const& environmentPair : m_environments)
				{
					CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(environments, environmentPair.first, nullptr);

					if (pEnvironment != nullptr)
					{
						float const offsetOnX = (static_cast<float>(pEnvironment->m_name.size()) + 5.1f) * 5.4f * fontSize;

						envPos.y += lineHeight;
						auxGeom.Draw2dLabel(
						  envPos.x - offsetOnX,
						  envPos.y,
						  fontSize,
						  textColor,
						  false,
						  "%s: %.2f\n",
						  pEnvironment->m_name.c_str(),
						  environmentPair.second);
					}
				}
			}

			CryFixedStringT<MaxMiscStringLength> controls;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_TRIGGERS) > 0 && !m_triggerStates.empty())
			{
				TriggerCountMap triggerCounts;

				for (auto const& triggerStatesPair : m_triggerStates)
				{
					++(triggerCounts[triggerStatesPair.second.audioTriggerId]);
				}

				for (auto const& triggerCountsPair : triggerCounts)
				{
					CATLTrigger const* const pTrigger = stl::find_in_map(triggers, triggerCountsPair.first, nullptr);

					if (pTrigger != nullptr)
					{
						size_t const numInstances = triggerCountsPair.second;

						if (numInstances == 1)
						{
							temp.Format("%s\n", pTrigger->m_name.c_str());
						}
						else
						{
							temp.Format("%s: %" PRISIZE_T "\n", pTrigger->m_name.c_str(), numInstances);
						}

						controls += temp;
						temp.clear();
					}
				}
			}

			if ((g_audioCVars.m_drawAudioDebug & eADDF_DRAW_OBJECT_STANDALONE_FILES) > 0 && !m_activeStandaloneFiles.empty())
			{
				std::map<CHashedString, size_t> numStandaloneFiles;
				for (auto const& standaloneFilePair : m_activeStandaloneFiles)
				{
					++(numStandaloneFiles[standaloneFilePair.first->m_hashedFilename]);
				}

				for (auto const& numInstancesPair : numStandaloneFiles)
				{
					size_t const numInstances = numInstancesPair.second;

					if (numInstances == 1)
					{
						temp.Format("%s\n", numInstancesPair.first.GetText().c_str());
					}
					else
					{
						temp.Format("%s: %" PRISIZE_T "\n", numInstancesPair.first.GetText().c_str(), numInstances);
					}

					controls += temp;
					temp.clear();
				}
			}

			if (!controls.empty())
			{
				auxGeom.Draw2dLabel(
				  screenPos.x,
				  screenPos.y + 2.0f * lineHeight,
				  fontSize, textColor,
				  false,
				  "%s",
				  controls.c_str());
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ForceImplementationRefresh(
  AudioTriggerLookup const& triggers,
  AudioParameterLookup const& parameters,
  AudioSwitchLookup const& switches,
  AudioEnvironmentLookup const& environments,
  bool const bSet3DAttributes)
{
	if (bSet3DAttributes)
	{
		m_pImplData->Set3DAttributes(m_attributes);
	}

	// Parameters
	ObjectParameterMap const audioParameters = m_parameters;
	for (auto const& parameterPair : audioParameters)
	{
		CParameter const* const pParameter = stl::find_in_map(parameters, parameterPair.first, nullptr);
		if (pParameter != nullptr)
		{
			ERequestStatus const result = HandleSetParameter(pParameter, parameterPair.second);
			if (result != eRequestStatus_Success)
			{
				g_audioLogger.Log(eAudioLogType_Warning, "Parameter \"%s\" failed during audio middleware switch on AudioObject \"%s\"", pParameter->m_name.c_str(), m_name.c_str());
			}
		}
	}

	// Switches
	ObjectStateMap const audioSwitches = m_switchStates;

	for (auto const& switchPair : audioSwitches)
	{
		CATLSwitch const* const pSwitch = stl::find_in_map(switches, switchPair.first, nullptr);

		if (pSwitch != nullptr)
		{
			CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, switchPair.second, nullptr);

			if (pState != nullptr)
			{
				ERequestStatus const result = HandleSetSwitchState(pSwitch, pState);

				if (result != eRequestStatus_Success)
				{
					g_audioLogger.Log(eAudioLogType_Warning, "SwitchStateImpl \"%s\" : \"%s\" failed during audio middleware switch on AudioObject \"%s\"", pSwitch->m_name.c_str(), pState->m_name.c_str(), m_name.c_str());
				}
			}
		}
	}

	// Environments
	ObjectEnvironmentMap const audioEnvironments = m_environments;

	for (auto const& environmentPair : audioEnvironments)
	{
		CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(environments, environmentPair.first, nullptr);

		if (pEnvironment != nullptr)
		{
			ERequestStatus const result = HandleSetEnvironment(pEnvironment, environmentPair.second);

			if (result != eRequestStatus_Success)
			{
				g_audioLogger.Log(eAudioLogType_Warning, "Environment \"%s\" failed during audio middleware switch on AudioObject \"%s\"", pEnvironment->m_name.c_str(), m_name.c_str());
			}
		}
	}

	// Last re-execute its active triggers and standalone files.
	ObjectTriggerStates& triggerStates = m_triggerStates;
	auto it = triggerStates.cbegin();
	auto end = triggerStates.cend();
	while (it != end)
	{
		auto const& triggerState = *it;
		CATLTrigger const* const pTrigger = stl::find_in_map(triggers, triggerState.second.audioTriggerId, nullptr);

		if (pTrigger != nullptr)
		{
			if (!pTrigger->m_implPtrs.empty())
			{
				for (auto const pTriggerImpl : pTrigger->m_implPtrs)
				{

					CATLEvent* const pEvent = s_pEventManager->ConstructAudioEvent();
					ERequestStatus activateResult = m_pImplData->ExecuteTrigger(pTriggerImpl->m_pImplData, pEvent->m_pImplData);

					if (activateResult == eRequestStatus_Success || activateResult == eRequestStatus_Pending)
					{
						pEvent->m_pAudioObject = this;
						pEvent->m_pTrigger = pTrigger;
						pEvent->m_audioTriggerImplId = pTriggerImpl->m_audioTriggerImplId;
						pEvent->m_audioTriggerInstanceId = triggerState.first;
						pEvent->SetDataScope(pTrigger->GetDataScope());

						if (activateResult == eRequestStatus_Success)
						{
							pEvent->m_audioEventState = eAudioEventState_Playing;
						}
						else if (activateResult == eRequestStatus_Pending)
						{
							pEvent->m_audioEventState = eAudioEventState_Loading;
						}
						ReportStartedEvent(pEvent);
					}
					else
					{
						g_audioLogger.Log(eAudioLogType_Warning, "TriggerImpl \"%s\" failed during audio middleware switch on AudioObject \"%s\"", pTrigger->m_name.c_str(), m_name.c_str());
						s_pEventManager->ReleaseAudioEvent(pEvent);
					}
				}
			}
			else
			{
				// The middleware has no connections set up.
				// Stop the event in this case.
				g_audioLogger.Log(eAudioLogType_Warning, "No trigger connections found during audio middleware switch for \"%s\" on \"%s\"", pTrigger->m_name.c_str(), m_name.c_str());
			}
		}
		else
		{
			it = triggerStates.erase(it);
			end = triggerStates.cend();
			continue;
		}

		++it;
	}

	ObjectStandaloneFileMap const& activeStandaloneFiles = m_activeStandaloneFiles;

	for (auto const& standaloneFilePair : activeStandaloneFiles)
	{
		CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.first;

		if (pStandaloneFile != nullptr)
		{
			CRY_ASSERT(pStandaloneFile->m_state == eAudioStandaloneFileState_Playing);

			ERequestStatus const status = HandlePlayFile(pStandaloneFile, pStandaloneFile->m_pOwner, pStandaloneFile->m_pUserData, pStandaloneFile->m_pUserDataOwner);

			if (status == eRequestStatus_Success || status == eRequestStatus_Pending)
			{
				if (status == eRequestStatus_Success)
				{
					pStandaloneFile->m_state = eAudioStandaloneFileState_Playing;
				}
				else if (status == eRequestStatus_Pending)
				{
					pStandaloneFile->m_state = eAudioStandaloneFileState_Loading;
				}
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "PlayFile failed with \"%s\" on AudioObject \"%s\"", pStandaloneFile->m_hashedFilename.GetText().c_str(), m_name.c_str());
			}
		}
		else
		{
			g_audioLogger.Log(eAudioLogType_Error, "Retrigger active standalone audio files failed on instance: %u and file: %u as m_audioStandaloneFileMgr.LookupID() returned nullptr!", standaloneFilePair.first, standaloneFilePair.second);
		}
	}
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> requestData(triggerId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (triggerId != InvalidControlId)
	{
		SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> requestData(triggerId);
		PushRequest(requestData, userData);
	}
	else
	{
		SAudioObjectRequestData<eAudioObjectRequestType_StopAllTriggers> requestData;
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetTransformation(CObjectTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> requestData(transformation);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(parameterId, value);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> requestData(audioSwitchId, audioSwitchStateId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetEnvironment(EnvironmentId const audioEnvironmentId, float const amount, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironment> requestData(audioEnvironmentId, amount);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetCurrentEnvironments(EntityId const entityToIgnore, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetCurrentEnvironments> requestData(entityToIgnore, m_attributes.transformation.GetPosition());
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ResetEnvironments(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_ResetEnvironments> requestData;
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	EnumFlagsType const index = static_cast<EnumFlagsType>(occlusionType);

	if (index < eOcclusionType_Count)
	{
		SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> requestData(s_occlusionTypeSwitchId, s_occlusionTypeStateIds[index]);
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopFile(char const* const szFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_StopFile> requestData(szFile);
	PushRequest(requestData, userData);
}
