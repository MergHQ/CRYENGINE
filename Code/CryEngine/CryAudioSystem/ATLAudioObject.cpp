// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CATLAudioObject::CATLAudioObject(CryAudio::Impl::IAudioObject* const pImplData /*= nullptr*/)
	: m_pImplData(pImplData)
	, m_flags(eAudioObjectFlags_None)
	, m_maxRadius(0.0f)
	, m_previousVelocity(0.0f)
	, m_propagationProcessor(m_attributes.transformation)
	, m_occlusionFadeOutDistance(0.0f)
{
	m_propagationProcessor.Init(this);
}

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
	SAudioTriggerInstanceState& audioTriggerInstanceState = m_triggerStates.emplace(audioTriggerInstanceId, SAudioTriggerInstanceState()).first->second;
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
void CATLAudioObject::GetStartedStandaloneFileRequestData(CATLStandaloneFile const* const pStandaloneFile, CAudioRequestInternal& request)
{
	ObjectStandaloneFileMap::const_iterator const iter(m_activeStandaloneFiles.find(pStandaloneFile->GetId()));

	if (iter != m_activeStandaloneFiles.end())
	{
		SAudioStandaloneFileData const& audioStandaloneFileData = iter->second;
		request.pOwner = audioStandaloneFileData.pOwnerOverride;
		request.pUserData = audioStandaloneFileData.pUserData;
		request.pUserDataOwner = audioStandaloneFileData.pUserDataOwner;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedStandaloneFile(
  CATLStandaloneFile const* const pStandaloneFile,
  void* const pOwner,
  void* const pUserData,
  void* const pUserDataOwner)
{
	m_activeStandaloneFiles.emplace(pStandaloneFile->GetId(),
	                                SAudioStandaloneFileData(
	                                  pOwner,
	                                  pUserData,
	                                  pUserDataOwner,
	                                  pStandaloneFile->m_id));
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedStandaloneFile(CATLStandaloneFile const* const pStandaloneFile)
{
	m_activeStandaloneFiles.erase(pStandaloneFile->GetId());
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
void CATLAudioObject::SetParameter(AudioControlId const audioParameterId, float const value)
{
	m_parameters[audioParameterId] = value;
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
	m_parameters.clear();
	m_environments.clear();
	m_attributes = SAudioObject3DAttributes();
	m_flags = eAudioObjectFlags_None;
	m_maxRadius = 0.0f;
	m_occlusionFadeOutDistance = 0.0f;
	m_previousVelocity = 0.0f;
	m_previousTime.SetValue(0);
	m_velocity = ZERO;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedTriggerInstance(ObjectTriggerStates::iterator& iter)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
	SAudioRequest request;
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> requestData(audioTriggerInstanceState.audioTriggerId);
	request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ThreadSafePush | eAudioRequestFlags_SyncCallback;
	request.pAudioObject = this;
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
void CATLAudioObject::SetTransformation(CAudioObjectTransformation const& transformation)
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
		SetTransformation(transformation);
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

			request.pAudioObject = this;
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

			request.pAudioObject = this;
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

				request.pAudioObject = this;
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

			request.pAudioObject = this;
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
void CATLAudioObject::CStateDebugDrawData::Update(AudioSwitchStateId const audioSwitchState)
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

typedef std::map<AudioControlId const, size_t, std::less<AudioControlId const>, STLSoundAllocator<std::pair<AudioControlId const, size_t>>>                      TriggerCountMap;
typedef std::map<AudioStandaloneFileId const, size_t, std::less<AudioStandaloneFileId const>, STLSoundAllocator<std::pair<AudioStandaloneFileId const, size_t>>> AudioStandaloneFileCountMap;

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::DrawDebugInfo(
  IRenderAuxGeom& auxGeom,
  Vec3 const& listenerPosition,
  AudioTriggerLookup const& triggers,
  AudioRtpcLookup const& parameters,
  AudioSwitchLookup const& switches,
  AudioPreloadRequestLookup const& preloadRequests,
  AudioEnvironmentLookup const& environments,
  AudioStandaloneFileLookup const& audioStandaloneFiles) const
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

			CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> temp;

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_LABEL) > 0)
			{
				static float const objectTextColor[4] = { 0.90f, 0.90f, 0.90f, 1.0f };
				static float const objectGrayTextColor[4] = { 0.50f, 0.50f, 0.50f, 1.0f };

				EAudioOcclusionType const occlusionType = m_propagationProcessor.GetOcclusionType();
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

			if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_OBJECT_RTPCS) > 0 && !m_parameters.empty())
			{
				Vec3 rtpcPos(screenPos);

				for (auto const& parameterPair : m_parameters)
				{
					CATLRtpc const* const pParameter = stl::find_in_map(parameters, parameterPair.first, nullptr);

					if (pParameter != nullptr)
					{
						float const offsetOnX = (static_cast<float>(pParameter->m_name.size()) + 5.6f) * 5.4f * fontSize;
						rtpcPos.y -= lineHeight;
						auxGeom.Draw2dLabel(
						  rtpcPos.x - offsetOnX,
						  rtpcPos.y,
						  fontSize,
						  textColor, false,
						  "%s: %2.2f\n",
						  pParameter->m_name.c_str(),
						  parameterPair.second);
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

			CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> controls;

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
				AudioStandaloneFileCountMap numStandaloneFiles;

				for (auto const& standaloneFilePair : m_activeStandaloneFiles)
				{
					++(numStandaloneFiles[standaloneFilePair.second.m_fileId]);
				}

				for (auto const& numInstancesPair : numStandaloneFiles)
				{
					CATLStandaloneFile* const pAudioStandaloneFile = stl::find_in_map(audioStandaloneFiles, numInstancesPair.first, nullptr);

					if (pAudioStandaloneFile != nullptr)
					{
						size_t const numInstances = numInstancesPair.second;

						if (numInstances == 1)
						{
							temp.Format("%s\n", pAudioStandaloneFile->m_name.c_str());
						}
						else
						{
							temp.Format("%s: %" PRISIZE_T "\n", pAudioStandaloneFile->m_name.c_str(), numInstances);
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
