// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "AudioEventManager.h"
#include "AudioSystem.h"
#include "AudioStandaloneFileManager.h"
#include "Common/Logger.h"
#include "Common.h"
#include <IAudioImpl.h>
#include <CryString/HashedString.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryMath/Cry_Camera.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CATLAudioObject::CATLAudioObject()
	: m_pImplData(nullptr)
	, m_maxRadius(0.0f)
	, m_flags(EObjectFlags::InUse)
	, m_previousRelativeVelocity(0.0f)
	, m_previousAbsoluteVelocity(0.0f)
	, m_positionForVelocityCalculation(ZERO)
	, m_previousPositionForVelocityCalculation(ZERO)
	, m_velocity(ZERO)
	, m_propagationProcessor(m_transformation)
	, m_entityId(INVALID_ENTITYID)
	, m_numPendingSyncCallbacks(0)
{}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Release()
{
	// Do not clear the object's name though!
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
void CATLAudioObject::AddEvent(CATLEvent* const pEvent)
{
	m_activeEvents.insert(pEvent);
	m_triggerImplStates.emplace(pEvent->m_audioTriggerImplId, SAudioTriggerImplState());

	// Set the max activity radius of all active events on this object.
	m_maxRadius = std::max(pEvent->GetTriggerRadius(), m_maxRadius);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::AddTriggerState(TriggerInstanceId const id, SAudioTriggerInstanceState const& audioTriggerInstanceState)
{
	m_triggerStates.emplace(id, audioTriggerInstanceState);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::AddStandaloneFile(CATLStandaloneFile* const pStandaloneFile, SUserDataBase const& userDataBase)
{
	m_activeStandaloneFiles.emplace(pStandaloneFile, userDataBase);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SendFinishedTriggerInstanceRequest(SAudioTriggerInstanceState const& audioTriggerInstanceState)
{
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedTriggerInstance> requestData(audioTriggerInstanceState.triggerId);
	CAudioRequest request(&requestData);
	request.pObject = this;
	request.pOwner = audioTriggerInstanceState.pOwnerOverride;
	request.pUserData = audioTriggerInstanceState.pUserData;
	request.pUserDataOwner = audioTriggerInstanceState.pUserDataOwner;

	if ((audioTriggerInstanceState.flags & ETriggerStatus::CallbackOnExternalThread) != 0)
	{
		request.flags = ERequestFlags::CallbackOnExternalOrCallingThread;
	}
	else if ((audioTriggerInstanceState.flags & ETriggerStatus::CallbackOnAudioThread) != 0)
	{
		request.flags = ERequestFlags::CallbackOnAudioThread;
	}

	g_pSystem->PushRequest(request);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportStartedEvent(CATLEvent* const pEvent)
{
	CRY_ASSERT_MESSAGE(m_activeEvents.find(pEvent) == m_activeEvents.end(), "An event was found in active list during CATLAudioObject::ReportStartedEvent!");

	AddEvent(pEvent);
	ObjectTriggerStates::iterator const iter(m_triggerStates.find(pEvent->m_audioTriggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;

		switch (pEvent->m_state)
		{
		case EEventState::Playing:
			{
				++(audioTriggerInstanceState.numPlayingEvents);

				break;
			}
		case EEventState::PlayingDelayed:
			{
				CRY_ASSERT(audioTriggerInstanceState.numLoadingEvents > 0);
				--(audioTriggerInstanceState.numLoadingEvents);
				++(audioTriggerInstanceState.numPlayingEvents);
				pEvent->m_state = EEventState::Playing;

				break;
			}
		case EEventState::Loading:
			{
				++(audioTriggerInstanceState.numLoadingEvents);

				break;
			}
		case EEventState::Unloading:
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
void CATLAudioObject::ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess)
{
	// If this assert fires we most likely have it ending before it was pushed into the active events list during trigger execution.
	CRY_ASSERT_MESSAGE(m_activeEvents.find(pEvent) != m_activeEvents.end(), "An event was not found in active list during CATLAudioObject::ReportFinishedEvent!");

	m_activeEvents.erase(pEvent);
	m_triggerImplStates.erase(pEvent->m_audioTriggerImplId);

	// Recalculate the max activity radius.
	m_maxRadius = 0.0f;

	for (auto const pActiveEvent : m_activeEvents)
	{
		m_maxRadius = std::max(pActiveEvent->GetTriggerRadius(), m_maxRadius);
	}

	ObjectTriggerStates::iterator iter(m_triggerStates.begin());

	if (FindPlace(m_triggerStates, pEvent->m_audioTriggerInstanceId, iter))
	{
		switch (pEvent->m_state)
		{
		case EEventState::Playing:
			{
				SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
				CRY_ASSERT_MESSAGE(audioTriggerInstanceState.numPlayingEvents > 0, "Number of playing events must be at least 1 during CATLAudioObject::ReportFinishedEvent!");

				if (--(audioTriggerInstanceState.numPlayingEvents) == 0 && audioTriggerInstanceState.numLoadingEvents == 0)
				{
					ReportFinishedTriggerInstance(iter);
				}

				break;
			}
		case EEventState::Loading:
			{
				if (bSuccess)
				{
					ReportFinishedLoadingTriggerImpl(pEvent->m_audioTriggerImplId, true);
				}

				break;
			}
		case EEventState::Unloading:
			{
				if (bSuccess)
				{
					ReportFinishedLoadingTriggerImpl(pEvent->m_audioTriggerImplId, false);
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
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Reported finished event on an inactive trigger %s", pEvent->GetTriggerName());
	}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
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
void CATLAudioObject::ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	m_activeStandaloneFiles.erase(pStandaloneFile);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedLoadingTriggerImpl(TriggerImplId const audioTriggerImplId, bool const bLoad)
{
	if (bLoad)
	{
		m_triggerImplStates[audioTriggerImplId].flags |= ETriggerStatus::Loaded;
	}
	else
	{
		m_triggerImplStates[audioTriggerImplId].flags &= ~ETriggerStatus::Loaded;
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleStopTrigger(CTrigger const* const pTrigger)
{
	for (auto const pEvent : m_activeEvents)
	{
		if ((pEvent != nullptr) && pEvent->IsPlaying() && (pEvent->GetTriggerId() == pTrigger->GetId()))
		{
			pEvent->Stop();
		}
	}

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetSwitchState(CATLSwitch const* const pSwitch, CATLSwitchState const* const pState)
{
	for (auto const pSwitchStateImpl : pState->m_implPtrs)
	{
		pSwitchStateImpl->Set(*this);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	StoreSwitchValue(pSwitch->GetId(), pState->GetId());
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetEnvironment(CATLAudioEnvironment const* const pEnvironment, float const value)
{
	for (auto const pEnvImpl : pEnvironment->m_implPtrs)
	{
		m_pImplData->SetEnvironment(pEnvImpl->m_pImplData, value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	StoreEnvironmentValue(pEnvironment->GetId(), value);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopAllTriggers()
{
	m_pImplData->StopAllTriggers();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	m_pImplData->SetObstructionOcclusion(obstruction, occlusion);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReportFinishedTriggerInstance(ObjectTriggerStates::iterator const& iter)
{
	SAudioTriggerInstanceState& audioTriggerInstanceState = iter->second;
	SendFinishedTriggerInstanceRequest(audioTriggerInstanceState);

	if ((audioTriggerInstanceState.flags & ETriggerStatus::Loaded) != 0)
	{
		// If the trigger instance was manually loaded -- keep it
		audioTriggerInstanceState.flags &= ~ETriggerStatus::Playing;
	}
	else
	{
		// If the trigger instance wasn't loaded -- kill it
		m_triggerStates.erase(iter);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::PushRequest(SAudioRequestData const& requestData, SRequestUserData const& userData)
{
	CAudioRequest const request(userData.flags, this, userData.pOwner, userData.pUserData, userData.pUserDataOwner, &requestData);
	g_pSystem->PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::HasActiveData(CATLAudioObject const* const pAudioObject) const
{
	for (auto const pEvent : pAudioObject->GetActiveEvents())
	{
		if (pEvent->IsPlaying())
		{
			return true;
		}
	}

	for (auto const& standaloneFilePair : pAudioObject->GetActiveStandaloneFiles())
	{
		CATLStandaloneFile const* const pStandaloneFile = standaloneFilePair.first;

		if (pStandaloneFile->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Update(
	float const deltaTime,
	float const distanceToListener,
	Vec3 const& listenerPosition,
	Vec3 const& listenerVelocity,
	bool const listenerMoved)
{
	m_propagationProcessor.Update(distanceToListener, listenerPosition, m_flags);

	if (m_propagationProcessor.HasNewOcclusionValues())
	{
		SATLSoundPropagationData propagationData;
		m_propagationProcessor.GetPropagationData(propagationData);
		m_pImplData->SetObstructionOcclusion(propagationData.obstruction, propagationData.occlusion);
	}

	UpdateControls(deltaTime, distanceToListener, listenerPosition, listenerVelocity, listenerMoved);
	m_pImplData->Update();
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo)
{
	m_propagationProcessor.ProcessPhysicsRay(pAudioRayInfo);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetTransformation(CObjectTransformation const& transformation, float const distanceToListener)
{
	if ((m_flags & (EObjectFlags::TrackAbsoluteVelocity | EObjectFlags::TrackRelativeVelocity)) != 0)
	{
		m_positionForVelocityCalculation = transformation.GetPosition();
		m_flags |= EObjectFlags::MovingOrDecaying;
	}

	float const threshold = distanceToListener * g_cvars.m_positionUpdateThresholdMultiplier;

	if (!m_transformation.IsEquivalent(transformation, threshold))
	{
		m_transformation = transformation;

		// Immediately propagate the new transformation down to the middleware to prevent executing a trigger before its transformation was set.
		// Calculation of potentially tracked absolute and relative velocities can be safely delayed to next audio frame.
		m_pImplData->SetTransformation(m_transformation);
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetOcclusionType(EOcclusionType const calcType, Vec3 const& listenerPosition)
{
	CRY_ASSERT(calcType != EOcclusionType::None);
	m_propagationProcessor.SetOcclusionType(calcType, listenerPosition);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ReleasePendingRays()
{
	m_propagationProcessor.ReleasePendingRays();
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleStopFile(char const* const szFile)
{
	CHashedString const hashedFilename(szFile);
	auto iter = m_activeStandaloneFiles.cbegin();
	auto iterEnd = m_activeStandaloneFiles.cend();

	while (iter != iterEnd)
	{
		CATLStandaloneFile* const pFile = iter->first;

		if (pFile != nullptr && pFile->m_hashedFilename == hashedFilename)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (pFile->m_state != EAudioStandaloneFileState::Playing)
			{
				char const* szState = "unknown";

				switch (pFile->m_state)
				{
				case EAudioStandaloneFileState::Playing:
					szState = "playing";
					break;
				case EAudioStandaloneFileState::Loading:
					szState = "loading";
					break;
				case EAudioStandaloneFileState::Stopping:
					szState = "stopping";
					break;
				default:
					szState = "unknown";
					break;
				}

				Cry::Audio::Log(ELogType::Warning, R"(Request to stop a standalone audio file that is not playing! State: "%s")", szState);
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			ERequestStatus const status = m_pImplData->StopFile(pFile->m_pImplData);

			if (status != ERequestStatus::Pending)
			{
				ReportFinishedStandaloneFile(pFile);
				g_pFileManager->ReleaseStandaloneFile(pFile);

				iter = m_activeStandaloneFiles.begin();
				iterEnd = m_activeStandaloneFiles.end();
				continue;
			}
			else
			{
				pFile->m_state = EAudioStandaloneFileState::Stopping;
			}
		}

		++iter;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Init(char const* const szName, Impl::IObject* const pImplData, Vec3 const& listenerPosition, EntityId entityId)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_name = szName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_entityId = entityId;
	m_pImplData = pImplData;
	m_propagationProcessor.Init(this, listenerPosition);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::UpdateControls(
	float const deltaTime,
	float const distanceToListener,
	Vec3 const& listenerPosition,
	Vec3 const& listenerVelocity,
	bool const listenerMoved)
{
	if ((m_flags& EObjectFlags::MovingOrDecaying) != 0)
	{
		Vec3 const deltaPos(m_positionForVelocityCalculation - m_previousPositionForVelocityCalculation);

		if (!deltaPos.IsZero())
		{
			m_velocity = deltaPos / deltaTime;
			m_previousPositionForVelocityCalculation = m_positionForVelocityCalculation;
		}
		else if (!m_velocity.IsZero())
		{
			// We did not move last frame, begin exponential decay towards zero.
			float const decay = std::max(1.0f - deltaTime / 0.05f, 0.0f);
			m_velocity *= decay;

			if (m_velocity.GetLengthSquared() < FloatEpsilon)
			{
				m_velocity = ZERO;
				m_flags &= ~EObjectFlags::MovingOrDecaying;
			}
		}

		if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
		{
			float const absoluteVelocity = m_velocity.GetLength();

			if (absoluteVelocity == 0.0f || fabs(absoluteVelocity - m_previousAbsoluteVelocity) > g_cvars.m_velocityTrackingThreshold)
			{
				m_previousAbsoluteVelocity = absoluteVelocity;
				g_pAbsoluteVelocityParameter->Set(*this, absoluteVelocity);
			}
		}

		if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
		{
			// Approaching positive, departing negative value.
			float relativeVelocity = 0.0f;

			if ((m_flags& EObjectFlags::MovingOrDecaying) != 0 && !listenerMoved)
			{
				relativeVelocity = -m_velocity.Dot((m_positionForVelocityCalculation - listenerPosition).GetNormalized());
			}
			else if ((m_flags& EObjectFlags::MovingOrDecaying) != 0 && listenerMoved)
			{
				Vec3 const relativeVelocityVec(m_velocity - listenerVelocity);
				relativeVelocity = -relativeVelocityVec.Dot((m_positionForVelocityCalculation - listenerPosition).GetNormalized());
			}

			TryToSetRelativeVelocity(relativeVelocity);
		}
	}
	else if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
	{
		// Approaching positive, departing negative value.
		if (listenerMoved)
		{
			float const relativeVelocity = listenerVelocity.Dot((m_positionForVelocityCalculation - listenerPosition).GetNormalized());
			TryToSetRelativeVelocity(relativeVelocity);
		}
		else if (m_previousRelativeVelocity != 0.0f)
		{
			TryToSetRelativeVelocity(0.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::TryToSetRelativeVelocity(float const relativeVelocity)
{
	if (relativeVelocity == 0.0f || fabs(relativeVelocity - m_previousRelativeVelocity) > g_cvars.m_velocityTrackingThreshold)
	{
		m_previousRelativeVelocity = relativeVelocity;
		g_pRelativeVelocityParameter->Set(*this, relativeVelocity);
	}
}

///////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::CanBeReleased() const
{
	return (m_flags& EObjectFlags::InUse) == 0 &&
	       m_activeEvents.empty() &&
	       m_activeStandaloneFiles.empty() &&
	       !m_propagationProcessor.HasPendingRays() &&
	       m_numPendingSyncCallbacks == 0;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetFlag(EObjectFlags const flag)
{
	m_flags |= flag;
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::RemoveFlag(EObjectFlags const flag)
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

//////////////////////////////////////////////////////////////////////////
char const* CATLAudioObject::GetDefaultParameterName(ControlId const id) const
{
	char const* szName = nullptr;

	switch (id)
	{
	case AbsoluteVelocityParameterId:
		szName = g_pAbsoluteVelocityParameter->GetName();
		break;
	case RelativeVelocityParameterId:
		szName = g_pRelativeVelocityParameter->GetName();
		break;
	default:
		CRY_ASSERT_MESSAGE(false, R"(The default parameter "%u" does not exist.)", id);
		break;
	}

	return szName;
}

//////////////////////////////////////////////////////////////////////////
char const* CATLAudioObject::GetDefaultTriggerName(ControlId const id) const
{
	char const* szName = nullptr;

	switch (id)
	{
	case LoseFocusTriggerId:
		szName = g_pLoseFocusTrigger->GetName();
		break;
	case GetFocusTriggerId:
		szName = g_pGetFocusTrigger->GetName();
		break;
	case MuteAllTriggerId:
		szName = g_pMuteAllTrigger->GetName();
		break;
	case UnmuteAllTriggerId:
		szName = g_pUnmuteAllTrigger->GetName();
		break;
	case PauseAllTriggerId:
		szName = g_pPauseAllTrigger->GetName();
		break;
	case ResumeAllTriggerId:
		szName = g_pResumeAllTrigger->GetName();
		break;
	default:
		CRY_ASSERT_MESSAGE(false, R"(The default trigger "%u" does not exist.)", id);
		break;
	}

	return szName;
}

using TriggerCountMap = std::map<ControlId const, size_t>;

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
	Vec3 const& position = m_transformation.GetPosition();
	Vec3 screenPos(ZERO);

	if (IRenderer* const pRenderer = gEnv->pRenderer)
	{
		auto const& camera = GetISystem()->GetViewCamera();
		pRenderer->ProjectToScreen(position.x, position.y, position.z, &screenPos.x, &screenPos.y, &screenPos.z);

		screenPos.x = screenPos.x * 0.01f * camera.GetViewSurfaceX();
		screenPos.y = screenPos.y * 0.01f * camera.GetViewSurfaceZ();
	}
	else
	{
		screenPos.z = -1.0f;
	}

	if ((screenPos.z >= 0.0f) && (screenPos.z <= 1.0f))
	{
		float const distance = position.GetDistance(listenerPosition);

		if ((g_cvars.m_debugDistance <= 0.0f) || ((g_cvars.m_debugDistance > 0.0f) && (distance <= g_cvars.m_debugDistance)))
		{
			float offsetOnY = 0.0f;

			// Check if text filter is enabled.
			CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
			lowerCaseSearchString.MakeLower();
			bool const isTextFilterDisabled = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0));
			bool const drawSphere = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::Spheres) != 0;
			bool const drawLabel = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectLabel) != 0;
			bool const drawTriggers = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectTriggers) != 0;
			bool const drawStandaloneFiles = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectStandaloneFiles) != 0;
			bool const drawStates = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectStates) != 0;
			bool const drawParameters = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectParameters) != 0;
			bool const drawEnvironments = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectEnvironments) != 0;
			bool const drawDistance = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectDistance) != 0;
			bool const drawOcclusionRayLabel = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::OcclusionRayLabels) != 0;
			bool const filterAllObjectInfo = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::FilterAllObjectInfo) != 0;

			// Check if any trigger matches text filter.
			bool doesTriggerMatchFilter = false;
			std::vector<CryFixedStringT<MaxMiscStringLength>> triggerInfo;

			if ((drawTriggers && !m_triggerStates.empty()) || filterAllObjectInfo)
			{
				TriggerCountMap triggerCounts;

				for (auto const& triggerStatesPair : m_triggerStates)
				{
					++(triggerCounts[triggerStatesPair.second.triggerId]);
				}

				for (auto const& triggerCountsPair : triggerCounts)
				{
					char const* szTriggerName = nullptr;
					auto const* const pTrigger = stl::find_in_map(triggers, triggerCountsPair.first, nullptr);

					if (pTrigger != nullptr)
					{
						szTriggerName = pTrigger->GetName();
					}
					else
					{
						szTriggerName = GetDefaultTriggerName(triggerCountsPair.first);
					}

					CRY_ASSERT_MESSAGE(szTriggerName != nullptr, "The trigger name mustn't be nullptr during CATLAudioObject::DrawDebugInfo.");

					if (!isTextFilterDisabled)
					{
						CryFixedStringT<MaxControlNameLength> lowerCaseTriggerName(szTriggerName);
						lowerCaseTriggerName.MakeLower();

						if (lowerCaseTriggerName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
						{
							doesTriggerMatchFilter = true;
						}
					}

					CryFixedStringT<MaxMiscStringLength> debugText;
					size_t const numInstances = triggerCountsPair.second;

					if (numInstances == 1)
					{
						debugText.Format("%s\n", szTriggerName);
					}
					else
					{
						debugText.Format("%s: %" PRISIZE_T "\n", szTriggerName, numInstances);
					}

					triggerInfo.emplace_back(debugText);
				}
			}

			// Check if any standalone file matches text filter.
			bool doesStandaloneFileMatchFilter = false;
			std::vector<CryFixedStringT<MaxMiscStringLength>> standaloneFileInfo;

			if ((drawStandaloneFiles && !m_activeStandaloneFiles.empty()) || filterAllObjectInfo)
			{
				std::map<CHashedString, size_t> numStandaloneFiles;

				for (auto const& standaloneFilePair : m_activeStandaloneFiles)
				{
					++(numStandaloneFiles[standaloneFilePair.first->m_hashedFilename]);
				}

				for (auto const& numInstancesPair : numStandaloneFiles)
				{
					char const* const szStandaloneFileName = numInstancesPair.first.GetText().c_str();

					if (!isTextFilterDisabled)
					{
						CryFixedStringT<MaxControlNameLength> lowerCaseStandaloneFileName(szStandaloneFileName);
						lowerCaseStandaloneFileName.MakeLower();

						if (lowerCaseStandaloneFileName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
						{
							doesStandaloneFileMatchFilter = true;
						}
					}

					CryFixedStringT<MaxMiscStringLength> debugText;
					size_t const numInstances = numInstancesPair.second;

					if (numInstances == 1)
					{
						debugText.Format("%s\n", szStandaloneFileName);
					}
					else
					{
						debugText.Format("%s: %" PRISIZE_T "\n", szStandaloneFileName, numInstances);
					}

					standaloneFileInfo.emplace_back(debugText);
				}
			}

			// Check if any state or switch matches text filter.
			bool doesStateSwitchMatchFilter = false;
			std::map<CATLSwitch const* const, CATLSwitchState const* const> switchStateInfo;

			if ((drawStates && !m_switchStates.empty()) || filterAllObjectInfo)
			{
				for (auto const& switchStatePair : m_switchStates)
				{
					CATLSwitch const* const pSwitch = stl::find_in_map(switches, switchStatePair.first, nullptr);

					if (pSwitch != nullptr)
					{
						CATLSwitchState const* const pSwitchState = stl::find_in_map(pSwitch->audioSwitchStates, switchStatePair.second, nullptr);

						if (pSwitchState != nullptr)
						{
							if (!isTextFilterDisabled)
							{
								char const* const szSwitchName = pSwitch->GetName();
								CryFixedStringT<MaxControlNameLength> lowerCaseSwitchName(szSwitchName);
								lowerCaseSwitchName.MakeLower();
								char const* const szStateName = pSwitchState->GetName();
								CryFixedStringT<MaxControlNameLength> lowerCaseStateName(szStateName);
								lowerCaseStateName.MakeLower();

								if ((lowerCaseSwitchName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
								    (lowerCaseStateName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos))
								{
									doesStateSwitchMatchFilter = true;
								}
							}

							switchStateInfo.emplace(pSwitch, pSwitchState);
						}
					}
				}
			}

			// Check if any parameter matches text filter.
			bool doesParameterMatchFilter = false;
			std::map<char const* const, float const> parameterInfo;

			if ((drawParameters && !m_parameters.empty()) || filterAllObjectInfo)
			{
				for (auto const& parameterPair : m_parameters)
				{
					char const* szParameterName = nullptr;
					auto const* const pParameter = stl::find_in_map(parameters, parameterPair.first, nullptr);

					if (pParameter != nullptr)
					{
						szParameterName = pParameter->GetName();
					}
					else
					{
						szParameterName = GetDefaultParameterName(parameterPair.first);
					}

					CRY_ASSERT_MESSAGE(szParameterName != nullptr, "The parameter name mustn't be nullptr during CATLAudioObject::DrawDebugInfo.");

					if (!isTextFilterDisabled)
					{
						CryFixedStringT<MaxControlNameLength> lowerCaseParameterName(szParameterName);
						lowerCaseParameterName.MakeLower();

						if (lowerCaseParameterName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
						{
							doesParameterMatchFilter = true;
						}
					}

					parameterInfo.emplace(szParameterName, parameterPair.second);
				}
			}

			// Check if any environment matches text filter.
			bool doesEnvironmentMatchFilter = false;
			std::map<char const* const, float const> environmentInfo;

			if ((drawEnvironments && !m_environments.empty()) || filterAllObjectInfo)
			{
				for (auto const& environmentPair : m_environments)
				{
					CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(environments, environmentPair.first, nullptr);

					if (pEnvironment != nullptr)
					{
						char const* const szEnvironmentName = pEnvironment->GetName();

						if (!isTextFilterDisabled)
						{
							CryFixedStringT<MaxControlNameLength> lowerCaseEnvironmentName(szEnvironmentName);
							lowerCaseEnvironmentName.MakeLower();

							if (lowerCaseEnvironmentName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
							{
								doesEnvironmentMatchFilter = true;
							}
						}

						environmentInfo.emplace(szEnvironmentName, environmentPair.second);
					}
				}
			}

			// Check if object name matches text filter.
			auto const pAudioObject = const_cast<CATLAudioObject*>(this);
			char const* const szObjectName = pAudioObject->m_name.c_str();
			bool doesObjectNameMatchFilter = false;

			if (!isTextFilterDisabled && (drawLabel || filterAllObjectInfo))
			{
				CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
				lowerCaseObjectName.MakeLower();
				doesObjectNameMatchFilter = (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
			}

			// Check if any object info text matches text filter.
			bool const drawObjectDebugInfo =
				isTextFilterDisabled ||
				doesObjectNameMatchFilter ||
				doesTriggerMatchFilter ||
				doesStandaloneFileMatchFilter ||
				doesStateSwitchMatchFilter ||
				doesParameterMatchFilter ||
				doesEnvironmentMatchFilter;

			if (drawObjectDebugInfo)
			{
				if (drawSphere)
				{
					SAuxGeomRenderFlags const previousRenderFlags = auxGeom.GetRenderFlags();
					SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
					newRenderFlags.SetCullMode(e_CullModeNone);
					auxGeom.SetRenderFlags(newRenderFlags);
					auxGeom.DrawSphere(position, Debug::g_objectRadiusPositionSphere, ColorB(255, 1, 1, 255));
					auxGeom.SetRenderFlags(previousRenderFlags);
				}

				if (drawLabel)
				{
					bool const hasActiveData = HasActiveData(pAudioObject);
					bool const draw = (g_cvars.m_hideInactiveAudioObjects == 0) || ((g_cvars.m_hideInactiveAudioObjects > 0) && hasActiveData);

					if (draw)
					{
						bool const isVirtual = (pAudioObject->GetFlags() & EObjectFlags::Virtual) != 0;

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							isVirtual ? Debug::g_objectColorVirtual.data() : (hasActiveData ? Debug::g_objectColorActive.data() : Debug::g_objectColorInactive.data()),
							false,
							"%s",
							szObjectName);

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawTriggers && !triggerInfo.empty())
				{
					for (auto const& debugText : triggerInfo)
					{
						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							Debug::g_objectColorTrigger.data(),
							false,
							"%s",
							debugText.c_str());

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawStandaloneFiles && !standaloneFileInfo.empty())
				{
					for (auto const& debugText : standaloneFileInfo)
					{
						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							Debug::g_objectColorStandaloneFile.data(),
							false,
							"%s",
							debugText.c_str());

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawStates && !switchStateInfo.empty())
				{
					for (auto const& switchStatePair : switchStateInfo)
					{
						auto const pSwitch = switchStatePair.first;
						auto const pSwitchState = switchStatePair.second;

						CStateDebugDrawData& drawData = m_stateDrawInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(pSwitch->GetId()), std::forward_as_tuple(pSwitchState->GetId())).first->second;
						drawData.Update(pSwitchState->GetId());
						float const switchTextColor[4] = { 0.8f, 0.3f, 0.6f, drawData.m_currentAlpha };

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							switchTextColor,
							false,
							"%s: %s\n",
							pSwitch->GetName(),
							pSwitchState->GetName());

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawParameters && !parameterInfo.empty())
				{
					for (auto const& parameterPair : parameterInfo)
					{
						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							Debug::g_objectColorParameter.data(),
							false,
							"%s: %2.2f\n",
							parameterPair.first,
							parameterPair.second);

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawEnvironments && !environmentInfo.empty())
				{
					for (auto const& environmentPair : environmentInfo)
					{
						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							Debug::g_objectColorEnvironment.data(),
							false,
							"%s: %.2f\n",
							environmentPair.first,
							environmentPair.second);

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				if (drawDistance)
				{
					bool const isVirtual = (pAudioObject->GetFlags() & EObjectFlags::Virtual) != 0;

					auxGeom.Draw2dLabel(
						screenPos.x,
						screenPos.y + offsetOnY,
						Debug::g_objectFontSize,
						isVirtual ? Debug::g_objectColorVirtual.data() : Debug::g_objectColorActive.data(),
						false,
						"Dist: %4.1fm",
						distance);

					offsetOnY += Debug::g_objectLineHeight;
				}

				if (drawOcclusionRayLabel)
				{
					EOcclusionType const occlusionType = m_propagationProcessor.GetOcclusionType();
					SATLSoundPropagationData propagationData;
					m_propagationProcessor.GetPropagationData(propagationData);

					bool const hasActiveData = HasActiveData(pAudioObject);
					bool const draw = (g_cvars.m_hideInactiveAudioObjects == 0) || ((g_cvars.m_hideInactiveAudioObjects > 0) && hasActiveData);

					if (draw)
					{
						CryFixedStringT<MaxMiscStringLength> debugText;

						if (distance < g_cvars.m_occlusionMaxDistance)
						{
							if (occlusionType == EOcclusionType::Adaptive)
							{
								debugText.Format(
									"%s(%s)",
									s_szOcclusionTypes[IntegralValue(occlusionType)],
									s_szOcclusionTypes[IntegralValue(m_propagationProcessor.GetOcclusionTypeWhenAdaptive())]);
							}
							else
							{
								debugText.Format("%s", s_szOcclusionTypes[IntegralValue(occlusionType)]);
							}
						}
						else
						{
							debugText.Format("Ignore (exceeded activity range)");
						}

						bool const isVirtual = (pAudioObject->GetFlags() & EObjectFlags::Virtual) != 0;
						float const activeRayLabelColor[4] = { propagationData.occlusion, 1.0f - propagationData.occlusion, 0.0f, 0.9f };

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y + offsetOnY,
							Debug::g_objectFontSize,
							((occlusionType != EOcclusionType::None) && (occlusionType != EOcclusionType::Ignore)) ? (isVirtual ? Debug::g_objectColorVirtual.data() : activeRayLabelColor) : Debug::g_objectColorInactive.data(),
							false,
							"Occl: %3.2f | Type: %s", // Add obstruction again once the engine supports it.
							propagationData.occlusion,
							debugText.c_str());

						offsetOnY += Debug::g_objectLineHeight;
					}
				}

				m_propagationProcessor.DrawDebugInfo(auxGeom, m_flags, listenerPosition);
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
		m_pImplData->SetTransformation(m_transformation);
	}

	// Parameters
	for (auto const& parameterPair : m_parameters)
	{
		auto const* const pParameter = stl::find_in_map(parameters, parameterPair.first, nullptr);

		if (pParameter != nullptr)
		{
			pParameter->Set(*this, parameterPair.second);
		}
		else if (!SetDefaultParameterValue(parameterPair.first, parameterPair.second))
		{
			Cry::Audio::Log(ELogType::Warning, "Parameter \"%u\" does not exist!", parameterPair.first);
		}
	}

	// Switches
	for (auto const& switchPair : m_switchStates)
	{
		CATLSwitch const* const pSwitch = stl::find_in_map(switches, switchPair.first, nullptr);

		if (pSwitch != nullptr)
		{
			CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, switchPair.second, nullptr);

			if (pState != nullptr)
			{
				HandleSetSwitchState(pSwitch, pState);
			}
		}
	}

	// Environments
	for (auto const& environmentPair : m_environments)
	{
		CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(environments, environmentPair.first, nullptr);

		if (pEnvironment != nullptr)
		{
			HandleSetEnvironment(pEnvironment, environmentPair.second);
		}
	}

	// Last re-execute its active triggers and standalone files.
	for (auto& triggerStatePair : m_triggerStates)
	{
		auto const* const pTrigger = stl::find_in_map(triggers, triggerStatePair.second.triggerId, nullptr);

		if (pTrigger != nullptr)
		{
			pTrigger->Execute(*this, triggerStatePair.first, triggerStatePair.second);
		}
		else if (!ExecuteDefaultTrigger(triggerStatePair.second.triggerId))
		{
			Cry::Audio::Log(ELogType::Warning, "Trigger \"%u\" does not exist!", triggerStatePair.second.triggerId);
		}
	}

	ObjectStandaloneFileMap const& activeStandaloneFiles = m_activeStandaloneFiles;

	for (auto const& standaloneFilePair : activeStandaloneFiles)
	{
		CATLStandaloneFile* const pFile = standaloneFilePair.first;
		CRY_ASSERT_MESSAGE(pFile != nullptr, "Standalone file pointer is nullptr during CATLAudioObject::ForceImplementationRefresh!");

		if (pFile != nullptr)
		{
			CRY_ASSERT_MESSAGE(pFile->m_state == EAudioStandaloneFileState::Playing, "Standalone file must be in playing state during CATLAudioObject::ForceImplementationRefresh!");
			CRY_ASSERT_MESSAGE(pFile->m_pAudioObject == this, "Standalone file played on wrong object during CATLAudioObject::ForceImplementationRefresh!");

			auto const* const pTrigger = stl::find_in_map(triggers, pFile->m_triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->PlayFile(*this, pFile);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CATLAudioObject::HandleSetName(char const* const szName)
{
	m_name = szName;
	return m_pImplData->SetName(szName);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StoreParameterValue(ControlId const id, float const value)
{
	m_parameters[id] = value;
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId)
{
	m_switchStates[switchId] = switchStateId;
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StoreEnvironmentValue(ControlId const id, float const value)
{
	m_environments[id] = value;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> requestData(triggerId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (triggerId != InvalidControlId)
	{
		SAudioObjectRequestData<EAudioObjectRequestType::StopTrigger> requestData(triggerId);
		PushRequest(requestData, userData);
	}
	else
	{
		SAudioObjectRequestData<EAudioObjectRequestType::StopAllTriggers> requestData;
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetTransformation(CObjectTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetTransformation> requestData(transformation);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetParameter> requestData(parameterId, value);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetSwitchState> requestData(audioSwitchId, audioSwitchStateId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetEnvironment(EnvironmentId const audioEnvironmentId, float const amount, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetEnvironment> requestData(audioEnvironmentId, amount);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetCurrentEnvironments(EntityId const entityToIgnore, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetCurrentEnvironments> requestData(entityToIgnore);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (occlusionType < EOcclusionType::Count)
	{
		SAudioObjectRequestData<EAudioObjectRequestType::SetSwitchState> requestData(OcclusionTypeSwitchId, OcclusionTypeStateIds[IntegralValue(occlusionType)]);
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopFile(char const* const szFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::StopFile> requestData(szFile);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetName(char const* const szName, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetName> requestData(szName);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::SetDefaultParameterValue(ControlId const id, float const value) const
{
	CRY_ASSERT_MESSAGE(this != g_pObject, "CATLAudioObject::SetDefaultParameterValue mustn't happen on the global object!");

	bool wasSuccess = true;

	switch (id)
	{
	case AbsoluteVelocityParameterId:
		g_pAbsoluteVelocityParameter->Set(*this, value);
		break;
	case RelativeVelocityParameterId:
		g_pRelativeVelocityParameter->Set(*this, value);
		break;
	default:
		wasSuccess = false;
		break;
	}

	return wasSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::ExecuteDefaultTrigger(ControlId const id)
{
	CRY_ASSERT_MESSAGE(this == g_pObject, "CATLAudioObject::ExecuteDefaultTrigger must happen only on the global object!");

	bool wasSuccess = true;

	switch (id)
	{
	case LoseFocusTriggerId:
		g_pLoseFocusTrigger->Execute();
		break;
	case GetFocusTriggerId:
		g_pGetFocusTrigger->Execute();
		break;
	case MuteAllTriggerId:
		g_pMuteAllTrigger->Execute();
		break;
	case UnmuteAllTriggerId:
		g_pUnmuteAllTrigger->Execute();
		break;
	case PauseAllTriggerId:
		g_pPauseAllTrigger->Execute();
		break;
	case ResumeAllTriggerId:
		g_pResumeAllTrigger->Execute();
		break;
	default:
		wasSuccess = false;
		break;
	}

	return wasSuccess;
}
} // namespace CryAudio
