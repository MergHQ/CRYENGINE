// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "CVars.h"
#include "Managers.h"
#include "EventManager.h"
#include "System.h"
#include "FileManager.h"
#include "ListenerManager.h"
#include "Request.h"
#include "Event.h"
#include "StandaloneFile.h"
#include "Environment.h"
#include "Parameter.h"
#include "Switch.h"
#include "SwitchState.h"
#include "Trigger.h"
#include "LoseFocusTrigger.h"
#include "GetFocusTrigger.h"
#include "MuteAllTrigger.h"
#include "UnmuteAllTrigger.h"
#include "PauseAllTrigger.h"
#include "ResumeAllTrigger.h"
#include "ObjectRequestData.h"
#include "CallbackRequestData.h"
#include "Common/Logger.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include <CryString/HashedString.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryMath/Cry_Camera.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "PreviewTrigger.h"
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CObject::Release()
{
	// Do not clear the object's name though!
	m_activeEvents.clear();
	m_activeStandaloneFiles.clear();

	for (auto& triggerStatesPair : m_triggerStates)
	{
		triggerStatesPair.second.numLoadingEvents = 0;
		triggerStatesPair.second.numPlayingEvents = 0;
	}

	m_pImplData = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_flags &= ~EObjectFlags::Virtual;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::AddEvent(CEvent* const pEvent)
{
	m_activeEvents.insert(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Set the max activity radius of all active events on this object.
	m_maxRadius = std::max(pEvent->GetTriggerRadius(), m_maxRadius);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::AddTriggerState(TriggerInstanceId const id, STriggerInstanceState const& triggerInstanceState)
{
	m_triggerStates.emplace(id, triggerInstanceState);
}

//////////////////////////////////////////////////////////////////////////
void CObject::AddStandaloneFile(CStandaloneFile* const pStandaloneFile, SUserDataBase const& userDataBase)
{
	m_activeStandaloneFiles.emplace(pStandaloneFile, userDataBase);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SendFinishedTriggerInstanceRequest(STriggerInstanceState const& triggerInstanceState)
{
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const requestData(triggerInstanceState.triggerId);

	ERequestFlags flags = ERequestFlags::None;

	if ((triggerInstanceState.flags & ETriggerStatus::CallbackOnExternalThread) != 0)
	{
		flags = ERequestFlags::CallbackOnExternalOrCallingThread;
	}
	else if ((triggerInstanceState.flags & ETriggerStatus::CallbackOnAudioThread) != 0)
	{
		flags = ERequestFlags::CallbackOnAudioThread;
	}

	CRequest const request(&requestData, this, flags, triggerInstanceState.pOwnerOverride, triggerInstanceState.pUserData, triggerInstanceState.pUserDataOwner);
	g_system.PushRequest(request);
}

///////////////////////////////////////////////////////////////////////////
void CObject::ReportStartedEvent(CEvent* const pEvent)
{
	CRY_ASSERT_MESSAGE(m_activeEvents.find(pEvent) == m_activeEvents.end(), "An event was found in active list during %s", __FUNCTION__);

	AddEvent(pEvent);
	ObjectTriggerStates::iterator const iter(m_triggerStates.find(pEvent->m_triggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		STriggerInstanceState& triggerInstanceState = iter->second;

		CRY_ASSERT(triggerInstanceState.numLoadingEvents > 0);
		--(triggerInstanceState.numLoadingEvents);
		++(triggerInstanceState.numPlayingEvents);
	}
	else
	{
		// must exist!
		CRY_ASSERT(false);
	}
}

///////////////////////////////////////////////////////////////////////////
void CObject::ReportFinishedEvent(CEvent* const pEvent, bool const bSuccess)
{
	// If this assert fires we most likely have it ending before it was pushed into the active events list during trigger execution.
	CRY_ASSERT_MESSAGE(m_activeEvents.find(pEvent) != m_activeEvents.end(), "An event was not found in active list during %s", __FUNCTION__);

	m_activeEvents.erase(pEvent);

	ObjectTriggerStates::iterator const iter(m_triggerStates.find(pEvent->m_triggerInstanceId));

	if (iter != m_triggerStates.end())
	{
		switch (pEvent->m_state)
		{
		case EEventState::Playing:
		case EEventState::Virtual:
			{
				STriggerInstanceState& triggerInstanceState = iter->second;
				CRY_ASSERT_MESSAGE(triggerInstanceState.numPlayingEvents > 0, "Number of playing events must be at least 1 during %s", __FUNCTION__);

				if (--(triggerInstanceState.numPlayingEvents) == 0 && triggerInstanceState.numLoadingEvents == 0)
				{
					ReportFinishedTriggerInstance(iter);
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
void CObject::GetStartedStandaloneFileRequestData(CStandaloneFile* const pStandaloneFile, CRequest& request)
{
	ObjectStandaloneFileMap::const_iterator const iter(m_activeStandaloneFiles.find(pStandaloneFile));

	if (iter != m_activeStandaloneFiles.end())
	{
		SUserDataBase const& standaloneFileData = iter->second;
		request.pOwner = standaloneFileData.pOwnerOverride;
		request.pUserData = standaloneFileData.pUserData;
		request.pUserDataOwner = standaloneFileData.pUserDataOwner;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::ReportFinishedStandaloneFile(CStandaloneFile* const pStandaloneFile)
{
	m_activeStandaloneFiles.erase(pStandaloneFile);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::HandleStopTrigger(CTrigger const* const pTrigger)
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
void CObject::HandleSetEnvironment(CEnvironment const* const pEnvironment, float const value)
{
	for (auto const pEnvImpl : pEnvironment->m_connections)
	{
		m_pImplData->SetEnvironment(pEnvImpl, value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	StoreEnvironmentValue(pEnvironment->GetId(), value);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CObject::StopAllTriggers()
{
	m_pImplData->StopAllTriggers();
}

///////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusion(float const occlusion)
{
	m_pImplData->SetOcclusion(occlusion);
}

///////////////////////////////////////////////////////////////////////////
void CObject::ReportFinishedTriggerInstance(ObjectTriggerStates::iterator const& iter)
{
	STriggerInstanceState& triggerInstanceState = iter->second;
	SendFinishedTriggerInstanceRequest(triggerInstanceState);

	if ((triggerInstanceState.flags & ETriggerStatus::Loaded) != 0)
	{
		// If the trigger instance was manually loaded -- keep it
		triggerInstanceState.flags &= ~ETriggerStatus::Playing;
	}
	else
	{
		// If the trigger instance wasn't loaded -- kill it
		m_triggerStates.erase(iter);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::PushRequest(SRequestData const& requestData, SRequestUserData const& userData)
{
	CRequest const request(&requestData, userData, this);
	g_system.PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
bool CObject::IsActive() const
{
	for (auto const pEvent : m_activeEvents)
	{
		if (pEvent->IsPlaying())
		{
			return true;
		}
	}

	for (auto const& standaloneFilePair : m_activeStandaloneFiles)
	{
		CStandaloneFile const* const pStandaloneFile = standaloneFilePair.first;

		if (pStandaloneFile->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	m_propagationProcessor.Update();

	if (m_propagationProcessor.HasNewOcclusionValues())
	{
		m_pImplData->SetOcclusion(m_propagationProcessor.GetOcclusion());
	}

	m_pImplData->Update(deltaTime);
}

///////////////////////////////////////////////////////////////////////////
void CObject::ProcessPhysicsRay(CRayInfo* const pRayInfo)
{
	m_propagationProcessor.ProcessPhysicsRay(pRayInfo);
}

///////////////////////////////////////////////////////////////////////////
void CObject::HandleSetTransformation(CTransformation const& transformation)
{
	m_transformation = transformation;
	m_pImplData->SetTransformation(transformation);
}

///////////////////////////////////////////////////////////////////////////
void CObject::HandleSetOcclusionType(EOcclusionType const calcType)
{
	CRY_ASSERT(calcType != EOcclusionType::None);
	m_propagationProcessor.SetOcclusionType(calcType);
	m_pImplData->SetOcclusionType(calcType);
}

//////////////////////////////////////////////////////////////////////////
void CObject::HandleSetOcclusionRayOffset(float const offset)
{
	m_propagationProcessor.SetOcclusionRayOffset(offset);
}

//////////////////////////////////////////////////////////////////////////
void CObject::ReleasePendingRays()
{
	m_propagationProcessor.ReleasePendingRays();
}

//////////////////////////////////////////////////////////////////////////
void CObject::HandleStopFile(char const* const szFile)
{
	CHashedString const hashedFilename(szFile);
	auto iter = m_activeStandaloneFiles.cbegin();
	auto iterEnd = m_activeStandaloneFiles.cend();

	while (iter != iterEnd)
	{
		CStandaloneFile* const pFile = iter->first;

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
void CObject::Init(char const* const szName, Impl::IObject* const pImplData, EntityId const entityId)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_name = szName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_entityId = entityId;
	m_pImplData = pImplData;
	m_propagationProcessor.Init();
}

///////////////////////////////////////////////////////////////////////////
bool CObject::CanBeReleased() const
{
	return (m_flags& EObjectFlags::InUse) == 0 &&
	       m_activeEvents.empty() &&
	       m_activeStandaloneFiles.empty() &&
	       !m_propagationProcessor.HasPendingRays() &&
	       m_numPendingSyncCallbacks == 0;
}

///////////////////////////////////////////////////////////////////////////
void CObject::SetFlag(EObjectFlags const flag)
{
	m_flags |= flag;
}

///////////////////////////////////////////////////////////////////////////
void CObject::RemoveFlag(EObjectFlags const flag)
{
	m_flags &= ~flag;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
float const CObject::CStateDebugDrawData::s_minSwitchColor = 0.3f;
float const CObject::CStateDebugDrawData::s_maxSwitchColor = 1.0f;
int const CObject::CStateDebugDrawData::s_maxToMinUpdates = 100;
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
	#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	case PreviewTriggerId:
		szName = g_previewTrigger.GetName();
		break;
	#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	default:
		CRY_ASSERT_MESSAGE(false, R"(The default trigger "%u" does not exist during %s)", id, __FUNCTION__);
		break;
	}

	return szName;
}

///////////////////////////////////////////////////////////////////////////
CObject::CStateDebugDrawData::CStateDebugDrawData(SwitchStateId const switchStateId)
	: m_currentStateId(switchStateId)
	, m_currentSwitchColor(s_maxSwitchColor)
{
}

///////////////////////////////////////////////////////////////////////////
void CObject::CStateDebugDrawData::Update(SwitchStateId const switchStateId)
{
	if ((switchStateId == m_currentStateId) && (m_currentSwitchColor > s_minSwitchColor))
	{
		m_currentSwitchColor -= (s_maxSwitchColor - s_minSwitchColor) / s_maxToMinUpdates;
	}
	else if (switchStateId != m_currentStateId)
	{
		m_currentStateId = switchStateId;
		m_currentSwitchColor = s_maxSwitchColor;
	}
}

using TriggerCountMap = std::map<ControlId const, size_t>;

///////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom)
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
				bool const drawSphere = (g_cvars.m_drawDebug & Debug::EDrawFilter::Spheres) != 0;
				bool const drawLabel = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectLabel) != 0;
				bool const drawTriggers = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectTriggers) != 0;
				bool const drawStandaloneFiles = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectStandaloneFiles) != 0;
				bool const drawStates = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectStates) != 0;
				bool const drawParameters = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectParameters) != 0;
				bool const drawEnvironments = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectEnvironments) != 0;
				bool const drawDistance = (g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectDistance) != 0;
				bool const drawOcclusionRayLabel = (g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRayLabels) != 0;
				bool const drawOcclusionRayOffset = (g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRayOffset) != 0;
				bool const filterAllObjectInfo = (g_cvars.m_drawDebug & Debug::EDrawFilter::FilterAllObjectInfo) != 0;

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

						CRY_ASSERT_MESSAGE(szTriggerName != nullptr, "The trigger name mustn't be nullptr during %s", __FUNCTION__);

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
				std::map<CSwitch const* const, CSwitchState const* const> switchStateInfo;

				if ((drawStates && !m_switchStates.empty()) || filterAllObjectInfo)
				{
					for (auto const& switchStatePair : m_switchStates)
					{
						CSwitch const* const pSwitch = stl::find_in_map(g_switches, switchStatePair.first, nullptr);

						if (pSwitch != nullptr)
						{
							CSwitchState const* const pSwitchState = stl::find_in_map(pSwitch->GetStates(), switchStatePair.second, nullptr);

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

						CRY_ASSERT_MESSAGE(szParameterName != nullptr, "The parameter name mustn't be nullptr during %s", __FUNCTION__);

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
						CEnvironment const* const pEnvironment = stl::find_in_map(g_environments, environmentPair.first, nullptr);

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
				auto const pObject = const_cast<CObject*>(this);
				char const* const szObjectName = pObject->m_name.c_str();
				bool doesObjectNameMatchFilter = false;

				if (!isTextFilterDisabled && (drawLabel || filterAllObjectInfo))
				{
					CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
					lowerCaseObjectName.MakeLower();
					doesObjectNameMatchFilter = (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
				}

				bool const hasActiveData = pObject->IsActive();
				bool const isVirtual = (pObject->GetFlags() & EObjectFlags::Virtual) != 0;
				bool const canDraw = (g_cvars.m_hideInactiveObjects == 0) || ((g_cvars.m_hideInactiveObjects != 0) && hasActiveData && !isVirtual);

				if (canDraw)
				{
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
						float const occlusion = m_propagationProcessor.GetOcclusion();

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

						float const activeRayLabelColor[4] = { occlusion, 1.0f - occlusion, 0.0f, 0.9f };

						auxGeom.Draw2dLabel(
							screenPos.x,
							screenPos.y,
							Debug::g_objectFontSize,
							((occlusionType != EOcclusionType::None) && (occlusionType != EOcclusionType::Ignore)) ? (isVirtual ? Debug::g_globalColorVirtual.data() : activeRayLabelColor) : Debug::g_globalColorInactive.data(),
							false,
							"Occl: %3.2f | Type: %s", // Add obstruction once the engine supports it.
							occlusion,
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

					if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectImplInfo) != 0)
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
void CObject::ForceImplementationRefresh(bool const setTransformation)
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
		CSwitch const* const pSwitch = stl::find_in_map(g_switches, switchPair.first, nullptr);

		if (pSwitch != nullptr)
		{
			CSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), switchPair.second, nullptr);

			if (pState != nullptr)
			{
				pState->Set(*this);
			}
		}
	}

	// Environments
	for (auto const& environmentPair : m_environments)
	{
		CEnvironment const* const pEnvironment = stl::find_in_map(g_environments, environmentPair.first, nullptr);

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
		CStandaloneFile* const pFile = standaloneFilePair.first;
		CRY_ASSERT_MESSAGE(pFile != nullptr, "Standalone file pointer is nullptr during %s", __FUNCTION__);

		if (pFile != nullptr)
		{
			CRY_ASSERT_MESSAGE(pFile->m_state == EStandaloneFileState::Playing, "Standalone file must be in playing state during %s", __FUNCTION__);
			CRY_ASSERT_MESSAGE(pFile->m_pObject == this, "Standalone file played on wrong object during %s", __FUNCTION__);

			auto const* const pTrigger = stl::find_in_map(g_triggers, pFile->m_triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->PlayFile(*this, pFile);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::HandleSetName(char const* const szName)
{
	m_name = szName;
	return m_pImplData->SetName(szName);
}

//////////////////////////////////////////////////////////////////////////
void CObject::StoreParameterValue(ControlId const id, float const value)
{
	m_parameters[id] = value;
}

//////////////////////////////////////////////////////////////////////////
void CObject::StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId)
{
	m_switchStates[switchId] = switchStateId;
}

//////////////////////////////////////////////////////////////////////////
void CObject::StoreEnvironmentValue(ControlId const id, float const value)
{
	m_environments[id] = value;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CObject::ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ExecuteTrigger> requestData(triggerId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
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
void CObject::SetTransformation(CTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetTransformation> requestData(transformation);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetParameter> requestData(parameterId, value);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ControlId const switchId, SwitchStateId const switchStateId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetSwitchState> requestData(switchId, switchStateId);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(EnvironmentId const environmentId, float const amount, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetEnvironment> requestData(environmentId, amount);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetCurrentEnvironments(EntityId const entityToIgnore, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> requestData(entityToIgnore);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (occlusionType < EOcclusionType::Count)
	{
		SObjectRequestData<EObjectRequestType::SetOcclusionType> requestData(occlusionType);
		PushRequest(requestData, userData);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionRayOffset(float const offset, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> requestData(offset);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::LoadTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::LoadTrigger> const requestData(triggerId);
	CRequest const request(&requestData, userData);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::UnloadTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::UnloadTrigger> const requestData(triggerId);
	CRequest const request(&requestData, userData);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopFile(char const* const szFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::StopFile> requestData(szFile);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetName(char const* const szName, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetName> requestData(szName);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleAbsoluteVelocityTracking(bool const enable, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> requestData(enable);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleRelativeVelocityTracking(bool const enable, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> requestData(enable);
	PushRequest(requestData, userData);
}

//////////////////////////////////////////////////////////////////////////
bool CObject::ExecuteDefaultTrigger(ControlId const id)
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
