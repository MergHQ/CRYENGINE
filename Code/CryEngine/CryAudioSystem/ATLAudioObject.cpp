// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLAudioObject.h"
#include "SoundCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Release()
{
	m_activeEvents.clear();
	m_triggerImplStates.clear();

	for (auto& triggerStatesPair : m_triggerStates)
	{
		triggerStatesPair.second.numLoadingEvents = 0;
		triggerStatesPair.second.numPlayingEvents = 0;
	}

	m_pImplData = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartingTriggerInstance(AudioTriggerInstanceId const audioTriggerInstanceId, AudioControlId const audioTriggerId)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = m_triggerStates.insert(std::make_pair(audioTriggerInstanceId, SAudioTriggerInstanceState())).first->second;
	audioTriggerInstanceState.audioTriggerId = audioTriggerId;
	audioTriggerInstanceState.flags |= eAudioTriggerStatus_Starting;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedTriggerInstance(
  AudioTriggerInstanceId const audioTriggerInstanceId,
  void* const pOwnerOverride,
  void* const pUserData,
  void* const pUserDataOwner,
  AudioEnumFlagsType const flags)
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

			if ((flags & eAudioRequestFlags_SyncFinishedCallback) == 0)
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
		g_audioLogger.Log(eAudioLogType_Warning, "Reported a started instance %u that couldn't be found on an object %u",
		                  audioTriggerInstanceId, GetId());
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
					ReportPrepUnprepTriggerImpl(_pEvent->m_audioTriggerImplId, true);
				}

				break;
			}
		case eAudioEventState_Unloading:
			{
				if (_bSuccess)
				{
					ReportPrepUnprepTriggerImpl(_pEvent->m_audioTriggerImplId, false);
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
		if (_pEvent->m_pTrigger)
		{
			g_audioLogger.Log(eAudioLogType_Warning, "Reported finished event %u on an inactive trigger %u", _pEvent->GetId(), _pEvent->m_pTrigger->GetId());
		}
		else
		{
			g_audioLogger.Log(eAudioLogType_Warning, "Reported finished event %u on a trigger that does not exist anymore", _pEvent->GetId());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::GetStartedStandaloneFileRequestData(CATLStandaloneFile const* const _pStandaloneFile, CAudioRequestInternal& _request)
{
	ObjectStandaloneFileMap::const_iterator const iter(m_activeStandaloneFiles.find(_pStandaloneFile->GetId()));

	if (iter != m_activeStandaloneFiles.end())
	{
		SAudioStandaloneFileData const& audioStandaloneFileData = iter->second;
		_request.pOwner = audioStandaloneFileData.pOwnerOverride;
		_request.pUserData = audioStandaloneFileData.pUserData;
		_request.pUserDataOwner = audioStandaloneFileData.pUserDataOwner;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedStandaloneFile(
  CATLStandaloneFile const* const _pStandaloneFile,
  void* const _pOwner,
  void* const _pUserData,
  void* const _pUserDataOwner)
{
	m_activeStandaloneFiles.insert(
	  std::pair<AudioStandaloneFileId, SAudioStandaloneFileData>(
	    _pStandaloneFile->GetId(),
	    SAudioStandaloneFileData(
	      _pOwner,
	      _pUserData,
	      _pUserDataOwner,
	      _pStandaloneFile->m_id)));
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedStandaloneFile(CATLStandaloneFile const* const _pStandaloneFile)
{
	m_activeStandaloneFiles.erase(_pStandaloneFile->GetId());
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportPrepUnprepTriggerImpl(AudioTriggerImplId const audioTriggerImplId, bool const bPrepared)
{
	if (bPrepared)
	{
		m_triggerImplStates[audioTriggerImplId].flags |= eAudioTriggerStatus_Prepared;
	}
	else
	{
		m_triggerImplStates[audioTriggerImplId].flags &= ~eAudioTriggerStatus_Prepared;
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetSwitchState(AudioControlId const audioSwitchId, AudioSwitchStateId const audioSwitchStateId)
{
	m_switchStates[audioSwitchId] = audioSwitchStateId;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetRtpc(AudioControlId const audioRtpcId, float const value)
{
	m_rtpcs[audioRtpcId] = value;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetEnvironmentAmount(AudioEnvironmentId const audioEnvironmentId, float const amount)
{
	if (amount > 0.0f)
	{
		m_environments[audioEnvironmentId] = amount;
	}
	else
	{
		m_environments.erase(audioEnvironmentId);
	}
}

///////////////////////////////////////////////////////////////////////////
ObjectTriggerImplStates const& CATLAudioObject::GetTriggerImpls() const
{
	return m_triggerImplStates;
}

///////////////////////////////////////////////////////////////////////////
ObjectEnvironmentMap const& CATLAudioObject::GetEnvironments() const
{
	return m_environments;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ClearEnvironments()
{
	m_environments.clear();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Clear()
{
	m_activeEvents.clear();
	m_triggerStates.clear();
	m_triggerImplStates.clear();
	m_switchStates.clear();
	m_rtpcs.clear();
	m_environments.clear();
	m_attributes = SAudioObject3DAttributes();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedTriggerInstance(ObjectTriggerStates::iterator& iter)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
	SAudioRequest request;
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> requestData(audioTriggerInstanceState.audioTriggerId);
	request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ThreadSafePush | eAudioRequestFlags_SyncCallback;
	request.audioObjectId = GetId();
	request.pData = &requestData;
	request.pOwner = audioTriggerInstanceState.pOwnerOverride;
	request.pUserData = audioTriggerInstanceState.pUserData;
	request.pUserDataOwner = audioTriggerInstanceState.pUserDataOwner;

	if ((audioTriggerInstanceState.flags & eAudioTriggerStatus_CallbackOnAudioThread) > 0)
	{
		request.flags &= ~eAudioRequestFlags_SyncCallback;
	}

	gEnv->pAudioSystem->PushRequest(request);

	if ((audioTriggerInstanceState.flags & eAudioTriggerStatus_Prepared) > 0)
	{
		// if the trigger instance was manually prepared -- keep it
		audioTriggerInstanceState.flags &= ~eAudioTriggerStatus_Playing;
	}
	else
	{
		//if the trigger instance wasn't prepared -- kill it
		m_triggerStates.erase(iter);
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Update(
  float const deltaTime,
  float const distance,
  Vec3 const& audioListenerPosition)
{
	m_propagationProcessor.Update(deltaTime, distance, audioListenerPosition);

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

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo)
{
	m_propagationProcessor.ProcessPhysicsRay(pAudioRayInfo);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetTransformation(CAudioObjectTransformation const& transformation)
{
	float const deltaTime = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();

	if (deltaTime > 0.0f)
	{
		m_attributes.transformation = transformation;
		m_attributes.velocity = (m_attributes.transformation.GetPosition() - m_previousAttributes.transformation.GetPosition()) / deltaTime;
		m_previousTime = g_lastMainThreadFrameStartTime;
		m_previousAttributes = m_attributes;
		m_flags |= eAudioObjectFlags_NeedsVelocityUpdate;
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetOcclusionType(EAudioOcclusionType const calcType, Vec3 const& audioListenerPosition)
{
	CRY_ASSERT(calcType != eAudioOcclusionType_None);
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
void CATLAudioObject::UpdateControls(float const deltaTime, SAudioObject3DAttributes const& listenerAttributes)
{
	if ((m_flags & eAudioObjectFlags_TrackDoppler) > 0)
	{
		// Approaching positive, departing negative value.
		if (m_attributes.velocity.GetLengthSquared() > 0.0f || listenerAttributes.velocity.GetLengthSquared() > 0.0f)
		{
			Vec3 const relativeVelocityVec(m_attributes.velocity - listenerAttributes.velocity);
			float const relativeVelocity = -relativeVelocityVec.Dot((m_attributes.transformation.GetPosition() - listenerAttributes.transformation.GetPosition()).GetNormalized());

			SAudioRequest request;
			SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(SATLInternalControlIDs::objectDopplerRtpcId, relativeVelocity);

			request.audioObjectId = GetId();
			request.flags = eAudioRequestFlags_ThreadSafePush;
			request.pData = &requestData;
			gEnv->pAudioSystem->PushRequest(request);
			m_flags |= eAudioObjectFlags_NeedsDopplerUpdate;
		}
		else if ((m_flags & eAudioObjectFlags_NeedsDopplerUpdate) > 0)
		{
			m_attributes.velocity = ZERO;
			SAudioRequest request;
			SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(SATLInternalControlIDs::objectDopplerRtpcId, 0.0f);

			request.audioObjectId = GetId();
			request.flags = eAudioRequestFlags_ThreadSafePush;
			request.pData = &requestData;
			gEnv->pAudioSystem->PushRequest(request);
			m_flags &= ~eAudioObjectFlags_NeedsDopplerUpdate;
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
				SAudioRequest request;
				SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(SATLInternalControlIDs::objectVelocityRtpcId, currentVelocity);

				request.audioObjectId = GetId();
				request.flags = eAudioRequestFlags_ThreadSafePush;
				request.pData = &requestData;
				gEnv->pAudioSystem->PushRequest(request);
			}
		}
		else if ((m_flags & eAudioObjectFlags_NeedsVelocityUpdate) > 0)
		{
			m_attributes.velocity = ZERO;
			m_previousVelocity = 0.0f;
			SAudioRequest request;
			SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(SATLInternalControlIDs::objectVelocityRtpcId, 0.0f);

			request.audioObjectId = GetId();
			request.flags = eAudioRequestFlags_ThreadSafePush;
			request.pData = &requestData;
			gEnv->pAudioSystem->PushRequest(request);
			m_flags &= ~eAudioObjectFlags_NeedsVelocityUpdate;
		}
	}

	if ((m_flags & (eAudioObjectFlags_TrackDoppler | eAudioObjectFlags_TrackVelocity)) > 0)
	{
		// Exponential decay towards zero.
		if (m_attributes.velocity.GetLengthSquared() > 0.0f)
		{
			float const deltaTime2 = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();
			float const decay = std::max(1.0f - deltaTime2 / 0.125f, 0.0f);
			m_attributes.velocity *= decay;
		}
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

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::CheckBeforeRemoval(CATLDebugNameStore const* const pDebugNameStore) const
{
	// warn if there is activity on an object being cleared
	if (!m_activeEvents.empty())
	{
		char const* const szEventString = GetEventIDs("; ").c_str();
		g_audioLogger.Log(eAudioLogType_Warning, "Active events on an object to be released: %u! EventIDs: %s", GetId(), szEventString);
	}

	if (!m_triggerStates.empty())
	{
		char const* const szTriggerString = GetTriggerNames("; ", pDebugNameStore).c_str();
		g_audioLogger.Log(eAudioLogType_Warning, "Active triggers on an object to be released: %u! TriggerNames: %s", GetId(), szTriggerString);
	}

	if (!m_activeStandaloneFiles.empty())
	{
		char const* const szStandaloneFilesString = GetStandaloneFileIDs("; ", pDebugNameStore).c_str();
		g_audioLogger.Log(eAudioLogType_Warning, "Active standalone files on an object to be released: %u! FileNames: %s", GetId(), szStandaloneFilesString);
	}
}
typedef std::map<AudioControlId, size_t, std::less<AudioControlId>,
                 STLSoundAllocator<std::pair<AudioControlId, size_t>>> TTriggerCountMap;

///////////////////////////////////////////////////////////////////////////
CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> CATLAudioObject::GetTriggerNames(char const* const _szSeparator, CATLDebugNameStore const* const _pDebugNameStore) const
{
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> triggers;
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> temp;
	TTriggerCountMap triggerCounts;

	for (auto const& triggerStatesPair : m_triggerStates)
	{
		++(triggerCounts[triggerStatesPair.second.audioTriggerId]);
	}

	for (auto const& triggerCountsPair : triggerCounts)
	{
		char const* const szName = _pDebugNameStore->LookupAudioTriggerName(triggerCountsPair.first);

		if (szName != nullptr)
		{
			size_t const numInstances = triggerCountsPair.second;

			if (numInstances == 1)
			{
				temp.Format("%s%s", szName, _szSeparator);
			}
			else
			{
				temp.Format("%s(%" PRISIZE_T " inst.)%s", szName, numInstances, _szSeparator);
			}

			triggers += temp;
		}
	}
	return triggers;
}

///////////////////////////////////////////////////////////////////////////
CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> CATLAudioObject::GetEventIDs(char const* const _szSeparator) const
{
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> events;
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> temp;

	for (auto const* const pEvent : m_activeEvents)
	{
		temp.Format("%u%s", pEvent->GetId(), _szSeparator);
		events += temp;
	}

	return events;
}

typedef std::map<AudioStandaloneFileId, size_t, std::less<AudioStandaloneFileId>,
                 STLSoundAllocator<std::pair<AudioStandaloneFileId, size_t>>> TAudioStandaloneFileCountMap;

//////////////////////////////////////////////////////////////////////////
CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> CATLAudioObject::GetStandaloneFileIDs(char const* const _szSeparator, CATLDebugNameStore const* const _pDebugNameStore) const
{
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> audioStandaloneFiles;
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> temp;

	TAudioStandaloneFileCountMap numStandaloneFiles;

	for (auto const& standaloneFilePair : m_activeStandaloneFiles)
	{
		++(numStandaloneFiles[standaloneFilePair.second.fileId]);
	}

	for (auto const& numInstancesPair : numStandaloneFiles)
	{
		char const* const szName = _pDebugNameStore->LookupAudioStandaloneFileName(numInstancesPair.first);

		if (szName != nullptr)
		{
			size_t const numInstances = numInstancesPair.second;

			if (numInstances == 1)
			{
				temp.Format("%s%s", szName, _szSeparator);
			}
			else
			{
				temp.Format("%s(%" PRISIZE_T " inst.)%s", szName, numInstances, _szSeparator);
			}
			audioStandaloneFiles += temp;
		}
	}

	return audioStandaloneFiles;
}

//////////////////////////////////////////////////////////////////////////
float const CATLAudioObject::CStateDebugDrawData::s_minAlpha = 0.5f;
float const CATLAudioObject::CStateDebugDrawData::s_maxAlpha = 1.0f;
int const CATLAudioObject::CStateDebugDrawData::s_maxToMinUpdates = 100;
static char const* const s_szOcclusionTypes[] = { "None", "Ignore", "Adaptive", "Low", "Medium", "High" };

///////////////////////////////////////////////////////////////////////////
CATLAudioObject::CStateDebugDrawData::CStateDebugDrawData(AudioSwitchStateId const audioSwitchState)
	: m_currentState(audioSwitchState)
	, m_currentAlpha(s_maxAlpha)
{
}

///////////////////////////////////////////////////////////////////////////
CATLAudioObject::CStateDebugDrawData::~CStateDebugDrawData()
{
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::CStateDebugDrawData::Update(AudioSwitchStateId const newState)
{
	if ((newState == m_currentState) && (m_currentAlpha > s_minAlpha))
	{
		m_currentAlpha -= (s_maxAlpha - s_minAlpha) / s_maxToMinUpdates;
	}
	else if (newState != m_currentState)
	{
		m_currentState = newState;
		m_currentAlpha = s_maxAlpha;
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPosition, CATLDebugNameStore const* const pDebugNameStore) const
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

		if ((0.0f <= screenPos.z) && (screenPos.z <= 1.0f))
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
					AudioControlId const switchId = switchStatePair.first;
					AudioSwitchStateId const stateId = switchStatePair.second;

					char const* const szSwitchName = pDebugNameStore->LookupAudioSwitchName(switchId);
					char const* const szStateName = pDebugNameStore->LookupAudioSwitchStateName(switchId, stateId);

					if ((szSwitchName != nullptr) && (szStateName != nullptr))
					{
						CStateDebugDrawData& drawData = m_stateDrawInfoMap[switchId];
						drawData.Update(stateId);
						float const switchTextColor[4] = { 0.8f, 0.8f, 0.8f, drawData.m_currentAlpha };

						switchPos.y -= lineHeight;
						auxGeom.Draw2dLabel(
						  switchPos.x,
						  switchPos.y,
						  fontSize,
						  switchTextColor,
						  false,
						  "%s: %s\n",
						  szSwitchName,
						  szStateName);
					}
				}
			}

			CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> temp;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_LABEL) > 0)
			{
				static float const objectTextColor[4] = { 0.90f, 0.90f, 0.90f, 1.0f };
				static float const objectGrayTextColor[4] = { 0.50f, 0.50f, 0.50f, 1.0f };

				EAudioOcclusionType const occlusionType = m_propagationProcessor.GetOcclusionType();
				SATLSoundPropagationData propagationData;
				m_propagationProcessor.GetPropagationData(propagationData);

				AudioObjectId const audioObjectId = GetId();

				auxGeom.Draw2dLabel(
				  screenPos.x,
				  screenPos.y,
				  fontSize,
				  objectTextColor,
				  false,
				  "%s ID: %u Dist:%4.1fm",
				  pDebugNameStore->LookupAudioObjectName(audioObjectId),
				  audioObjectId,
				  distance);

				if (distance < g_audioCVars.m_occlusionMaxDistance)
				{
					if (occlusionType == eAudioOcclusionType_Adaptive)
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
				  (occlusionType != eAudioOcclusionType_None && occlusionType != eAudioOcclusionType_Ignore) ? objectTextColor : objectGrayTextColor,
				  false,
				  "Obst: %3.2f Occl: %3.2f Type: %s",
				  propagationData.obstruction,
				  propagationData.occlusion,
				  temp.c_str());
			}

			float const textColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_RTPCS) > 0 && !m_rtpcs.empty())
			{
				Vec3 rtpcPos(screenPos);

				for (auto const& rtpcPair : m_rtpcs)
				{
					AudioControlId const rtpcId = rtpcPair.first;
					float const rtpcValue = rtpcPair.second;
					char const* const szRtpcName = pDebugNameStore->LookupAudioRtpcName(rtpcId);

					if (szRtpcName != nullptr)
					{
						float const offsetOnX = (strlen(szRtpcName) + 5.6f) * 5.4f * fontSize;

						rtpcPos.y -= lineHeight;
						auxGeom.Draw2dLabel(
						  rtpcPos.x - offsetOnX,
						  rtpcPos.y,
						  fontSize,
						  textColor, false,
						  "%s: %2.2f\n",
						  szRtpcName,
						  rtpcValue);
					}
				}
			}

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_ENVIRONMENTS) > 0 && !m_environments.empty())
			{
				Vec3 envPos(screenPos);

				for (auto const& environmentPair : m_environments)
				{
					AudioControlId const envId = environmentPair.first;
					float const envValue = environmentPair.second;

					char const* const szName = pDebugNameStore->LookupAudioEnvironmentName(envId);

					if (szName != nullptr)
					{
						float const offsetOnX = (strlen(szName) + 5.1f) * 5.4f * fontSize;

						envPos.y += lineHeight;
						auxGeom.Draw2dLabel(
						  envPos.x - offsetOnX,
						  envPos.y,
						  fontSize,
						  textColor,
						  false,
						  "%s: %.2f\n",
						  szName,
						  envValue);
					}
				}
			}

			CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> controls;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_TRIGGERS) > 0 && !m_triggerStates.empty())
			{
				TTriggerCountMap triggerCounts;

				for (auto const& triggerStatesPair : m_triggerStates)
				{
					++(triggerCounts[triggerStatesPair.second.audioTriggerId]);
				}

				for (auto const& triggerCountsPair : triggerCounts)
				{
					char const* const szName = pDebugNameStore->LookupAudioTriggerName(triggerCountsPair.first);

					if (szName != nullptr)
					{
						size_t const numInstances = triggerCountsPair.second;

						if (numInstances == 1)
						{
							temp.Format("%s\n", szName);
						}
						else
						{
							temp.Format("%s: %" PRISIZE_T "\n", szName, numInstances);
						}

						controls += temp;
						temp.clear();
					}
				}
			}

			if ((g_audioCVars.m_drawAudioDebug & eADDF_DRAW_OBJECT_STANDALONE_FILES) > 0 && !m_activeStandaloneFiles.empty())
			{
				TAudioStandaloneFileCountMap numStandaloneFiles;

				for (auto const& standaloneFilePair : m_activeStandaloneFiles)
				{
					++(numStandaloneFiles[standaloneFilePair.second.fileId]);
				}

				for (auto const& numInstancesPair : numStandaloneFiles)
				{
					char const* const szName = pDebugNameStore->LookupAudioStandaloneFileName(numInstancesPair.first);

					if (szName != nullptr)
					{
						size_t const numInstances = numInstancesPair.second;

						if (numInstances == 1)
						{
							temp.Format("%s\n", szName);
						}
						else
						{
							temp.Format("%s: %" PRISIZE_T "\n", szName, numInstances);
						}

						controls += temp;
						temp.clear();
					}
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

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
