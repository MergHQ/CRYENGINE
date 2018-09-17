// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "Managers.h"
#include "AudioEventManager.h"
#include "AudioSystem.h"
#include "AudioStandaloneFileManager.h"
#include "AudioListenerManager.h"
#include "Common/Logger.h"
#include <IAudioImpl.h>
#include <CryString/HashedString.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryMath/Cry_Camera.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "DebugColor.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CATLAudioObject::CATLAudioObject(CObjectTransformation const& transformation)
	: m_pImplData(nullptr)
	, m_transformation(transformation)
	, m_flags(EObjectFlags::InUse)
	, m_propagationProcessor(m_transformation, m_flags)
	, m_entityId(INVALID_ENTITYID)
	, m_numPendingSyncCallbacks(0)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	, m_maxRadius(0.0f)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Set the max activity radius of all active events on this object.
	m_maxRadius = std::max(pEvent->GetTriggerRadius(), m_maxRadius);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedTriggerInstance> requestData(audioTriggerInstanceState.triggerId);
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

	g_system.PushRequest(request);
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

	ObjectTriggerStates::iterator const iter(m_triggerStates.find(pEvent->m_audioTriggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		switch (pEvent->m_state)
		{
		case EEventState::Playing:
		case EEventState::Virtual:
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

	// Recalculate the max activity radius.
	m_maxRadius = 0.0f;

	for (auto const pActiveEvent : m_activeEvents)
	{
		m_maxRadius = std::max(pActiveEvent->GetTriggerRadius(), m_maxRadius);
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
		if ((pEvent != nullptr) && (pEvent->IsPlaying() || pEvent->IsVirtual()) && (pEvent->GetTriggerId() == pTrigger->GetId()))
		{
			pEvent->Stop();
		}
	}

	return ERequestStatus::Success;
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
void CATLAudioObject::PushRequest(SRequestData const& requestData, SRequestUserData const& userData)
{
	CAudioRequest const request(userData.flags, this, userData.pOwner, userData.pUserData, userData.pUserDataOwner, &requestData);
	g_system.PushRequest(request);
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
void CATLAudioObject::Update(float const deltaTime)
{
	m_propagationProcessor.Update();

	if (m_propagationProcessor.HasNewOcclusionValues())
	{
		SATLSoundPropagationData propagationData;
		m_propagationProcessor.GetPropagationData(propagationData);
		m_pImplData->SetObstructionOcclusion(propagationData.obstruction, propagationData.occlusion);
	}

	m_pImplData->Update(deltaTime);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo)
{
	m_propagationProcessor.ProcessPhysicsRay(pAudioRayInfo);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetTransformation(CObjectTransformation const& transformation)
{
	m_transformation = transformation;
	m_pImplData->SetTransformation(transformation);
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetOcclusionType(EOcclusionType const calcType)
{
	CRY_ASSERT(calcType != EOcclusionType::None);
	m_propagationProcessor.SetOcclusionType(calcType);
	m_pImplData->SetOcclusionType(calcType);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::HandleSetOcclusionRayOffset(float const offset)
{
	m_propagationProcessor.SetOcclusionRayOffset(offset);
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
			if (pFile->m_state != EStandaloneFileState::Playing)
			{
				char const* szState = "unknown";

				switch (pFile->m_state)
				{
				case EStandaloneFileState::Playing:
					szState = "playing";
					break;
				case EStandaloneFileState::Loading:
					szState = "loading";
					break;
				case EStandaloneFileState::Stopping:
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
				g_fileManager.ReleaseStandaloneFile(pFile);

				iter = m_activeStandaloneFiles.begin();
				iterEnd = m_activeStandaloneFiles.end();
				continue;
			}
			else
			{
				pFile->m_state = EStandaloneFileState::Stopping;
			}
		}

		++iter;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::Init(char const* const szName, Impl::IObject* const pImplData, EntityId const entityId)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_name = szName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_entityId = entityId;
	m_pImplData = pImplData;
	m_propagationProcessor.Init(this);
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
float const CATLAudioObject::CStateDebugDrawData::s_minSwitchColor = 0.3f;
float const CATLAudioObject::CStateDebugDrawData::s_maxSwitchColor = 1.0f;
int const CATLAudioObject::CStateDebugDrawData::s_maxToMinUpdates = 100;
static char const* const s_szOcclusionTypes[] = { "None", "Ignore", "Adaptive", "Low", "Medium", "High" };

//////////////////////////////////////////////////////////////////////////
char const* GetDefaultTriggerName(ControlId const id)
{
	char const* szName = nullptr;

	switch (id)
	{
	case LoseFocusTriggerId:
		szName = g_loseFocusTrigger.GetName();
		break;
	case GetFocusTriggerId:
		szName = g_getFocusTrigger.GetName();
		break;
	case MuteAllTriggerId:
		szName = g_muteAllTrigger.GetName();
		break;
	case UnmuteAllTriggerId:
		szName = g_unmuteAllTrigger.GetName();
		break;
	case PauseAllTriggerId:
		szName = g_pauseAllTrigger.GetName();
		break;
	case ResumeAllTriggerId:
		szName = g_resumeAllTrigger.GetName();
		break;
	case PreviewTriggerId:
		szName = g_previewTrigger.GetName();
		break;
	default:
		CRY_ASSERT_MESSAGE(false, R"(The default trigger "%u" does not exist.)", id);
		break;
	}

	return szName;
}

///////////////////////////////////////////////////////////////////////////
CATLAudioObject::CStateDebugDrawData::CStateDebugDrawData(SwitchStateId const audioSwitchState)
	: m_currentState(audioSwitchState)
	, m_currentSwitchColor(s_maxSwitchColor)
{
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::CStateDebugDrawData::Update(SwitchStateId const audioSwitchState)
{
	if ((audioSwitchState == m_currentState) && (m_currentSwitchColor > s_minSwitchColor))
	{
		m_currentSwitchColor -= (s_maxSwitchColor - s_minSwitchColor) / s_maxToMinUpdates;
	}
	else if (audioSwitchState != m_currentState)
	{
		m_currentState = audioSwitchState;
		m_currentSwitchColor = s_maxSwitchColor;
	}
}

using TriggerCountMap = std::map<ControlId const, size_t>;

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::DrawDebugInfo(IRenderAuxGeom& auxGeom)
{
	if (this != &g_previewObject)
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
			float const distance = position.GetDistance(g_listenerManager.GetActiveListenerTransformation().GetPosition());

			if ((g_cvars.m_debugDistance <= 0.0f) || ((g_cvars.m_debugDistance > 0.0f) && (distance <= g_cvars.m_debugDistance)))
			{
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
				bool const drawOcclusionRayOffset = (g_cvars.m_drawAudioDebug & Debug::EDrawFilter::OcclusionRayOffset) != 0;
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
						auto const* const pTrigger = stl::find_in_map(g_triggers, triggerCountsPair.first, nullptr);

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
						CATLSwitch const* const pSwitch = stl::find_in_map(g_switches, switchStatePair.first, nullptr);

						if (pSwitch != nullptr)
						{
							CATLSwitchState const* const pSwitchState = stl::find_in_map(pSwitch->GetStates(), switchStatePair.second, nullptr);

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

				if (drawParameters || filterAllObjectInfo)
				{
					for (auto const& parameterPair : m_parameters)
					{
						char const* szParameterName = nullptr;
						auto const* const pParameter = stl::find_in_map(g_parameters, parameterPair.first, nullptr);

						if (pParameter != nullptr)
						{
							szParameterName = pParameter->GetName();
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
						CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(g_environments, environmentPair.first, nullptr);

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
				auto const pObject = const_cast<CATLAudioObject*>(this);
				char const* const szObjectName = pObject->m_name.c_str();
				bool doesObjectNameMatchFilter = false;

				if (!isTextFilterDisabled && (drawLabel || filterAllObjectInfo))
				{
					CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
					lowerCaseObjectName.MakeLower();
					doesObjectNameMatchFilter = (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
				}

				bool const hasActiveData = HasActiveData(pObject);
				bool const canDraw = (g_cvars.m_hideInactiveAudioObjects == 0) || ((g_cvars.m_hideInactiveAudioObjects != 0) && hasActiveData);

				if (canDraw)
				{
					bool const isVirtual = (pObject->GetFlags() & EObjectFlags::Virtual) != 0;

					if (drawSphere)
					{
						auxGeom.DrawSphere(
							position,
							Debug::g_objectRadiusPositionSphere,
							isVirtual ? ColorB(25, 200, 200, 255) : ColorB(255, 1, 1, 255));
					}

					if (drawLabel && (isTextFilterDisabled || doesObjectNameMatchFilter))
					{
						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y,
							Debug::g_objectFontSize,
							isVirtual ? Debug::g_globalColorVirtual.data() : (hasActiveData ? Debug::g_objectColorActive.data() : Debug::g_globalColorInactive.data()),
							false,
							szObjectName);

						screenPos.y += Debug::g_objectLineHeight;
					}

					if (drawTriggers && (isTextFilterDisabled || doesTriggerMatchFilter))
					{
						for (auto const& debugText : triggerInfo)
						{
							auxGeom.Draw2dLabel(
								screenPos.x,
								screenPos.y,
								Debug::g_objectFontSize,
								isVirtual ? Debug::g_globalColorVirtual.data() : Debug::g_objectColorTrigger.data(),
								false,
								debugText.c_str());

							screenPos.y += Debug::g_objectLineHeight;
						}
					}

					if (drawStandaloneFiles && (isTextFilterDisabled || doesStandaloneFileMatchFilter))
					{
						for (auto const& debugText : standaloneFileInfo)
						{
							auxGeom.Draw2dLabel(
								screenPos.x,
								screenPos.y,
								Debug::g_objectFontSize,
								isVirtual ? Debug::g_globalColorVirtual.data() : Debug::g_objectColorStandaloneFile.data(),
								false,
								debugText.c_str());

							screenPos.y += Debug::g_objectLineHeight;
						}
					}

					if (drawStates && (isTextFilterDisabled || doesStateSwitchMatchFilter))
					{
						for (auto const& switchStatePair : switchStateInfo)
						{
							auto const pSwitch = switchStatePair.first;
							auto const pSwitchState = switchStatePair.second;

							CStateDebugDrawData& drawData = m_stateDrawInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(pSwitch->GetId()), std::forward_as_tuple(pSwitchState->GetId())).first->second;
							drawData.Update(pSwitchState->GetId());
							float const switchTextColor[4] = { 0.8f, drawData.m_currentSwitchColor, 0.6f, 1.0f };

							auxGeom.Draw2dLabel(
								screenPos.x,
								screenPos.y,
								Debug::g_objectFontSize,
								isVirtual ? Debug::g_globalColorVirtual.data() : switchTextColor,
								false,
								"%s: %s\n",
								pSwitch->GetName(),
								pSwitchState->GetName());

							screenPos.y += Debug::g_objectLineHeight;
						}
					}

					if (drawParameters && (isTextFilterDisabled || doesParameterMatchFilter))
					{
						for (auto const& parameterPair : parameterInfo)
						{
							auxGeom.Draw2dLabel(
								screenPos.x,
								screenPos.y,
								Debug::g_objectFontSize,
								isVirtual ? Debug::g_globalColorVirtual.data() : Debug::g_objectColorParameter.data(),
								false,
								"%s: %2.2f\n",
								parameterPair.first,
								parameterPair.second);

							screenPos.y += Debug::g_objectLineHeight;
						}
					}

					if (drawEnvironments && (isTextFilterDisabled || doesEnvironmentMatchFilter))
					{
						for (auto const& environmentPair : environmentInfo)
						{
							auxGeom.Draw2dLabel(
								screenPos.x,
								screenPos.y,
								Debug::g_objectFontSize,
								isVirtual ? Debug::g_globalColorVirtual.data() : Debug::g_objectColorEnvironment.data(),
								false,
								"%s: %.2f\n",
								environmentPair.first,
								environmentPair.second);

							screenPos.y += Debug::g_objectLineHeight;
						}
					}

					if (drawDistance)
					{

						CryFixedStringT<MaxMiscStringLength> debugText;
						float const maxRadius = pObject->GetMaxRadius();

						if (maxRadius > 0.0f)
						{
							debugText.Format("Dist: %4.1fm / Max: %.1fm", distance, maxRadius);
						}
						else
						{
							debugText.Format("Dist: %4.1fm", distance);
						}

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y,
							Debug::g_objectFontSize,
							isVirtual ? Debug::g_globalColorVirtual.data() : Debug::g_objectColorActive.data(),
							false,
							debugText.c_str());

						screenPos.y += Debug::g_objectLineHeight;
					}

					if (drawOcclusionRayLabel)
					{
						EOcclusionType const occlusionType = m_propagationProcessor.GetOcclusionType();
						SATLSoundPropagationData propagationData;
						m_propagationProcessor.GetPropagationData(propagationData);

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

						float const activeRayLabelColor[4] = { propagationData.occlusion, 1.0f - propagationData.occlusion, 0.0f, 0.9f };

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y,
							Debug::g_objectFontSize,
							((occlusionType != EOcclusionType::None) && (occlusionType != EOcclusionType::Ignore)) ? (isVirtual ? Debug::g_globalColorVirtual.data() : activeRayLabelColor) : Debug::g_globalColorInactive.data(),
							false,
							"Occl: %3.2f | Type: %s", // Add obstruction once the engine supports it.
							propagationData.occlusion,
							debugText.c_str());

						screenPos.y += Debug::g_objectLineHeight;
					}

					if (drawOcclusionRayOffset && !isVirtual)
					{
						float const occlusionRayOffset = pObject->GetOcclusionRayOffset();

						if (occlusionRayOffset > 0.0f)
						{
							SAuxGeomRenderFlags const previousRenderFlags = auxGeom.GetRenderFlags();
							SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
							auxGeom.SetRenderFlags(newRenderFlags);

							auxGeom.DrawSphere(
								position,
								occlusionRayOffset,
								ColorB(1, 255, 1, 85));

							auxGeom.SetRenderFlags(previousRenderFlags);
						}
					}

					if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectImplInfo) != 0)
					{
						m_pImplData->DrawDebugInfo(auxGeom, screenPos.x, screenPos.y, (isTextFilterDisabled ? nullptr : lowerCaseSearchString.c_str()));
					}

					m_propagationProcessor.DrawDebugInfo(auxGeom);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ForceImplementationRefresh(bool const setTransformation)
{
	if (setTransformation)
	{
		m_pImplData->SetTransformation(m_transformation);
	}

	m_pImplData->ToggleFunctionality(Impl::EObjectFunctionality::TrackAbsoluteVelocity, (m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0);
	m_pImplData->ToggleFunctionality(Impl::EObjectFunctionality::TrackRelativeVelocity, (m_flags& EObjectFlags::TrackRelativeVelocity) != 0);

	// Parameters
	for (auto const& parameterPair : m_parameters)
	{
		auto const* const pParameter = stl::find_in_map(g_parameters, parameterPair.first, nullptr);

		if (pParameter != nullptr)
		{
			pParameter->Set(*this, parameterPair.second);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Parameter \"%u\" does not exist!", parameterPair.first);
		}
	}

	// Switches
	for (auto const& switchPair : m_switchStates)
	{
		CATLSwitch const* const pSwitch = stl::find_in_map(g_switches, switchPair.first, nullptr);

		if (pSwitch != nullptr)
		{
			CATLSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), switchPair.second, nullptr);

			if (pState != nullptr)
			{
				pState->Set(*this);
			}
		}
	}

	// Environments
	for (auto const& environmentPair : m_environments)
	{
		CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(g_environments, environmentPair.first, nullptr);

		if (pEnvironment != nullptr)
		{
			HandleSetEnvironment(pEnvironment, environmentPair.second);
		}
	}

	// Last re-execute its active triggers and standalone files.
	for (auto& triggerStatePair : m_triggerStates)
	{
		auto const* const pTrigger = stl::find_in_map(g_triggers, triggerStatePair.second.triggerId, nullptr);

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
			CRY_ASSERT_MESSAGE(pFile->m_state == EStandaloneFileState::Playing, "Standalone file must be in playing state during CATLAudioObject::ForceImplementationRefresh!");
			CRY_ASSERT_MESSAGE(pFile->m_pAudioObject == this, "Standalone file played on wrong object during CATLAudioObject::ForceImplementationRefresh!");

			auto const* const pTrigger = stl::find_in_map(g_triggers, pFile->m_triggerId, nullptr);

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
	SObjectRequestData<EObjectRequestType::ExecuteTrigger> requestData(triggerId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (triggerId != InvalidControlId)
	{
		SObjectRequestData<EObjectRequestType::StopTrigger> requestData(triggerId);
		PushRequest(requestData, userData);
	}
	else
	{
		SObjectRequestData<EObjectRequestType::StopAllTriggers> requestData;
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetTransformation(CObjectTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetTransformation> requestData(transformation);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetParameter> requestData(parameterId, value);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetSwitchState> requestData(audioSwitchId, audioSwitchStateId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetEnvironment(EnvironmentId const audioEnvironmentId, float const amount, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetEnvironment> requestData(audioEnvironmentId, amount);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetCurrentEnvironments(EntityId const entityToIgnore, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> requestData(entityToIgnore);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (occlusionType < EOcclusionType::Count)
	{
		SObjectRequestData<EObjectRequestType::SetOcclusionType> requestData(occlusionType);
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetOcclusionRayOffset(float const offset, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> requestData(offset);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::StopFile(char const* const szFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::StopFile> requestData(szFile);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::SetName(char const* const szName, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetName> requestData(szName);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ToggleAbsoluteVelocityTracking(bool const enable, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> requestData(enable);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CATLAudioObject::ToggleRelativeVelocityTracking(bool const enable, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> requestData(enable);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
bool CATLAudioObject::ExecuteDefaultTrigger(ControlId const id)
{
	bool wasSuccess = true;

	switch (id)
	{
	case LoseFocusTriggerId:
		g_loseFocusTrigger.Execute();
		break;
	case GetFocusTriggerId:
		g_getFocusTrigger.Execute();
		break;
	case MuteAllTriggerId:
		g_muteAllTrigger.Execute();
		break;
	case UnmuteAllTriggerId:
		g_unmuteAllTrigger.Execute();
		break;
	case PauseAllTriggerId:
		g_pauseAllTrigger.Execute();
		break;
	case ResumeAllTriggerId:
		g_resumeAllTrigger.Execute();
		break;
	default:
		wasSuccess = false;
		break;
	}

	return wasSuccess;
}
} // namespace CryAudio
