// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "System.h"
#include "Impl.h"
#include "PoolObject_impl.h"
#include "Managers.h"
#include "FileCacheManager.h"
#include "ListenerManager.h"
#include "EventListenerManager.h"
#include "XMLProcessor.h"
#include "FileManager.h"
#include "SystemRequestData.h"
#include "ObjectRequestData.h"
#include "ListenerRequestData.h"
#include "CallbackRequestData.h"
#include "CVars.h"
#include "FileEntry.h"
#include "Listener.h"
#include "Object.h"
#include "StandaloneFile.h"
#include "LoseFocusTrigger.h"
#include "GetFocusTrigger.h"
#include "MuteAllTrigger.h"
#include "UnmuteAllTrigger.h"
#include "PauseAllTrigger.h"
#include "ResumeAllTrigger.h"
#include "Environment.h"
#include "Parameter.h"
#include "PreloadRequest.h"
#include "Setting.h"
#include "Switch.h"
#include "SwitchState.h"
#include "Trigger.h"
#include "Common/IObject.h"
#include "PropagationProcessor.h"
#include <CrySystem/ITimer.h>
#include <CryString/CryPath.h>
#include <CryEntitySystem/IEntitySystem.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "PreviewTrigger.h"
	#include "Debug.h"
	#include "Common/Logger.h"
	#include "Common/DebugStyle.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
enum class ELoggingOptions : EnumFlagsType
{
	None,
	Errors   = BIT(6), // a
	Warnings = BIT(7), // b
	Comments = BIT(8), // c
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ELoggingOptions);

constexpr uint16 g_systemExecuteTriggerPoolSize = 4;
constexpr uint16 g_systemExecuteTriggerExPoolSize = 16;
constexpr uint16 g_systemStopTriggerPoolSize = 4;
constexpr uint16 g_systemRegisterObjectPoolSize = 16;
constexpr uint16 g_systemReleaseObjectPoolSize = 16;
constexpr uint16 g_systemSetParameterPoolSize = 4;
constexpr uint16 g_systemSetSwitchStatePoolSize = 4;

constexpr uint16 g_objectExecuteTriggerPoolSize = 64;
constexpr uint16 g_objectStopTriggerPoolSize = 128;
constexpr uint16 g_objectSetTransformationPoolSize = 128;
constexpr uint16 g_objectSetParameterPoolSize = 128;
constexpr uint16 g_objectSetSwitchStatePoolSize = 64;
constexpr uint16 g_objectSetCurrentEnvironmentsPoolSize = 16;
constexpr uint16 g_objectSetEnvironmentPoolSize = 64;
constexpr uint16 g_objectProcessPhysicsRayPoolSize = 128;

constexpr uint16 g_listenerSetTransformationPoolSize = 2;

constexpr uint16 g_callbackReportStartedTriggerConnectionInstancePoolSize = 64;
constexpr uint16 g_callbackReportFinishedTriggerConnectionInstancePoolSize = 128;
constexpr uint16 g_callbackReportFinishedTriggerInstancePoolSize = 128;
constexpr uint16 g_callbackReportPhysicalizedObjectPoolSize = 64;
constexpr uint16 g_callbackReportVirtualizedObjectPoolSize = 64;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
struct SRequestCount final
{
	uint16 requests = 0;

	uint16 systemExecuteTrigger = 0;
	uint16 systemExecuteTriggerEx = 0;
	uint16 systemStopTrigger = 0;
	uint16 systemRegisterObject = 0;
	uint16 systemReleaseObject = 0;
	uint16 systemSetParameter = 0;
	uint16 systemSetSwitchState = 0;

	uint16 objectExecuteTrigger = 0;
	uint16 objectStopTrigger = 0;
	uint16 objectSetTransformation = 0;
	uint16 objectSetParameter = 0;
	uint16 objectSetSwitchState = 0;
	uint16 objectSetCurrentEnvironments = 0;
	uint16 objectSetEnvironment = 0;
	uint16 objectProcessPhysicsRay = 0;

	uint16 listenerSetTransformation = 0;

	uint16 callbackReportStartedriggerConnectionInstance = 0;
	uint16 callbackReportFinishedTriggerConnectionInstance = 0;
	uint16 callbackReportFinishedTriggerInstance = 0;
	uint16 callbackReportPhysicalizedObject = 0;
	uint16 callbackReportVirtualizedObject = 0;
};

SRequestCount g_requestsPerUpdate;
SRequestCount g_requestPeaks;

//////////////////////////////////////////////////////////////////////////
void CountRequestPerUpdate(CRequest const& request)
{
	auto const pRequestData = request.GetData();

	if (pRequestData != nullptr)
	{
		g_requestsPerUpdate.requests++;

		switch (request.GetData()->requestType)
		{
		case ERequestType::SystemRequest:
			{
				auto const pBase = static_cast<SSystemRequestDataBase const*>(pRequestData);

				switch (pBase->systemRequestType)
				{
				case ESystemRequestType::RegisterObject:
					{
						g_requestsPerUpdate.systemRegisterObject++;

						break;
					}
				case ESystemRequestType::ReleaseObject:
					{
						g_requestsPerUpdate.systemReleaseObject++;

						break;
					}
				case ESystemRequestType::ExecuteTrigger:
					{
						g_requestsPerUpdate.systemExecuteTrigger++;

						break;
					}
				case ESystemRequestType::ExecuteTriggerEx:
					{
						g_requestsPerUpdate.systemExecuteTriggerEx++;

						break;
					}
				case ESystemRequestType::StopTrigger:
					{
						g_requestsPerUpdate.systemStopTrigger++;

						break;
					}
				case ESystemRequestType::SetParameter:
					{
						g_requestsPerUpdate.systemSetParameter++;

						break;
					}
				case ESystemRequestType::SetSwitchState:
					{
						g_requestsPerUpdate.systemSetSwitchState++;

						break;
					}
				default:
					{
						break;
					}
				}

				break;
			}
		case ERequestType::ObjectRequest:
			{
				auto const pBase = static_cast<SObjectRequestDataBase const*>(pRequestData);

				switch (pBase->objectRequestType)
				{
				case EObjectRequestType::ExecuteTrigger:
					{
						g_requestsPerUpdate.objectExecuteTrigger++;

						break;
					}
				case EObjectRequestType::StopTrigger:
					{
						g_requestsPerUpdate.objectStopTrigger++;

						break;
					}
				case EObjectRequestType::SetTransformation:
					{
						g_requestsPerUpdate.objectSetTransformation++;

						break;
					}
				case EObjectRequestType::SetParameter:
					{
						g_requestsPerUpdate.objectSetParameter++;

						break;
					}
				case EObjectRequestType::SetSwitchState:
					{
						g_requestsPerUpdate.objectSetSwitchState++;

						break;
					}
				case EObjectRequestType::SetCurrentEnvironments:
					{
						g_requestsPerUpdate.objectSetCurrentEnvironments++;

						break;
					}
				case EObjectRequestType::SetEnvironment:
					{
						g_requestsPerUpdate.objectSetEnvironment++;

						break;
					}
				case EObjectRequestType::ProcessPhysicsRay:
					{
						g_requestsPerUpdate.objectProcessPhysicsRay++;

						break;
					}
				default:
					{
						break;
					}
				}

				break;
			}
		case ERequestType::ListenerRequest:
			{
				auto const pBase = static_cast<SListenerRequestDataBase const*>(pRequestData);

				if (pBase->listenerRequestType == EListenerRequestType::SetTransformation)
				{
					g_requestsPerUpdate.listenerSetTransformation++;
				}

				break;
			}
		case ERequestType::CallbackRequest:
			{
				auto const pBase = static_cast<SCallbackRequestDataBase const*>(pRequestData);

				switch (pBase->callbackRequestType)
				{
				case ECallbackRequestType::ReportStartedTriggerConnectionInstance:
					{
						g_requestsPerUpdate.callbackReportStartedriggerConnectionInstance++;

						break;
					}
				case ECallbackRequestType::ReportFinishedTriggerConnectionInstance:
					{
						g_requestsPerUpdate.callbackReportFinishedTriggerConnectionInstance++;

						break;
					}
				case ECallbackRequestType::ReportFinishedTriggerInstance:
					{
						g_requestsPerUpdate.callbackReportFinishedTriggerInstance++;

						break;
					}
				case ECallbackRequestType::ReportPhysicalizedObject:
					{
						g_requestsPerUpdate.callbackReportPhysicalizedObject++;

						break;
					}
				case ECallbackRequestType::ReportVirtualizedObject:
					{
						g_requestsPerUpdate.callbackReportVirtualizedObject++;

						break;
					}
				default:
					{
						break;
					}
				}

				break;
			}
		default:
			{
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void SetRequestCountPeak()
{
	g_requestPeaks.requests = std::max(g_requestPeaks.requests, g_requestsPerUpdate.requests);

	g_requestPeaks.systemRegisterObject = std::max(g_requestPeaks.systemRegisterObject, g_requestsPerUpdate.systemRegisterObject);
	g_requestPeaks.systemReleaseObject = std::max(g_requestPeaks.systemReleaseObject, g_requestsPerUpdate.systemReleaseObject);
	g_requestPeaks.systemExecuteTrigger = std::max(g_requestPeaks.systemExecuteTrigger, g_requestsPerUpdate.systemExecuteTrigger);
	g_requestPeaks.systemExecuteTriggerEx = std::max(g_requestPeaks.systemExecuteTriggerEx, g_requestsPerUpdate.systemExecuteTriggerEx);
	g_requestPeaks.systemStopTrigger = std::max(g_requestPeaks.systemStopTrigger, g_requestsPerUpdate.systemStopTrigger);
	g_requestPeaks.systemSetParameter = std::max(g_requestPeaks.systemSetParameter, g_requestsPerUpdate.systemSetParameter);
	g_requestPeaks.systemSetSwitchState = std::max(g_requestPeaks.systemSetSwitchState, g_requestsPerUpdate.systemSetSwitchState);

	g_requestPeaks.objectExecuteTrigger = std::max(g_requestPeaks.objectExecuteTrigger, g_requestsPerUpdate.objectExecuteTrigger);
	g_requestPeaks.objectStopTrigger = std::max(g_requestPeaks.objectStopTrigger, g_requestsPerUpdate.objectStopTrigger);
	g_requestPeaks.objectSetTransformation = std::max(g_requestPeaks.objectSetTransformation, g_requestsPerUpdate.objectSetTransformation);
	g_requestPeaks.objectSetParameter = std::max(g_requestPeaks.objectSetParameter, g_requestsPerUpdate.objectSetParameter);
	g_requestPeaks.objectSetSwitchState = std::max(g_requestPeaks.objectSetSwitchState, g_requestsPerUpdate.objectSetSwitchState);
	g_requestPeaks.objectSetCurrentEnvironments = std::max(g_requestPeaks.objectSetCurrentEnvironments, g_requestsPerUpdate.objectSetCurrentEnvironments);
	g_requestPeaks.objectSetEnvironment = std::max(g_requestPeaks.objectSetEnvironment, g_requestsPerUpdate.objectSetEnvironment);
	g_requestPeaks.objectProcessPhysicsRay = std::max(g_requestPeaks.objectProcessPhysicsRay, g_requestsPerUpdate.objectProcessPhysicsRay);

	g_requestPeaks.listenerSetTransformation = std::max(g_requestPeaks.listenerSetTransformation, g_requestsPerUpdate.listenerSetTransformation);

	g_requestPeaks.callbackReportStartedriggerConnectionInstance = std::max(g_requestPeaks.callbackReportStartedriggerConnectionInstance, g_requestsPerUpdate.callbackReportStartedriggerConnectionInstance);
	g_requestPeaks.callbackReportFinishedTriggerConnectionInstance = std::max(g_requestPeaks.callbackReportFinishedTriggerConnectionInstance, g_requestsPerUpdate.callbackReportFinishedTriggerConnectionInstance);
	g_requestPeaks.callbackReportFinishedTriggerInstance = std::max(g_requestPeaks.callbackReportFinishedTriggerInstance, g_requestsPerUpdate.callbackReportFinishedTriggerInstance);
	g_requestPeaks.callbackReportPhysicalizedObject = std::max(g_requestPeaks.callbackReportPhysicalizedObject, g_requestsPerUpdate.callbackReportPhysicalizedObject);
	g_requestPeaks.callbackReportVirtualizedObject = std::max(g_requestPeaks.callbackReportVirtualizedObject, g_requestsPerUpdate.callbackReportVirtualizedObject);

	ZeroStruct(g_requestsPerUpdate);
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools()
{
	// Controls
	CTrigger::CreateAllocator(g_poolSizes.triggers);
	CParameter::CreateAllocator(g_poolSizes.parameters);
	CSwitch::CreateAllocator(g_poolSizes.switches);
	CSwitchState::CreateAllocator(g_poolSizes.states);
	CEnvironment::CreateAllocator(g_poolSizes.environments);
	CPreloadRequest::CreateAllocator(g_poolSizes.preloads);
	CSetting::CreateAllocator(g_poolSizes.settings);

	// Files
	CFileEntry::CreateAllocator(g_poolSizes.files);

	// System requests
	SSystemRequestData<ESystemRequestType::ExecuteTrigger>::CreateAllocator(g_systemExecuteTriggerPoolSize);
	SSystemRequestData<ESystemRequestType::ExecuteTriggerEx>::CreateAllocator(g_systemExecuteTriggerExPoolSize);
	SSystemRequestData<ESystemRequestType::StopTrigger>::CreateAllocator(g_systemStopTriggerPoolSize);
	SSystemRequestData<ESystemRequestType::RegisterObject>::CreateAllocator(g_systemRegisterObjectPoolSize);
	SSystemRequestData<ESystemRequestType::ReleaseObject>::CreateAllocator(g_systemReleaseObjectPoolSize);
	SSystemRequestData<ESystemRequestType::SetParameter>::CreateAllocator(g_systemSetParameterPoolSize);
	SSystemRequestData<ESystemRequestType::SetSwitchState>::CreateAllocator(g_systemSetSwitchStatePoolSize);

	// Object requests
	SObjectRequestData<EObjectRequestType::ExecuteTrigger>::CreateAllocator(g_objectExecuteTriggerPoolSize);
	SObjectRequestData<EObjectRequestType::StopTrigger>::CreateAllocator(g_objectStopTriggerPoolSize);
	SObjectRequestData<EObjectRequestType::SetTransformation>::CreateAllocator(g_objectSetTransformationPoolSize);
	SObjectRequestData<EObjectRequestType::SetParameter>::CreateAllocator(g_objectSetParameterPoolSize);
	SObjectRequestData<EObjectRequestType::SetSwitchState>::CreateAllocator(g_objectSetSwitchStatePoolSize);
	SObjectRequestData<EObjectRequestType::SetCurrentEnvironments>::CreateAllocator(g_objectSetCurrentEnvironmentsPoolSize);
	SObjectRequestData<EObjectRequestType::SetEnvironment>::CreateAllocator(g_objectSetEnvironmentPoolSize);
	SObjectRequestData<EObjectRequestType::ProcessPhysicsRay>::CreateAllocator(g_objectProcessPhysicsRayPoolSize);

	// Listener requests
	SListenerRequestData<EListenerRequestType::SetTransformation>::CreateAllocator(g_listenerSetTransformationPoolSize);

	// Callback requests
	SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance>::CreateAllocator(g_callbackReportStartedTriggerConnectionInstancePoolSize);
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance>::CreateAllocator(g_callbackReportFinishedTriggerConnectionInstancePoolSize);
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance>::CreateAllocator(g_callbackReportFinishedTriggerInstancePoolSize);
	SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject>::CreateAllocator(g_callbackReportPhysicalizedObjectPoolSize);
	SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject>::CreateAllocator(g_callbackReportVirtualizedObjectPoolSize);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	// Controls
	CTrigger::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CSwitch::FreeMemoryPool();
	CSwitchState::FreeMemoryPool();
	CEnvironment::FreeMemoryPool();
	CPreloadRequest::FreeMemoryPool();
	CSetting::FreeMemoryPool();

	// Files
	CFileEntry::FreeMemoryPool();

	// System requests
	SSystemRequestData<ESystemRequestType::ExecuteTrigger>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::ExecuteTriggerEx>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::StopTrigger>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::RegisterObject>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::ReleaseObject>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::SetParameter>::FreeMemoryPool();
	SSystemRequestData<ESystemRequestType::SetSwitchState>::FreeMemoryPool();

	// Object requests
	SObjectRequestData<EObjectRequestType::ExecuteTrigger>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::StopTrigger>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::SetTransformation>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::SetParameter>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::SetSwitchState>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::SetCurrentEnvironments>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::SetEnvironment>::FreeMemoryPool();
	SObjectRequestData<EObjectRequestType::ProcessPhysicsRay>::FreeMemoryPool();

	// Listener requests
	SListenerRequestData<EListenerRequestType::SetTransformation>::FreeMemoryPool();

	// Callback requests
	SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance>::FreeMemoryPool();
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance>::FreeMemoryPool();
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance>::FreeMemoryPool();
	SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject>::FreeMemoryPool();
	SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject>::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void UpdateActiveObjects(float const deltaTime)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CPropagationProcessor::s_totalAsyncPhysRays = 0;
	CPropagationProcessor::s_totalSyncPhysRays = 0;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (deltaTime > 0.0f)
	{
		auto iter = g_activeObjects.begin();
		auto iterEnd = g_activeObjects.end();

		while (iter != iterEnd)
		{
			CObject* const pObject = *iter;

			if (pObject->IsPlaying())
			{
				pObject->Update(deltaTime);
			}
			else if (!pObject->HasPendingCallbacks())
			{
				pObject->RemoveFlag(EObjectFlags::Active);

				if ((pObject->GetFlags() & EObjectFlags::InUse) == 0)
				{
					pObject->Destruct();
				}

				if (iter != (iterEnd - 1))
				{
					(*iter) = g_activeObjects.back();
				}

				g_activeObjects.pop_back();
				iter = g_activeObjects.begin();
				iterEnd = g_activeObjects.end();
				continue;
			}

			++iter;
		}
	}
	else
	{
		for (auto const pObject : g_activeObjects)
		{
			pObject->GetImplDataPtr()->Update(deltaTime);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
	CRY_ASSERT_MESSAGE(g_pIImpl == nullptr, "<Audio> The implementation must get destroyed before the audio system is destructed during %s", __FUNCTION__);
	CRY_ASSERT_MESSAGE(g_activeObjects.empty(), "There are still active objects during %s", __FUNCTION__);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CRY_ASSERT_MESSAGE(g_constructedObjects.empty(), "There are still objects during %s", __FUNCTION__);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::AddRequestListener(void (*func)(SRequestInfo const* const), void* const pObjectToListenTo, ESystemEvents const eventMask)
{
	SSystemRequestData<ESystemRequestType::AddRequestListener> const requestData(pObjectToListenTo, func, eventMask);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking, pObjectToListenTo); // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RemoveRequestListener(void (*func)(SRequestInfo const* const), void* const pObjectToListenTo)
{
	SSystemRequestData<ESystemRequestType::RemoveRequestListener> const requestData(pObjectToListenTo, func);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking, pObjectToListenTo); // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExternalUpdate()
{
	CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: External Update");

	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	CRequest request;

	while (m_syncCallbacks.dequeue(request))
	{
		NotifyListener(request);

		if (request.GetData()->requestType == ERequestType::ObjectRequest)
		{
			auto const pBase = static_cast<SObjectRequestDataBase const*>(request.GetData());
			pBase->pObject->DecrementSyncCallbackCounter();
			// No sync callback counting for global object, because it gets released on unloading of the audio system dll.
		}
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	DrawDebug();
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	m_accumulatedFrameTime += gEnv->pTimer->GetFrameTime();
	++m_externalThreadFrameId;

	// If sleeping, wake up the audio thread to start processing requests again.
	m_audioThreadWakeupEvent.Set();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PushRequest(CRequest const& request)
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);

	if ((g_systemStates& ESystemStates::ImplShuttingDown) == 0)
	{
		m_requestQueue.enqueue(request);

		if ((request.flags & ERequestFlags::ExecuteBlocking) != 0)
		{
			// If sleeping, wake up the audio thread to start processing requests again.
			m_audioThreadWakeupEvent.Set();

			m_mainEvent.Wait();
			m_mainEvent.Reset();

			if ((request.flags & ERequestFlags::CallbackOnExternalOrCallingThread) != 0)
			{
				NotifyListener(m_syncRequest);
			}
		}
	}
	else
	{
		Log(ELogType::Warning, "Discarded PushRequest due to Audio System not allowing for new ones!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			// This event is issued in Editor and Game mode.
			string const levelNameOnly = PathUtil::GetFileName(reinterpret_cast<const char*>(wparam));

			if (!levelNameOnly.empty() && levelNameOnly.compareNoCase("Untitled") != 0)
			{
				CryFixedStringT<MaxFilePathLength> levelPath(g_configPath);
				levelPath += g_szLevelsFolderName;
				levelPath += "/";
				levelPath += levelNameOnly;

				SSystemRequestData<ESystemRequestType::ParseControlsData> const requestData1(levelPath, EDataScope::LevelSpecific);
				CRequest const request1(&requestData1);
				PushRequest(request1);

				SSystemRequestData<ESystemRequestType::ParsePreloadsData> const requestData2(levelPath, EDataScope::LevelSpecific);
				CRequest const request2(&requestData2);
				PushRequest(request2);

				PreloadRequestId const preloadRequestId = StringToId(levelNameOnly.c_str());
				SSystemRequestData<ESystemRequestType::PreloadSingleRequest> const requestData3(preloadRequestId, true);
				CRequest const request3(&requestData3);
				PushRequest(request3);

				SSystemRequestData<ESystemRequestType::AutoLoadSetting> const requestData4(EDataScope::LevelSpecific);
				CRequest const request4(&requestData4);
				PushRequest(request4);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				ResetRequestCount();
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}

			break;
		}
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			// This event is issued in Editor and Game mode.
			CPropagationProcessor::s_bCanIssueRWIs = false;

			SSystemRequestData<ESystemRequestType::ReleasePendingRays> const requestData1;
			CRequest const request1(&requestData1);
			PushRequest(request1);

			SSystemRequestData<ESystemRequestType::UnloadAFCMDataByScope> const requestData2(EDataScope::LevelSpecific);
			CRequest const request2(&requestData2);
			PushRequest(request2);

			SSystemRequestData<ESystemRequestType::ClearControlsData> const requestData3(EDataScope::LevelSpecific);
			CRequest const request3(&requestData3);
			PushRequest(request3);

			SSystemRequestData<ESystemRequestType::ClearPreloadsData> const requestData4(EDataScope::LevelSpecific);
			CRequest const request4(&requestData4);
			PushRequest(request4);

			break;
		}
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			// This event is issued in Editor and Game mode.
			CPropagationProcessor::s_bCanIssueRWIs = true;

			break;
		}
	case ESYSTEM_EVENT_CRYSYSTEM_INIT_DONE:
		{
			if (gEnv->pInput != nullptr)
			{
				gEnv->pInput->AddConsoleEventListener(this);
			}

			break;
		}
	case ESYSTEM_EVENT_FULL_SHUTDOWN: // Intentional fall-through.
	case ESYSTEM_EVENT_FAST_SHUTDOWN:
		{
			if (gEnv->pInput != nullptr)
			{
				gEnv->pInput->RemoveConsoleEventListener(this);
			}

			break;
		}
	case ESYSTEM_EVENT_ACTIVATE:
		{
			// When Alt+Tabbing out of the application while it's in full-screen mode
			// ESYSTEM_EVENT_ACTIVATE is sent instead of ESYSTEM_EVENT_CHANGE_FOCUS.
			if (g_cvars.m_ignoreWindowFocus == 0)
			{
				// wparam != 0 is active, wparam == 0 is inactive
				// lparam != 0 is minimized, lparam == 0 is not minimized
				if (wparam == 0 || lparam != 0)
				{
					// lost focus
					ExecuteDefaultTrigger(EDefaultTriggerType::LoseFocus);
				}
				else
				{
					// got focus
					ExecuteDefaultTrigger(EDefaultTriggerType::GetFocus);
				}
			}

			break;
		}
	case ESYSTEM_EVENT_CHANGE_FOCUS:
		{
			if (g_cvars.m_ignoreWindowFocus == 0)
			{
				// wparam != 0 is focused, wparam == 0 is not focused
				if (wparam == 0)
				{
					// lost focus
					ExecuteDefaultTrigger(EDefaultTriggerType::LoseFocus);
				}
				else
				{
					// got focus
					ExecuteDefaultTrigger(EDefaultTriggerType::GetFocus);
				}
			}

			break;
		}
	case ESYSTEM_EVENT_AUDIO_MUTE:
		{
			ExecuteDefaultTrigger(EDefaultTriggerType::MuteAll);

			break;
		}
	case ESYSTEM_EVENT_AUDIO_UNMUTE:
		{
			ExecuteDefaultTrigger(EDefaultTriggerType::UnmuteAll);

			break;
		}
	case ESYSTEM_EVENT_AUDIO_PAUSE:
		{
			ExecuteDefaultTrigger(EDefaultTriggerType::PauseAll);

			break;
		}
	case ESYSTEM_EVENT_AUDIO_RESUME:
		{
			ExecuteDefaultTrigger(EDefaultTriggerType::ResumeAll);

			break;
		}
	case ESYSTEM_EVENT_AUDIO_LANGUAGE_CHANGED:
		{
			OnLanguageChanged();

			break;
		}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	case ESYSTEM_EVENT_AUDIO_REFRESH:
		{
			auto const szLevelName = reinterpret_cast<const char*>(wparam);
			SSystemRequestData<ESystemRequestType::RefreshSystem> const requestData(szLevelName);
			CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
			PushRequest(request);

			break;
		}
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::OnInputEvent(SInputEvent const& event)
{
	if (event.state == eIS_Changed && event.deviceType == eIDT_Gamepad)
	{
		if (event.keyId == eKI_SYS_ConnectDevice)
		{
			g_pIImpl->GamepadConnected(event.deviceUniqueID);
		}
		else if (event.keyId == eKI_SYS_DisconnectDevice)
		{
			g_pIImpl->GamepadDisconnected(event.deviceUniqueID);
		}
	}

	// Do not consume event
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::InternalUpdate()
{
	CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: Internal Update");

	if (m_lastExternalThreadFrameId != m_externalThreadFrameId)
	{
		if (g_pIImpl != nullptr)
		{
			g_listenerManager.Update(m_accumulatedFrameTime);
			UpdateActiveObjects(m_accumulatedFrameTime);
			g_pIImpl->Update();
		}

		ProcessRequests(m_requestQueue);
		m_lastExternalThreadFrameId = m_externalThreadFrameId;
		m_accumulatedFrameTime = 0.0f;
		m_didThreadWait = false;
	}
	else if (m_didThreadWait)
	{
		// Effectively no time has passed for the external thread as it didn't progress.
		// Consequently 0.0f is passed for deltaTime.
		if (g_pIImpl != nullptr)
		{
			g_listenerManager.Update(0.0f);
			UpdateActiveObjects(0.0f);
			g_pIImpl->Update();
		}

		ProcessRequests(m_requestQueue);
		m_didThreadWait = false;
	}
	else
	{
		// If we're faster than the external thread let's wait to make room for other threads.
		CRY_PROFILE_REGION_WAITING(PROFILE_AUDIO, "Wait - Audio Update");

		// The external thread will wake the audio thread up effectively syncing it to itself.
		// If not however, the audio thread will execute at a minimum of roughly 30 fps.
		if (m_audioThreadWakeupEvent.Wait(30))
		{
			// Only reset if the event was signaled, not timed-out!
			m_audioThreadWakeupEvent.Reset();
		}

		m_didThreadWait = true;
	}
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::Initialize()
{
	if (!m_isInitialized)
	{
		g_cvars.RegisterVariables();

		if (g_cvars.m_objectPoolSize < 1)
		{
			g_cvars.m_objectPoolSize = 1;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(Audio Object pool size must be at least 1. Forcing the cvar "s_AudioObjectPoolSize" to 1!)");
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}

		CObject::CreateAllocator(static_cast<uint16>(g_cvars.m_objectPoolSize));

		if (g_cvars.m_standaloneFilePoolSize < 1)
		{
			g_cvars.m_standaloneFilePoolSize = 1;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(Audio Standalone File pool size must be at least 1. Forcing the cvar "s_AudioStandaloneFilePoolSize" to 1!)");
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}

		CStandaloneFile::CreateAllocator(static_cast<uint16>(g_cvars.m_standaloneFilePoolSize));

		// Add the callback for the obstruction calculation.
		gEnv->pPhysicalWorld->AddEventClient(
			EventPhysRWIResult::id,
			&CPropagationProcessor::OnObstructionTest,
			1);

		m_objectPoolSize = static_cast<uint16>(g_cvars.m_objectPoolSize);

		g_activeObjects.reserve(static_cast<size_t>(m_objectPoolSize));

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		g_constructedObjects.reserve(static_cast<size_t>(m_objectPoolSize));
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		g_fileCacheManager.Initialize();

		CRY_ASSERT_MESSAGE(!m_mainThread.IsActive(), "AudioSystem thread active before initialization during %s", __FUNCTION__);
		m_mainThread.Activate();
		AddRequestListener(&CSystem::OnCallback, nullptr, ESystemEvents::TriggerExecuted | ESystemEvents::TriggerFinished);
		m_isInitialized = true;
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "AudioSystem has already been initialized during %s", __FUNCTION__);
	}

	return m_isInitialized;
}

///////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
	if (m_isInitialized)
	{
		RemoveRequestListener(&CSystem::OnCallback, nullptr);

		SSystemRequestData<ESystemRequestType::ReleaseImpl> const requestData;
		CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
		PushRequest(request);

		m_mainThread.Deactivate();

		if (gEnv->pPhysicalWorld != nullptr)
		{
			// remove the callback for the obstruction calculation
			gEnv->pPhysicalWorld->RemoveEventClient(
				EventPhysRWIResult::id,
				&CPropagationProcessor::OnObstructionTest,
				1);
		}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		for (auto const pObject : g_constructedObjects)
		{
			CRY_ASSERT_MESSAGE(pObject->GetImplDataPtr() == nullptr, "An object cannot have valid impl data during %s", __FUNCTION__);
			delete pObject;
		}

		g_constructedObjects.clear();
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		g_listenerManager.Terminate();
		g_cvars.UnregisterVariables();

		CObject::FreeMemoryPool();
		CStandaloneFile::FreeMemoryPool();

		m_isInitialized = false;
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "AudioSystem has already been released or was never properly initialized during %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetImpl(Impl::IImpl* const pIImpl, SRequestUserData const& userData /*= SAudioRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::SetImpl> const requestData(pIImpl);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteTriggerEx(SExecuteTriggerData const& triggerData, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::ExecuteTriggerEx> const requestData(triggerData);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteDefaultTrigger(EDefaultTriggerType const type, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(type);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::SetParameter> const requestData(parameterId, value);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetGlobalParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::SetGlobalParameter> const requestData(parameterId, value);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetSwitchState(ControlId const switchId, SwitchStateId const switchStateId, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::SetSwitchState> const requestData(switchId, switchStateId);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetGlobalSwitchState(ControlId const switchId, SwitchStateId const switchStateId, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::SetGlobalSwitchState> const requestData(switchId, switchStateId);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::ExecuteTrigger> const requestData(triggerId);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (triggerId != InvalidControlId)
	{
		SSystemRequestData<ESystemRequestType::StopTrigger> const requestData(triggerId);
		CRequest const request(&requestData, userData);
		PushRequest(request);
	}
	else
	{
		SSystemRequestData<ESystemRequestType::StopAllTriggers> const requestData;
		CRequest const request(&requestData, userData);
		PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecutePreviewTrigger(ControlId const triggerId)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	switch (triggerId)
	{
	case g_loseFocusTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::LoseFocus);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	case g_getFocusTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::GetFocus);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	case g_muteAllTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::MuteAll);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	case g_unmuteAllTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::UnmuteAll);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	case g_pauseAllTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::PauseAll);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	case g_resumeAllTriggerId:
		{
			SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const requestData(EDefaultTriggerType::ResumeAll);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	default:
		{
			SSystemRequestData<ESystemRequestType::ExecutePreviewTrigger> const requestData(triggerId);
			CRequest const request(&requestData);
			PushRequest(request);

			break;
		}
	}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecutePreviewTriggerEx(Impl::ITriggerInfo const& triggerInfo)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	SSystemRequestData<ESystemRequestType::ExecutePreviewTriggerEx> const requestData(triggerInfo);
	CRequest const request(&requestData);
	PushRequest(request);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopPreviewTrigger()
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	SSystemRequestData<ESystemRequestType::StopPreviewTrigger> const requestData;
	CRequest const request(&requestData);
	PushRequest(request);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CSystem::ResetRequestCount()
{
	SSystemRequestData<ESystemRequestType::ResetRequestCount> const requestData;
	CRequest const request(&requestData);
	PushRequest(request);
}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CSystem::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::PlayFile> const requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	CRequest const request(
		&requestData,
		userData.flags,
		((userData.pOwner != nullptr) ? userData.pOwner : &g_system),
		userData.pUserData,
		userData.pUserDataOwner);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopFile(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::StopFile> const requestData(szName);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportStartedFile(
	CStandaloneFile& standaloneFile,
	bool const bSuccessfullyStarted,
	SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackRequestData<ECallbackRequestType::ReportStartedFile> const requestData(standaloneFile, bSuccessfullyStarted);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportStoppedFile(CStandaloneFile& standaloneFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SCallbackRequestData<ECallbackRequestType::ReportStoppedFile> const requestData(standaloneFile);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportStartedTriggerConnectionInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> const requestData(triggerInstanceId, result);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportFinishedTriggerConnectionInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance> const requestData(triggerInstanceId, result);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportPhysicalizedObject(Impl::IObject* const pIObject, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> const requestData(pIObject);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportVirtualizedObject(Impl::IObject* const pIObject, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject> const requestData(pIObject);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopAllSounds(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::StopAllSounds> const requestData;
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ParseControlsData(
	char const* const szFolderPath,
	EDataScope const dataScope,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::ParseControlsData> const requestData(szFolderPath, dataScope);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ParsePreloadsData(
	char const* const szFolderPath,
	EDataScope const dataScope,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::ParsePreloadsData> const requestData(szFolderPath, dataScope);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PreloadSingleRequest(PreloadRequestId const id, bool const bAutoLoadOnly, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::PreloadSingleRequest> const requestData(id, bAutoLoadOnly);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadSingleRequest(PreloadRequestId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::UnloadSingleRequest> const requestData(id);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::AutoLoadSetting(EDataScope const dataScope, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::AutoLoadSetting> const requestData(dataScope);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadSetting(ControlId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::LoadSetting> const requestData(id);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadSetting(ControlId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SSystemRequestData<ESystemRequestType::UnloadSetting> const requestData(id);
	CRequest const request(&requestData, userData);
	PushRequest(request);
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CSystem::RetriggerControls(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SSystemRequestData<ESystemRequestType::RetriggerControls> const requestData;
	CRequest const request(&requestData, userData);
	PushRequest(request);
}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CSystem::ReloadControlsData(
	char const* const szFolderPath,
	char const* const szLevelName,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	SSystemRequestData<ESystemRequestType::ReloadControlsData> const requestData(szFolderPath, szLevelName);
	CRequest const request(&requestData, userData);
	PushRequest(request);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
char const* CSystem::GetConfigPath() const
{
	return g_configPath.c_str();
}

///////////////////////////////////////////////////////////////////////////
CryAudio::IListener* CSystem::CreateListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	CListener* pListener = nullptr;
	SSystemRequestData<ESystemRequestType::RegisterListener> const requestData(&pListener, transformation, szName);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
	PushRequest(request);

	return static_cast<CryAudio::IListener*>(pListener);
}

///////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseListener(CryAudio::IListener* const pIListener)
{
	SSystemRequestData<ESystemRequestType::ReleaseListener> const requestData(static_cast<CListener*>(pIListener));
	CRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
CryAudio::IObject* CSystem::CreateObject(SCreateObjectData const& objectData /*= SCreateObjectData::GetEmptyObject()*/)
{
	CObject* pObject = nullptr;
	SSystemRequestData<ESystemRequestType::RegisterObject> const requestData(&pObject, objectData);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
	PushRequest(request);

	return static_cast<CryAudio::IObject*>(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseObject(CryAudio::IObject* const pIObject)
{
	SSystemRequestData<ESystemRequestType::ReleaseObject> const requestData(static_cast<CObject*>(pIObject));
	CRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetFileData(char const* const szName, SFileData& fileData)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	SSystemRequestData<ESystemRequestType::GetFileData> const requestData(szName, fileData);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
	PushRequest(request);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetTriggerData(ControlId const triggerId, STriggerData& triggerData)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CTrigger const* const pTrigger = stl::find_in_map(g_triggers, triggerId, nullptr);

	if (pTrigger != nullptr)
	{
		triggerData.radius = pTrigger->GetRadius();
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseImpl()
{
	// Reject new requests during shutdown.
	g_systemStates |= ESystemStates::ImplShuttingDown;

	g_pIImpl->OnBeforeRelease();

	// Release middleware specific data before its shutdown.
	g_fileManager.ReleaseImplData();
	g_listenerManager.ReleaseImplData();

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	// Don't delete objects here as we need them to survive a middleware switch!
	for (auto const pObject : g_constructedObjects)
	{
		g_pIImpl->DestructObject(pObject->GetImplDataPtr());
		pObject->Release();
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	g_pIImpl->DestructObject(g_object.GetImplDataPtr());
	g_object.SetImplDataPtr(nullptr);
	g_object.Release();

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	g_pIImpl->DestructObject(g_previewObject.GetImplDataPtr());
	g_previewObject.SetImplDataPtr(nullptr);
	g_previewObject.Release();
	ResetRequestCount();
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	g_xmlProcessor.ClearPreloadsData(EDataScope::All);
	g_xmlProcessor.ClearControlsData(EDataScope::All);

	g_pIImpl->ShutDown();
	g_pIImpl->Release();
	g_pIImpl = nullptr;

	// Release engine specific data after impl shut down to prevent dangling data accesses during shutdown.
	// Note: The object and listener managers are an exception as we need their data to survive in case the middleware is swapped out.
	g_fileManager.Release();

	FreeMemoryPools();

	g_systemStates &= ~ESystemStates::ImplShuttingDown;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageChanged()
{
	SSystemRequestData<ESystemRequestType::ChangeLanguage> const requestData;
	CRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Log(ELogType const type, char const* const szFormat, ...)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if (szFormat != nullptr && szFormat[0] != '\0' && gEnv->pLog->GetVerbosityLevel() != -1)
	{
		CRY_PROFILE_REGION(PROFILE_AUDIO, "CSystem::Log");

		char buffer[256];
		va_list ArgList;
		va_start(ArgList, szFormat);
		cry_vsprintf(buffer, szFormat, ArgList);
		va_end(ArgList);

		auto const loggingOptions = static_cast<ELoggingOptions>(g_cvars.m_loggingOptions);

		switch (type)
		{
		case ELogType::Warning:
			{
				if ((loggingOptions& ELoggingOptions::Warnings) != 0)
				{
					gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, nullptr, "<Audio>: %s", buffer);
				}

				break;
			}
		case ELogType::Error:
			{
				if ((loggingOptions& ELoggingOptions::Errors) != 0)
				{
					gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, nullptr, "<Audio>: %s", buffer);
				}

				break;
			}
		case ELogType::Comment:
			{
				if ((gEnv->pLog != nullptr) && (gEnv->pLog->GetVerbosityLevel() >= 4) && ((loggingOptions& ELoggingOptions::Comments) != 0))
				{
					CryLogAlways("<Audio>: %s", buffer);
				}

				break;
			}
		case ELogType::Always:
			{
				CryLogAlways("<Audio>: %s", buffer);

				break;
			}
		default:
			{
				CRY_ASSERT(false);

				break;
			}
		}
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ProcessRequests(Requests& requestQueue)
{
	CRequest request;

	while (requestQueue.dequeue(request))
	{
		if (request.status == ERequestStatus::None)
		{
			request.status = ERequestStatus::Pending;
			ProcessRequest(request);
		}
		else
		{
			// TODO: handle pending requests!
		}

		if (request.status != ERequestStatus::Pending)
		{
			if ((request.flags & ERequestFlags::CallbackOnAudioThread) != 0)
			{
				NotifyListener(request);

				if ((request.flags & ERequestFlags::ExecuteBlocking) != 0)
				{
					m_mainEvent.Set();
				}
			}
			else if ((request.flags & ERequestFlags::CallbackOnExternalOrCallingThread) != 0)
			{
				if ((request.flags & ERequestFlags::ExecuteBlocking) != 0)
				{
					m_syncRequest = request;
					m_mainEvent.Set();
				}
				else
				{
					if (request.GetData()->requestType == ERequestType::ObjectRequest)
					{
						auto const pBase = static_cast<SObjectRequestDataBase const*>(request.GetData());
						pBase->pObject->IncrementSyncCallbackCounter();
						// No sync callback counting for global object, because it gets released on unloading of the audio system dll.
					}

					m_syncCallbacks.enqueue(request);
				}
			}
			else if ((request.flags & ERequestFlags::ExecuteBlocking) != 0)
			{
				m_mainEvent.Set();
			}
		}
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	SetRequestCountPeak();
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ProcessRequest(CRequest& request)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CountRequestPerUpdate(request);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	ERequestStatus result = ERequestStatus::None;

	if (request.GetData())
	{
		switch (request.GetData()->requestType)
		{
		case ERequestType::ObjectRequest:
			{
				result = ProcessObjectRequest(request);

				break;
			}
		case ERequestType::ListenerRequest:
			{
				result = ProcessListenerRequest(request.GetData());

				break;
			}
		case ERequestType::CallbackRequest:
			{
				result = ProcessCallbackRequest(request);

				break;
			}
		case ERequestType::SystemRequest:
			{
				result = ProcessSystemRequest(request);

				break;
			}
		default:
			{
				CRY_ASSERT_MESSAGE(false, "Unknown request type: %u during %s", request.GetData()->requestType, __FUNCTION__);

				break;
			}
		}
	}

	request.status = result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessSystemRequest(CRequest const& request)
{
	ERequestStatus result = ERequestStatus::Failure;
	auto const pBase = static_cast<SSystemRequestDataBase const*>(request.GetData());

	switch (pBase->systemRequestType)
	{
	case ESystemRequestType::AddRequestListener:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::AddRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.AddRequestListener(pRequestData);

			break;
		}
	case ESystemRequestType::RemoveRequestListener:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::RemoveRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.RemoveRequestListener(pRequestData->func, pRequestData->pObjectToListenTo);

			break;
		}
	case ESystemRequestType::SetImpl:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::SetImpl> const*>(request.GetData());
			result = HandleSetImpl(pRequestData->pIImpl);

			break;
		}
	case ESystemRequestType::ExecuteTrigger:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecuteTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->Execute(g_object, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::StopTrigger:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::StopTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = g_object.HandleStopTrigger(pTrigger);
			}

			break;
		}
	case ESystemRequestType::StopAllTriggers:
		{
			g_object.StopAllTriggers();
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ExecuteTriggerEx:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecuteTriggerEx> const*>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CObject");

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				auto const pNewObject = new CObject(pRequestData->transformation, pRequestData->name.c_str());
				pNewObject->Init(g_pIImpl->ConstructObject(pRequestData->transformation, pNewObject->GetName()), pRequestData->entityId);
				g_constructedObjects.push_back(pNewObject);
#else
				auto const pNewObject = new CObject(pRequestData->transformation);
				pNewObject->Init(g_pIImpl->ConstructObject(pRequestData->transformation), pRequestData->entityId);
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE

				if (pRequestData->setCurrentEnvironments)
				{
					SetCurrentEnvironmentsOnObject(pNewObject, INVALID_ENTITYID);
				}

#if defined(CRY_AUDIO_USE_OCCLUSION)
				SetOcclusionType(*pNewObject, pRequestData->occlusionType);
#endif    // CRY_AUDIO_USE_OCCLUSION
				pTrigger->Execute(*pNewObject, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				pNewObject->RemoveFlag(EObjectFlags::InUse);

				if ((pNewObject->GetFlags() & EObjectFlags::Active) == 0)
				{
					pNewObject->Destruct();
				}

				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::ExecuteDefaultTrigger:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const*>(request.GetData());

			switch (pRequestData->triggerType)
			{
			case EDefaultTriggerType::LoseFocus:
				{
					if ((g_systemStates& ESystemStates::IsMuted) == 0)
					{
						g_loseFocusTrigger.Execute();
					}

					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::GetFocus:
				{
					if ((g_systemStates& ESystemStates::IsMuted) == 0)
					{
						g_getFocusTrigger.Execute();
					}

					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::MuteAll:
				{
					g_muteAllTrigger.Execute();
					result = ERequestStatus::Success;
					g_systemStates |= ESystemStates::IsMuted;

					break;
				}
			case EDefaultTriggerType::UnmuteAll:
				{
					g_unmuteAllTrigger.Execute();
					result = ERequestStatus::Success;
					g_systemStates &= ~ESystemStates::IsMuted;

					break;
				}
			case EDefaultTriggerType::PauseAll:
				{
					g_pauseAllTrigger.Execute();
					result = ERequestStatus::Success;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					g_systemStates |= ESystemStates::IsPaused;
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE

					break;
				}
			case EDefaultTriggerType::ResumeAll:
				{
					g_resumeAllTrigger.Execute();
					result = ERequestStatus::Success;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					g_systemStates &= ~ESystemStates::IsPaused;
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE

					break;
				}
			default:
				break;
			}

			break;
		}
	case ESystemRequestType::StopAllSounds:
		{
			result = g_pIImpl->StopAllSounds();

			break;
		}
	case ESystemRequestType::ParseControlsData:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ParseControlsData> const*>(request.GetData());
			g_xmlProcessor.ParseControlsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ParsePreloadsData:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ParsePreloadsData> const*>(request.GetData());
			g_xmlProcessor.ParsePreloadsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ClearControlsData:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ClearControlsData> const*>(request.GetData());
			g_xmlProcessor.ClearControlsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ClearPreloadsData:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ClearPreloadsData> const*>(request.GetData());
			g_xmlProcessor.ClearPreloadsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::PreloadSingleRequest:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::PreloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryLoadRequest(pRequestData->preloadRequestId, ((request.flags & ERequestFlags::ExecuteBlocking) != 0), pRequestData->bAutoLoadOnly);

			break;
		}
	case ESystemRequestType::UnloadSingleRequest:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::UnloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryUnloadRequest(pRequestData->preloadRequestId);

			break;
		}
	case ESystemRequestType::SetParameter:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::SetParameter> const*>(request.GetData());

			CParameter const* const pParameter = stl::find_in_map(g_parameters, pRequestData->parameterId, nullptr);

			if (pParameter != nullptr)
			{
				pParameter->Set(g_object, pRequestData->value);
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::SetGlobalParameter:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::SetParameter> const*>(request.GetData());

			CParameter const* const pParameter = stl::find_in_map(g_parameters, pRequestData->parameterId, nullptr);

			if (pParameter != nullptr)
			{
				pParameter->SetGlobally(pRequestData->value);
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::SetSwitchState:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::SetSwitchState> const*>(request.GetData());

			CSwitch const* const pSwitch = stl::find_in_map(g_switches, pRequestData->switchId, nullptr);

			if (pSwitch != nullptr)
			{
				CSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), pRequestData->switchStateId, nullptr);

				if (pState != nullptr)
				{
					pState->Set(g_object);
					result = ERequestStatus::Success;
				}
			}

			break;
		}
	case ESystemRequestType::SetGlobalSwitchState:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::SetSwitchState> const*>(request.GetData());

			CSwitch const* const pSwitch = stl::find_in_map(g_switches, pRequestData->switchId, nullptr);

			if (pSwitch != nullptr)
			{
				CSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), pRequestData->switchStateId, nullptr);

				if (pState != nullptr)
				{
					pState->SetGlobally();
					result = ERequestStatus::Success;
				}
			}

			break;
		}
	case ESystemRequestType::PlayFile:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::PlayFile> const*>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				if (pRequestData->usedTriggerId != InvalidControlId)
				{
					CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->usedTriggerId, nullptr);

					if (pTrigger != nullptr)
					{
						pTrigger->PlayFile(
							g_object,
							pRequestData->file.c_str(),
							pRequestData->bLocalized,
							request.pOwner,
							request.pUserData,
							request.pUserDataOwner);
					}
				}

				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::StopFile:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::StopFile> const*>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				g_object.HandleStopFile(pRequestData->file.c_str());
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::AutoLoadSetting:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::AutoLoadSetting> const*>(request.GetData());

			for (auto const& settingPair : g_settings)
			{
				CSetting const* const pSetting = settingPair.second;

				if (pSetting->IsAutoLoad() && (pSetting->GetDataScope() == pRequestData->scope))
				{
					pSetting->Load();
					break;
				}
			}

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::LoadSetting:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::LoadSetting> const*>(request.GetData());

			CSetting const* const pSetting = stl::find_in_map(g_settings, pRequestData->id, nullptr);

			if (pSetting != nullptr)
			{
				pSetting->Load();
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::UnloadSetting:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::UnloadSetting> const*>(request.GetData());

			CSetting const* const pSetting = stl::find_in_map(g_settings, pRequestData->id, nullptr);

			if (pSetting != nullptr)
			{
				pSetting->Unload();
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::UnloadAFCMDataByScope:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::UnloadAFCMDataByScope> const*>(request.GetData());
			result = g_fileCacheManager.UnloadDataByScope(pRequestData->dataScope);

			break;
		}
	case ESystemRequestType::ReleaseImpl:
		{
			ReleaseImpl();
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ChangeLanguage:
		{
			SetImplLanguage();

			g_fileCacheManager.UpdateLocalizedFileCacheEntries();
			result = ERequestStatus::Success;

			break;
		}
#if defined(CRY_AUDIO_USE_OCCLUSION)
	case ESystemRequestType::ReleasePendingRays:
		{
			for (auto const pObject : g_activeObjects)
			{
				pObject->ReleasePendingRays();
			}

			result = ERequestStatus::Success;

			break;
		}
#endif // CRY_AUDIO_USE_OCCLUSION
	case ESystemRequestType::GetFileData:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::GetFileData> const*>(request.GetData());
			g_pIImpl->GetFileData(pRequestData->name.c_str(), pRequestData->fileData);
			break;
		}
	case ESystemRequestType::GetImplInfo:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::GetImplInfo> const*>(request.GetData());
			g_pIImpl->GetInfo(pRequestData->implInfo);
			break;
		}
	case ESystemRequestType::RegisterListener:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::RegisterListener> const*>(request.GetData());
			*pRequestData->ppListener = g_listenerManager.CreateListener(pRequestData->transformation, pRequestData->name.c_str());
			break;
		}
	case ESystemRequestType::ReleaseListener:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ReleaseListener> const*>(request.GetData());

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				g_listenerManager.ReleaseListener(pRequestData->pListener);
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::RegisterObject:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::RegisterObject> const*>(request.GetData());

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CObject");

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			auto const pNewObject = new CObject(pRequestData->transformation, pRequestData->name.c_str());
			pNewObject->Init(g_pIImpl->ConstructObject(pRequestData->transformation, pNewObject->GetName()), pRequestData->entityId);
			g_constructedObjects.push_back(pNewObject);
#else
			auto const pNewObject = new CObject(pRequestData->transformation);
			pNewObject->Init(g_pIImpl->ConstructObject(pRequestData->transformation), pRequestData->entityId);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

			if (pRequestData->setCurrentEnvironments)
			{
				SetCurrentEnvironmentsOnObject(pNewObject, INVALID_ENTITYID);
			}

#if defined(CRY_AUDIO_USE_OCCLUSION)
			SetOcclusionType(*pNewObject, pRequestData->occlusionType);
#endif  // CRY_AUDIO_USE_OCCLUSION
			*pRequestData->ppObject = pNewObject;
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ReleaseObject:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ReleaseObject> const*>(request.GetData());

			CRY_ASSERT(pRequestData->pObject != nullptr);

			if (pRequestData->pObject != nullptr)
			{
				if (pRequestData->pObject != &g_object)
				{
					pRequestData->pObject->RemoveFlag(EObjectFlags::InUse);

					if ((pRequestData->pObject->GetFlags() & EObjectFlags::Active) == 0)
					{
						pRequestData->pObject->Destruct();
					}
					result = ERequestStatus::Success;
				}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				else
				{
					Cry::Audio::Log(ELogType::Warning, "Audio System received a request to release the global object");
				}
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}

			break;
		}
	case ESystemRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	case ESystemRequestType::RefreshSystem:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::RefreshSystem> const*>(request.GetData());
			HandleRefresh(pRequestData->levelName);
			HandleRetriggerControls();
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ExecutePreviewTrigger:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecutePreviewTrigger> const*>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->Execute(g_previewObject, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				result = ERequestStatus::Success;
			}

			break;
		}
	case ESystemRequestType::ExecutePreviewTriggerEx:
		{
			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecutePreviewTriggerEx> const*>(request.GetData());

			g_previewTrigger.Execute(pRequestData->triggerInfo);
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::StopPreviewTrigger:
		{
			g_previewObject.StopAllTriggers();
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ResetRequestCount:
		{
			ZeroStruct(g_requestsPerUpdate);
			ZeroStruct(g_requestPeaks);
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::RetriggerControls:
		{
			HandleRetriggerControls();
			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::ReloadControlsData:
		{

			auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ReloadControlsData> const*>(request.GetData());

			for (auto const pObject : g_activeObjects)
			{
				pObject->StopAllTriggers();
			}

			g_xmlProcessor.ClearControlsData(EDataScope::All);
			g_xmlProcessor.ParseSystemData();
			g_xmlProcessor.ParseControlsData(pRequestData->folderPath, EDataScope::Global);

			if (strcmp(pRequestData->levelName, "") != 0)
			{
				g_xmlProcessor.ParseControlsData(pRequestData->folderPath + pRequestData->levelName, EDataScope::LevelSpecific);
			}

			HandleRetriggerControls();

			result = ERequestStatus::Success;

			break;
		}
	case ESystemRequestType::DrawDebugInfo:
		{
			HandleDrawDebug();
			m_canDraw = true;
			result = ERequestStatus::Success;

			break;
		}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
	default:
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, "Unknown manager request type: %u", pBase->systemRequestType);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessCallbackRequest(CRequest& request)
{
	ERequestStatus result = ERequestStatus::Failure;
	auto const pBase = static_cast<SCallbackRequestDataBase const*>(request.GetData());

	switch (pBase->callbackRequestType)
	{
	case ECallbackRequestType::ReportStartedFile:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportStartedFile> const*>(request.GetData());

			CStandaloneFile& standaloneFile = pRequestData->standaloneFile;

			if (standaloneFile.m_pObject != &g_object)
			{
				standaloneFile.m_pObject->GetStartedStandaloneFileRequestData(&standaloneFile, request);
			}
			else
			{
				g_object.GetStartedStandaloneFileRequestData(&standaloneFile, request);
			}

			standaloneFile.m_state = (pRequestData->bSuccess) ? EStandaloneFileState::Playing : EStandaloneFileState::None;

			result = (pRequestData->bSuccess) ? ERequestStatus::Success : ERequestStatus::Failure;

			break;
		}
	case ECallbackRequestType::ReportStoppedFile:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportStoppedFile> const*>(request.GetData());

			CStandaloneFile& standaloneFile = pRequestData->standaloneFile;

			if (standaloneFile.m_pObject != &g_object)
			{
				standaloneFile.m_pObject->GetStartedStandaloneFileRequestData(&standaloneFile, request);
				standaloneFile.m_pObject->ReportFinishedStandaloneFile(&standaloneFile);
			}
			else
			{
				g_object.GetStartedStandaloneFileRequestData(&standaloneFile, request);
				g_object.ReportFinishedStandaloneFile(&standaloneFile);
			}

			g_fileManager.ReleaseStandaloneFile(&standaloneFile);

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackRequestType::ReportStartedTriggerConnectionInstance:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> const*>(request.GetData());

			CObject* const pObject = stl::find_in_map(g_triggerInstanceIdToObject, pRequestData->triggerInstanceId, nullptr);

			if (pObject != nullptr)
			{
				pObject->ReportStartedTriggerInstance(pRequestData->triggerInstanceId, pRequestData->result);
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, "TriggerInstanceId %u is not mapped to an object during %s", pRequestData->triggerInstanceId, __FUNCTION__);
			}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackRequestType::ReportFinishedTriggerConnectionInstance:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance> const*>(request.GetData());

			CObject* const pObject = stl::find_in_map(g_triggerInstanceIdToObject, pRequestData->triggerInstanceId, nullptr);

			if (pObject != nullptr)
			{
				pObject->ReportFinishedTriggerInstance(pRequestData->triggerInstanceId, pRequestData->result);
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, "TriggerInstanceId %u is not mapped to an object during %s", pRequestData->triggerInstanceId, __FUNCTION__);
			}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackRequestType::ReportPhysicalizedObject:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> const*>(request.GetData());

			for (auto const pObject : g_activeObjects)
			{
				if (pObject->GetImplDataPtr() == pRequestData->pIObject)
				{
					pObject->RemoveFlag(EObjectFlags::Virtual);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE) && defined(CRY_AUDIO_USE_OCCLUSION)
					pObject->ResetObstructionRays();
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE && CRY_AUDIO_USE_OCCLUSION

					break;
				}
			}

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackRequestType::ReportVirtualizedObject:
		{
			auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject> const*>(request.GetData());

			for (auto const pObject : g_activeObjects)
			{
				if (pObject->GetImplDataPtr() == pRequestData->pIObject)
				{
					pObject->SetFlag(EObjectFlags::Virtual);
					break;
				}
			}

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackRequestType::ReportFinishedTriggerInstance: // Intentional fall-through.
	case ECallbackRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
	default:
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, "Unknown callback manager request type: %u", pBase->callbackRequestType);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessObjectRequest(CRequest const& request)
{
	ERequestStatus result = ERequestStatus::Failure;

	auto const pBase = static_cast<SObjectRequestDataBase const*>(request.GetData());
	CObject* const pObject = (pBase->pObject != nullptr) ? pBase->pObject : &g_object;

	switch (pBase->objectRequestType)
	{
	case EObjectRequestType::PlayFile:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::PlayFile> const*>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				if (pRequestData->usedTriggerId != InvalidControlId)
				{
					CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->usedTriggerId, nullptr);

					if (pTrigger != nullptr)
					{
						pTrigger->PlayFile(
							*pObject,
							pRequestData->file.c_str(),
							pRequestData->bLocalized,
							request.pOwner,
							request.pUserData,
							request.pUserDataOwner);
					}
				}

				result = ERequestStatus::Success;
			}

			break;
		}
	case EObjectRequestType::StopFile:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::StopFile> const*>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				pObject->HandleStopFile(pRequestData->file.c_str());
				result = ERequestStatus::Success;
			}

			break;
		}
	case EObjectRequestType::ExecuteTrigger:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ExecuteTrigger> const*>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->Execute(*pObject, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				result = ERequestStatus::Success;
			}

			break;
		}
	case EObjectRequestType::StopTrigger:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::StopTrigger> const*>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->HandleStopTrigger(pTrigger);
			}

			break;
		}
	case EObjectRequestType::StopAllTriggers:
		{
			pObject->StopAllTriggers();
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetTransformation:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetTransformation> const*>(request.GetData());

			pObject->HandleSetTransformation(pRequestData->transformation);
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetParameter:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetParameter> const*>(request.GetData());

			CParameter const* const pParameter = stl::find_in_map(g_parameters, pRequestData->parameterId, nullptr);

			if (pParameter != nullptr)
			{
				pParameter->Set(*pObject, pRequestData->value);
				result = ERequestStatus::Success;
			}

			break;
		}
	case EObjectRequestType::SetSwitchState:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetSwitchState> const*>(request.GetData());

			CSwitch const* const pSwitch = stl::find_in_map(g_switches, pRequestData->switchId, nullptr);

			if (pSwitch != nullptr)
			{
				CSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), pRequestData->switchStateId, nullptr);

				if (pState != nullptr)
				{
					pState->Set(*pObject);
					result = ERequestStatus::Success;
				}
			}

			break;
		}
#if defined(CRY_AUDIO_USE_OCCLUSION)
	case EObjectRequestType::SetOcclusionType:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetOcclusionType> const*>(request.GetData());

			SetOcclusionType(*pObject, pRequestData->occlusionType);
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetOcclusionRayOffset:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> const*>(request.GetData());

			pObject->HandleSetOcclusionRayOffset(pRequestData->occlusionRayOffset);
			result = ERequestStatus::Success;

			break;
		}
#endif // CRY_AUDIO_USE_OCCLUSION
	case EObjectRequestType::SetCurrentEnvironments:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> const*>(request.GetData());

			SetCurrentEnvironmentsOnObject(pObject, pRequestData->entityToIgnore);

			break;
		}
	case EObjectRequestType::SetEnvironment:
		{
			if (pObject != &g_object)
			{
				auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetEnvironment> const*>(request.GetData());

				CEnvironment const* const pEnvironment = stl::find_in_map(g_environments, pRequestData->environmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					pEnvironment->Set(*pObject, pRequestData->amount);
					result = ERequestStatus::Success;
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Audio System received a request to set an environment on the global object");
			}
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
#if defined(CRY_AUDIO_USE_OCCLUSION)
	case EObjectRequestType::ProcessPhysicsRay:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const*>(request.GetData());

			pObject->ProcessPhysicsRay(pRequestData->rayInfo);
			result = ERequestStatus::Success;

			break;
		}
#endif // CRY_AUDIO_USE_OCCLUSION
	case EObjectRequestType::ToggleAbsoluteVelocityTracking:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> const*>(request.GetData());

			if (pRequestData->isEnabled)
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(EObjectFunctionality::TrackAbsoluteVelocity, true);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				pObject->SetFlag(EObjectFlags::TrackAbsoluteVelocity);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}
			else
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(EObjectFunctionality::TrackAbsoluteVelocity, false);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				pObject->RemoveFlag(EObjectFlags::TrackAbsoluteVelocity);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}

			result = ERequestStatus::Success;
			break;
		}
	case EObjectRequestType::ToggleRelativeVelocityTracking:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> const*>(request.GetData());

			if (pRequestData->isEnabled)
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(EObjectFunctionality::TrackRelativeVelocity, true);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				pObject->SetFlag(EObjectFlags::TrackRelativeVelocity);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}
			else
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(EObjectFunctionality::TrackRelativeVelocity, false);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				pObject->RemoveFlag(EObjectFlags::TrackRelativeVelocity);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE
			}

			result = ERequestStatus::Success;
			break;
		}
	case EObjectRequestType::None:
		{
			result = ERequestStatus::Success;
			break;
		}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	case EObjectRequestType::SetName:
		{
			auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::SetName> const*>(request.GetData());

			result = pObject->HandleSetName(pRequestData->name.c_str());

			if (result == ERequestStatus::SuccessNeedsRefresh)
			{
				pObject->ForceImplementationRefresh(true);
				result = ERequestStatus::Success;
			}

			break;
		}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
	default:
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, "Unknown object request type: %u", pBase->objectRequestType);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessListenerRequest(SRequestData const* const pPassedRequestData)
{
	ERequestStatus result = ERequestStatus::Failure;
	auto const pBase = static_cast<SListenerRequestDataBase const*>(pPassedRequestData);

	switch (pBase->listenerRequestType)
	{
	case EListenerRequestType::SetTransformation:
		{
			auto const pRequestData = static_cast<SListenerRequestData<EListenerRequestType::SetTransformation> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				pRequestData->pListener->HandleSetTransformation(pRequestData->transformation);
			}

			result = ERequestStatus::Success;

			break;
		}

	case EListenerRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	case EListenerRequestType::SetName:
		{
			auto const pRequestData = static_cast<SListenerRequestData<EListenerRequestType::SetName> const*>(pPassedRequestData);

			pRequestData->pListener->HandleSetName(pRequestData->name.c_str());
			result = ERequestStatus::Success;

			break;
		}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
	default:
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, "Unknown listener request type: %u", pBase->listenerRequestType);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::NotifyListener(CRequest const& request)
{
	ESystemEvents systemEvent = ESystemEvents::None;
	CStandaloneFile* pStandaloneFile = nullptr;
	ControlId controlID = InvalidControlId;
	EntityId entityId = INVALID_ENTITYID;

	switch (request.GetData()->requestType)
	{
	case ERequestType::SystemRequest:
		{
			auto const pBase = static_cast<SSystemRequestDataBase const*>(request.GetData());

			switch (pBase->systemRequestType)
			{
			case ESystemRequestType::SetImpl:
				systemEvent = ESystemEvents::ImplSet;
				break;
			case ESystemRequestType::ExecuteTrigger:
				{
					auto const pRequestData = static_cast<SSystemRequestData<ESystemRequestType::ExecuteTrigger> const*>(pBase);
					controlID = pRequestData->triggerId;
					systemEvent = ESystemEvents::TriggerExecuted;

					break;
				}
			case ESystemRequestType::PlayFile:
				systemEvent = ESystemEvents::FilePlay;
				break;
			}

			break;
		}
	case ERequestType::CallbackRequest:
		{
			auto const pBase = static_cast<SCallbackRequestDataBase const*>(request.GetData());

			switch (pBase->callbackRequestType)
			{
			case ECallbackRequestType::ReportFinishedTriggerInstance:
				{
					auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const*>(pBase);
					controlID = pRequestData->triggerId;
					entityId = pRequestData->entityId;
					systemEvent = ESystemEvents::TriggerFinished;

					break;
				}
			case ECallbackRequestType::ReportStartedFile:
				{
					auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportStartedFile> const*>(pBase);
					pStandaloneFile = &pRequestData->standaloneFile;
					systemEvent = ESystemEvents::FileStarted;

					break;
				}
			case ECallbackRequestType::ReportStoppedFile:
				{
					auto const pRequestData = static_cast<SCallbackRequestData<ECallbackRequestType::ReportStoppedFile> const*>(pBase);
					pStandaloneFile = &pRequestData->standaloneFile;
					systemEvent = ESystemEvents::FileStopped;

					break;
				}
			}

			break;
		}
	case ERequestType::ObjectRequest:
		{
			auto const pBase = static_cast<SObjectRequestDataBase const*>(request.GetData());
			entityId = pBase->pObject->GetEntityId();

			switch (pBase->objectRequestType)
			{
			case EObjectRequestType::ExecuteTrigger:
				{
					auto const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ExecuteTrigger> const*>(pBase);
					controlID = pRequestData->triggerId;
					systemEvent = ESystemEvents::TriggerExecuted;

					break;
				}
			case EObjectRequestType::PlayFile:
				systemEvent = ESystemEvents::FilePlay;
				break;
			}

			break;
		}
	case ERequestType::ListenerRequest:
		{
			// Nothing to do currently for this type of request.

			break;
		}
	default:
		{
			CryFatalError("Unknown request type during %s!", __FUNCTION__);

			break;
		}
	}

	ERequestResult result = ERequestResult::Failure;

	switch (request.status)
	{
	case ERequestStatus::Success:
		{
			result = ERequestResult::Success;
			break;
		}
	case ERequestStatus::Failure:
	case ERequestStatus::PartialSuccess:
		{
			result = ERequestResult::Failure;
			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(false, "Invalid request status '%u'. Cannot be converted to a request result during %s", request.status, __FUNCTION__);
			result = ERequestResult::Failure;
			break;
		}
	}

	SRequestInfo const requestInfo(
		result,
		request.pOwner,
		request.pUserData,
		request.pUserDataOwner,
		systemEvent,
		controlID,
		entityId,
		pStandaloneFile);

	g_eventListenerManager.NotifyListener(&requestInfo);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::HandleSetImpl(Impl::IImpl* const pIImpl)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (g_pIImpl != nullptr && pIImpl != g_pIImpl)
	{
		ReleaseImpl();
	}

	g_pIImpl = pIImpl;

	if (g_pIImpl == nullptr)
	{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, "nullptr passed to SetImpl, will run with the null implementation");
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::Impl::Null::CImpl");
		auto const pImpl = new Impl::Null::CImpl();
		CRY_ASSERT(pImpl != nullptr);
		g_pIImpl = static_cast<Impl::IImpl*>(pImpl);
	}

	g_xmlProcessor.ParseSystemData();

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if ((g_systemStates& ESystemStates::PoolsAllocated) == 0)
	{
		// Don't allocate again after impl switch.
		AllocateMemoryPools();
		g_systemStates |= ESystemStates::PoolsAllocated;
	}
#else
	AllocateMemoryPools();
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	result = g_pIImpl->Init(m_objectPoolSize);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	// Get impl info again (was done in ParseSystemData) to set the impl name, because
	// it's not guaranteed that it already existed in the impl constructor.
	g_pIImpl->GetInfo(g_implInfo);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (result != ERequestStatus::Success)
	{
		// The impl failed to initialize, allow it to shut down and release then fall back to the null impl.

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Error, "Failed to set the AudioImpl %s. Will run with the null implementation.", m_implInfo.name.c_str());
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		// There's no need to call Shutdown when the initialization failed as
		// we expect the implementation to clean-up itself if it couldn't be initialized

		g_pIImpl->Release(); // Release the engine specific data.

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::Impl::Null::CImpl");
		auto const pImpl = new Impl::Null::CImpl();
		CRY_ASSERT(pImpl != nullptr);
		g_pIImpl = static_cast<Impl::IImpl*>(pImpl);
	}

	CRY_ASSERT_MESSAGE(g_object.GetImplDataPtr() == nullptr, "<Audio> The global object's impl-data must be nullptr during %s", __FUNCTION__);
	g_object.SetImplDataPtr(g_pIImpl->ConstructGlobalObject());

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CRY_ASSERT_MESSAGE(g_previewObject.GetImplDataPtr() == nullptr, "<Audio> The preview object's impl-data must be nullptr during %s", __FUNCTION__);
	g_previewObject.SetImplDataPtr(g_pIImpl->ConstructObject(g_previewObject.GetTransformation(), g_previewObject.GetName()));

	for (auto const pObject : g_constructedObjects)
	{
		CRY_ASSERT_MESSAGE(pObject->GetImplDataPtr() == nullptr, "<Audio> The object's impl-data must be nullptr during %s", __FUNCTION__);

		pObject->SetImplDataPtr(g_pIImpl->ConstructObject(pObject->GetTransformation(), pObject->GetName()));
	}

#endif // CRY_AUDIO_USE_PRODUCTION_CODE
	g_listenerManager.OnAfterImplChanged();

	SetImplLanguage();

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetImplLanguage()
{
	if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		g_pIImpl->SetLanguage(pCVar->GetString());
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetCurrentEnvironmentsOnObject(CObject* const pObject, EntityId const entityToIgnore)
{
	IAreaManager* const pIAreaManager = gEnv->pEntitySystem->GetAreaManager();
	size_t numAreas = 0;
	static size_t const s_maxAreas = 10;
	static SAudioAreaInfo s_areaInfos[s_maxAreas];

	if (pIAreaManager->QueryAudioAreas(pObject->GetTransformation().GetPosition(), s_areaInfos, s_maxAreas, numAreas))
	{
		for (size_t i = 0; i < numAreas; ++i)
		{
			SAudioAreaInfo const& areaInfo = s_areaInfos[i];

			if (entityToIgnore == INVALID_ENTITYID || entityToIgnore != areaInfo.envProvidingEntityId)
			{
				CEnvironment const* const pEnvironment = stl::find_in_map(g_environments, areaInfo.audioEnvironmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					pEnvironment->Set(*pObject, areaInfo.amount);
				}
			}
		}
	}
}

#if defined(CRY_AUDIO_USE_OCCLUSION)
//////////////////////////////////////////////////////////////////////////
void CSystem::SetOcclusionType(CObject& object, EOcclusionType const occlusionType) const
{
	switch (occlusionType)
	{
	case EOcclusionType::Ignore:
		{
			object.HandleSetOcclusionType(EOcclusionType::Ignore);
			object.SetOcclusion(0.0f);

			break;
		}
	case EOcclusionType::Adaptive:
		{
			object.HandleSetOcclusionType(EOcclusionType::Adaptive);

			break;
		}
	case EOcclusionType::Low:
		{
			object.HandleSetOcclusionType(EOcclusionType::Low);

			break;
		}
	case EOcclusionType::Medium:
		{
			object.HandleSetOcclusionType(EOcclusionType::Medium);

			break;
		}
	case EOcclusionType::High:
		{
			object.HandleSetOcclusionType(EOcclusionType::High);

			break;
		}
	default:
		{
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, "Unknown occlusion type during %s: %u", __FUNCTION__, occlusionType);
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

			break;
		}
	}
}
#endif // CRY_AUDIO_USE_OCCLUSION

//////////////////////////////////////////////////////////////////////////
void CSystem::OnCallback(SRequestInfo const* const pRequestInfo)
{
	if ((gEnv->mMainThreadId == CryGetCurrentThreadId()) && (pRequestInfo->entityId != INVALID_ENTITYID))
	{
		IEntity* const pIEntity = gEnv->pEntitySystem->GetEntity(pRequestInfo->entityId);

		if (pIEntity != nullptr)
		{
			SEntityEvent eventData;
			eventData.nParam[0] = reinterpret_cast<intptr_t>(pRequestInfo);

			if (pRequestInfo->systemEvent == ESystemEvents::TriggerExecuted)
			{
				eventData.event = ENTITY_EVENT_AUDIO_TRIGGER_STARTED;
				pIEntity->SendEvent(eventData);
			}

			if ((pRequestInfo->systemEvent == ESystemEvents::TriggerFinished) ||
			    ((pRequestInfo->systemEvent == ESystemEvents::TriggerExecuted) && (pRequestInfo->requestResult != ERequestResult::Success)))
			{
				eventData.event = ENTITY_EVENT_AUDIO_TRIGGER_ENDED;
				pIEntity->SendEvent(eventData);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetImplInfo(SImplInfo& implInfo)
{
	SSystemRequestData<ESystemRequestType::GetImplInfo> const requestData(implInfo);
	CRequest const request(&requestData, ERequestFlags::ExecuteBlocking);
	PushRequest(request);
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CSystem::HandleRefresh(char const* const szLevelName)
{
	Cry::Audio::Log(ELogType::Warning, "Beginning to refresh the AudioSystem!");

	ERequestStatus result = g_pIImpl->StopAllSounds();
	CRY_ASSERT(result == ERequestStatus::Success);

	result = g_fileCacheManager.UnloadDataByScope(EDataScope::LevelSpecific);
	CRY_ASSERT(result == ERequestStatus::Success);

	result = g_fileCacheManager.UnloadDataByScope(EDataScope::Global);
	CRY_ASSERT(result == ERequestStatus::Success);

	g_xmlProcessor.ClearPreloadsData(EDataScope::All);
	g_xmlProcessor.ClearControlsData(EDataScope::All);

	ResetRequestCount();

	g_pIImpl->OnRefresh();

	g_xmlProcessor.ParseSystemData();
	g_xmlProcessor.ParseControlsData(g_configPath.c_str(), EDataScope::Global);
	g_xmlProcessor.ParsePreloadsData(g_configPath.c_str(), EDataScope::Global);

	// The global preload might not exist if no preloads have been created, for that reason we don't check the result of this call
	g_fileCacheManager.TryLoadRequest(GlobalPreloadRequestId, true, true);

	AutoLoadSetting(EDataScope::Global);

	if (szLevelName != nullptr && szLevelName[0] != '\0')
	{
		CryFixedStringT<MaxFilePathLength> levelPath = g_configPath;
		levelPath += g_szLevelsFolderName;
		levelPath += "/";
		levelPath += szLevelName;
		g_xmlProcessor.ParseControlsData(levelPath.c_str(), EDataScope::LevelSpecific);
		g_xmlProcessor.ParsePreloadsData(levelPath.c_str(), EDataScope::LevelSpecific);

		PreloadRequestId const preloadRequestId = StringToId(szLevelName);
		result = g_fileCacheManager.TryLoadRequest(preloadRequestId, true, true);

		if (result != ERequestStatus::Success)
		{
			Cry::Audio::Log(ELogType::Warning, R"(No preload request found for level - "%s"!)", szLevelName);
		}

		AutoLoadSetting(EDataScope::LevelSpecific);
	}

	Cry::Audio::Log(ELogType::Warning, "Done refreshing the AudioSystem!");
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ScheduleIRenderAuxGeomForRendering(IRenderAuxGeom* pRenderAuxGeom)
{
	auto oldRenderAuxGeom = m_currentRenderAuxGeom.exchange(pRenderAuxGeom);
	CRY_ASSERT(oldRenderAuxGeom != pRenderAuxGeom);

	// Kill FIFO entries beyond 1, only the head survives in m_currentRenderAuxGeom
	// Throw away all older entries
	if (oldRenderAuxGeom && oldRenderAuxGeom != pRenderAuxGeom)
	{
		gEnv->pRenderer->DeleteAuxGeom(oldRenderAuxGeom);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SubmitLastIRenderAuxGeomForRendering()
{
	// Consume the FIFO head
	auto curRenderAuxGeom = m_currentRenderAuxGeom.exchange(nullptr);
	if (curRenderAuxGeom)
	{
		// Replace the active Aux rendering by a new one only if there is a new one
		// Otherwise keep rendering the currently active one
		auto oldRenderAuxGeom = m_lastRenderAuxGeom.exchange(curRenderAuxGeom);
		if (oldRenderAuxGeom)
		{
			gEnv->pRenderer->DeleteAuxGeom(oldRenderAuxGeom);
		}
	}

	if (m_lastRenderAuxGeom != (volatile IRenderAuxGeom*)nullptr)
	{
		gEnv->pRenderer->SubmitAuxGeom(m_lastRenderAuxGeom);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::DrawDebug()
{
	if (g_cvars.m_drawDebug > 0)
	{
		SubmitLastIRenderAuxGeomForRendering();

		if (m_canDraw)
		{
			m_canDraw = false;

			SSystemRequestData<ESystemRequestType::DrawDebugInfo> const requestData;
			CRequest const request(&requestData);
			PushRequest(request);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void DrawMemoryPoolInfo(
	IRenderAuxGeom* const pAuxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType,
	uint16 const poolSize)
{
	CryFixedStringT<MaxMiscStringLength> memAllocString;

	if (mem.nAlloc < 1024)
	{
		memAllocString.Format("%" PRISIZE_T " Byte", mem.nAlloc);
	}
	else
	{
		memAllocString.Format("%" PRISIZE_T " KiB", mem.nAlloc >> 10);
	}

	ColorF const color = (static_cast<uint16>(pool.nUsed) > poolSize) ? Debug::s_globalColorError : Debug::s_systemColorTextPrimary;

	posY += Debug::g_systemLineHeight;
	pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, color, false,
	                      "[%s] Constructed: %" PRISIZE_T " | Allocated: %" PRISIZE_T " | Preallocated: %u | Pool Size: %s",
	                      szType, pool.nUsed, pool.nAlloc, poolSize, memAllocString.c_str());
}

//////////////////////////////////////////////////////////////////////////
void DrawRequestCategoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, char const* const szType)
{
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false, "%s Request Peak:", szType);
	posY += Debug::g_systemLineHeight;
}

//////////////////////////////////////////////////////////////////////////
void DrawRequestPeakInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, char const* const szType, uint16 const peak, uint16 poolSize)
{
	bool const poolSizeExceeded = (peak > poolSize) && (poolSize != 0);
	CryFixedStringT<MaxMiscStringLength> debugText;

	if (poolSizeExceeded)
	{
		debugText.Format("%s: %u (Pool Size: %u)", szType, peak, poolSize);
	}
	else
	{
		debugText.Format("%s: %u", szType, peak);
	}

	auxGeom.Draw2dLabel(
		posX,
		posY,
		Debug::g_systemFontSize,
		poolSizeExceeded ? Debug::s_globalColorError : Debug::s_systemColorTextPrimary,
		false,
		debugText.c_str());
	posY += Debug::g_systemLineHeight;
}

//////////////////////////////////////////////////////////////////////////
void DrawRequestDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY)
{
	auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Audio Requests");
	posY += Debug::g_listHeaderLineHeight;

	DrawRequestPeakInfo(auxGeom, posX, posY, "Total", g_requestPeaks.requests, 0);

	DrawRequestCategoryInfo(auxGeom, posX, posY, "System");
	DrawRequestPeakInfo(auxGeom, posX, posY, "ExecuteTrigger", g_requestPeaks.systemExecuteTrigger, g_systemExecuteTriggerPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ExecuteTriggerEx", g_requestPeaks.systemExecuteTriggerEx, g_systemExecuteTriggerExPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "StopTrigger", g_requestPeaks.systemStopTrigger, g_systemStopTriggerPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "RegisterObject", g_requestPeaks.systemRegisterObject, g_systemRegisterObjectPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReleaseObject", g_requestPeaks.systemReleaseObject, g_systemReleaseObjectPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetParameter", g_requestPeaks.systemSetParameter, g_systemSetParameterPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetSwitchState", g_requestPeaks.systemSetSwitchState, g_systemSetSwitchStatePoolSize);

	DrawRequestCategoryInfo(auxGeom, posX, posY, "Object");
	DrawRequestPeakInfo(auxGeom, posX, posY, "ExecuteTrigger", g_requestPeaks.objectExecuteTrigger, g_objectExecuteTriggerPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "StopTrigger", g_requestPeaks.objectStopTrigger, g_objectStopTriggerPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetTransformation", g_requestPeaks.objectSetTransformation, g_objectSetTransformationPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetParameter", g_requestPeaks.objectSetParameter, g_objectSetParameterPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetSwitchState", g_requestPeaks.objectSetSwitchState, g_objectSetSwitchStatePoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetCurrentEnvironments", g_requestPeaks.objectSetCurrentEnvironments, g_objectSetCurrentEnvironmentsPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetEnvironment", g_requestPeaks.objectSetEnvironment, g_objectSetEnvironmentPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ProcessPhysicsRay", g_requestPeaks.objectProcessPhysicsRay, g_objectProcessPhysicsRayPoolSize);

	DrawRequestCategoryInfo(auxGeom, posX, posY, "Listener");
	DrawRequestPeakInfo(auxGeom, posX, posY, "SetTransformation", g_requestPeaks.listenerSetTransformation, g_listenerSetTransformationPoolSize);

	DrawRequestCategoryInfo(auxGeom, posX, posY, "Callback");
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReportStartedTriggerConnectionInstance", g_requestPeaks.callbackReportStartedriggerConnectionInstance, g_callbackReportStartedTriggerConnectionInstancePoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReportFinishedTriggerConnectionInstance", g_requestPeaks.callbackReportFinishedTriggerConnectionInstance, g_callbackReportFinishedTriggerConnectionInstancePoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReportFinishedTriggerInstance", g_requestPeaks.callbackReportFinishedTriggerInstance, g_callbackReportFinishedTriggerInstancePoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReportPhysicalizedObject", g_requestPeaks.callbackReportPhysicalizedObject, g_callbackReportPhysicalizedObjectPoolSize);
	DrawRequestPeakInfo(auxGeom, posX, posY, "ReportVirtualizedObject", g_requestPeaks.callbackReportVirtualizedObject, g_callbackReportVirtualizedObjectPoolSize);
}

//////////////////////////////////////////////////////////////////////////
void DrawObjectInfo(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	CObject const& object,
	CryFixedStringT<MaxControlNameLength> const& lowerCaseSearchString,
	size_t& numObjects)
{
	Vec3 const& position = object.GetTransformation().GetPosition();
	float const distance = position.GetDistance(g_listenerManager.GetActiveListenerTransformation().GetPosition());

	if (g_cvars.m_debugDistance <= 0.0f || (g_cvars.m_debugDistance > 0.0f && distance < g_cvars.m_debugDistance))
	{
		char const* const szObjectName = object.GetName();
		CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
		lowerCaseObjectName.MakeLower();
		bool const hasActiveData = (object.GetFlags() & EObjectFlags::Active) != 0;
		bool const isVirtual = (object.GetFlags() & EObjectFlags::Virtual) != 0;
		bool const stringFound = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0)) || (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
		bool const draw = stringFound && ((g_cvars.m_hideInactiveObjects == 0) || ((g_cvars.m_hideInactiveObjects != 0) && hasActiveData && !isVirtual));

		if (draw)
		{
			CryFixedStringT<MaxMiscStringLength> debugText;

			if ((object.GetFlags() & EObjectFlags::InUse) != 0)
			{
				debugText.Format(szObjectName);
			}
			else
			{
				debugText.Format("%s [To Be Released]", szObjectName);
			}

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize,
			                    !hasActiveData ? Debug::s_globalColorInactive : (isVirtual ? Debug::s_globalColorVirtual : Debug::s_listColorItemActive),
			                    false,
			                    debugText.c_str());

			posY += Debug::g_listLineHeight;
			++numObjects;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void DrawObjectDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY)
{
	size_t numObjects = 0;
	float const headerPosY = posY;
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
	lowerCaseSearchString.MakeLower();

	posY += Debug::g_listHeaderLineHeight;

	DrawObjectInfo(auxGeom, posX, posY, g_object, lowerCaseSearchString, numObjects);
	DrawObjectInfo(auxGeom, posX, posY, g_previewObject, lowerCaseSearchString, numObjects);

	for (auto const pObject : g_constructedObjects)
	{
		DrawObjectInfo(auxGeom, posX, posY, *pObject, lowerCaseSearchString, numObjects);
	}

	auxGeom.Draw2dLabel(posX, headerPosY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Audio Objects [%" PRISIZE_T "]", numObjects);
}

//////////////////////////////////////////////////////////////////////////
void DrawPerActiveObjectDebugInfo(IRenderAuxGeom& auxGeom)
{
	for (auto const pObject : g_activeObjects)
	{
		pObject->DrawDebugInfo(auxGeom);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::HandleDrawDebug()
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);
	IRenderAuxGeom* const pAuxGeom = gEnv->pRenderer ? gEnv->pRenderer->GetOrCreateIRenderAuxGeom() : nullptr;

	if (pAuxGeom != nullptr)
	{
		if ((g_cvars.m_drawDebug & Debug::objectMask) != 0)
		{
			// Needs to be called first so that the rest of the labels are printed on top.
			// (Draw2dLabel doesn't provide a way to set which labels are printed on top)
			DrawPerActiveObjectDebugInfo(*pAuxGeom);
		}

		float posX = 8.0f;
		float posY = 4.0f;

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::HideMemoryInfo) == 0)
		{
			CryModuleMemoryInfo memInfo;
			ZeroStruct(memInfo);
			CryGetMemoryInfoForModule(&memInfo);

			CryFixedStringT<MaxMiscStringLength> memInfoString;
			auto const memAlloc = static_cast<uint32>(memInfo.allocated - memInfo.freed);

			if (memAlloc < 1024)
			{
				memInfoString.Format("%u Byte", memAlloc);
			}
			else
			{
				memInfoString.Format("%u KiB", memAlloc >> 10);
			}

			char const* const szMuted = ((g_systemStates& ESystemStates::IsMuted) != 0) ? " - Muted" : "";
			char const* const szPaused = ((g_systemStates& ESystemStates::IsPaused) != 0) ? " - Paused" : "";

			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemHeaderFontSize, Debug::s_globalColorHeader, false,
			                      "Audio System (Total Memory: %s)%s%s", memInfoString.c_str(), szMuted, szPaused);

			posY += Debug::g_systemHeaderLineSpacerHeight;

			if ((g_cvars.m_drawDebug & Debug::EDrawFilter::DetailedMemoryInfo) != 0)
			{
				{
					auto& allocator = CObject::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", m_objectPoolSize);
				}

				{
					auto& allocator = CStandaloneFile::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Standalone Files", static_cast<uint16>(g_cvars.m_standaloneFilePoolSize));
				}

				if (g_debugPoolSizes.triggers > 0)
				{
					auto& allocator = CTrigger::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers", g_poolSizes.triggers);
				}

				if (g_debugPoolSizes.parameters > 0)
				{
					auto& allocator = CParameter::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters", g_poolSizes.parameters);
				}

				if (g_debugPoolSizes.switches > 0)
				{
					auto& allocator = CSwitch::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Switches", g_poolSizes.switches);
				}

				if (g_debugPoolSizes.states > 0)
				{
					auto& allocator = CSwitchState::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SwitchStates", g_poolSizes.states);
				}

				if (g_debugPoolSizes.environments > 0)
				{
					auto& allocator = CEnvironment::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Environments", g_poolSizes.environments);
				}

				if (g_debugPoolSizes.preloads > 0)
				{
					auto& allocator = CPreloadRequest::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Preloads", g_poolSizes.preloads);
				}

				if (g_debugPoolSizes.settings > 0)
				{
					auto& allocator = CSetting::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Settings", g_poolSizes.settings);
				}
			}

			size_t const numObjects = g_constructedObjects.size();
			size_t const numActiveObjects = g_activeObjects.size();
			size_t const numListeners = g_listenerManager.GetNumActiveListeners();
			size_t const numEventListeners = g_eventListenerManager.GetNumEventListeners();
			static float const SMOOTHING_ALPHA = 0.2f;
			static float syncRays = 0;
			static float asyncRays = 0;
			syncRays += (CPropagationProcessor::s_totalSyncPhysRays - syncRays) * SMOOTHING_ALPHA;
			asyncRays += (CPropagationProcessor::s_totalAsyncPhysRays - asyncRays) * SMOOTHING_ALPHA * 0.1f;

			posY += Debug::g_systemLineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false,
			                      "Objects: %3" PRISIZE_T "/%3" PRISIZE_T " | EventListeners %3" PRISIZE_T " | Listeners: %" PRISIZE_T " | SyncRays: %3.1f AsyncRays: %3.1f",
			                      numActiveObjects, numObjects, numEventListeners, numListeners, syncRays, asyncRays);

			if (g_pIImpl != nullptr)
			{
				posY += Debug::g_systemHeaderLineHeight;
				g_pIImpl->DrawDebugMemoryInfo(*pAuxGeom, posX, posY, (g_cvars.m_drawDebug & Debug::EDrawFilter::DetailedMemoryInfo) != 0);
			}

			posY += Debug::g_systemHeaderLineHeight;
		}

		string debugFilter = g_cvars.m_pDebugFilter->GetString();

		if (debugFilter.IsEmpty() || debugFilter == "0")
		{
			debugFilter = "<none>";
		}

		string debugDistance = ToString(g_cvars.m_debugDistance) + " m";

		if (g_cvars.m_debugDistance <= 0)
		{
			debugDistance = "<infinite>";
		}

		string debugDraw = "";

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::Spheres) != 0)
		{
			debugDraw += "Spheres, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectLabel) != 0)
		{
			debugDraw += "Labels, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectTriggers) != 0)
		{
			debugDraw += "Triggers, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectStates) != 0)
		{
			debugDraw += "States, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectParameters) != 0)
		{
			debugDraw += "Parameters, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectEnvironments) != 0)
		{
			debugDraw += "Environments, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectDistance) != 0)
		{
			debugDraw += "Distances, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRayLabels) != 0)
		{
			debugDraw += "Occlusion Ray Labels, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRays) != 0)
		{
			debugDraw += "Occlusion Rays, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRayOffset) != 0)
		{
			debugDraw += "Occlusion Ray Offset, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ListenerOcclusionPlane) != 0)
		{
			debugDraw += "Listener Occlusion Plane, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectStandaloneFiles) != 0)
		{
			debugDraw += "Object Standalone Files, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectImplInfo) != 0)
		{
			debugDraw += "Object Middleware Info, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::StandaloneFiles) != 0)
		{
			debugDraw += "Standalone Files, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ImplList) != 0)
		{
			debugDraw += "Implementation List, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ActiveObjects) != 0)
		{
			debugDraw += "Active Objects, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::FileCacheManagerInfo) != 0)
		{
			debugDraw += "File Cache Manager, ";
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::RequestInfo) != 0)
		{
			debugDraw += "Requests, ";
		}

		if (!debugDraw.IsEmpty())
		{
			debugDraw.erase(debugDraw.length() - 2, 2);
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextPrimary, false, "Debug Draw: %s", debugDraw.c_str());
			posY += Debug::g_systemLineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextPrimary, false, "Debug Filter: %s | Debug Distance: %s", debugFilter.c_str(), debugDistance.c_str());

			posY += Debug::g_systemHeaderLineHeight;
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::FileCacheManagerInfo) != 0)
		{
			g_fileCacheManager.DrawDebugInfo(*pAuxGeom, posX, posY);
			posX += 600.0f;
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ActiveObjects) != 0)
		{
			DrawObjectDebugInfo(*pAuxGeom, posX, posY);
			posX += 300.0f;
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ImplList) != 0)
		{
			if (g_pIImpl != nullptr)
			{
				g_pIImpl->DrawDebugInfoList(*pAuxGeom, posX, posY, g_cvars.m_debugDistance, g_cvars.m_pDebugFilter->GetString());
				// The impl is responsible for increasing posX.
			}
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::RequestInfo) != 0)
		{
			DrawRequestDebugInfo(*pAuxGeom, posX, posY);
			posX += 600.0f;
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::StandaloneFiles) != 0)
		{
			g_fileManager.DrawDebugInfo(*pAuxGeom, posX, posY);
		}
	}

	g_system.ScheduleIRenderAuxGeomForRendering(pAuxGeom);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::HandleRetriggerControls()
{
	for (auto const pObject : g_constructedObjects)
	{
		pObject->ForceImplementationRefresh(true);
	}

	g_object.ForceImplementationRefresh(false);

	g_previewObject.ForceImplementationRefresh(false);

	if ((g_systemStates& ESystemStates::IsMuted) != 0)
	{
		ExecuteDefaultTrigger(EDefaultTriggerType::MuteAll);
	}

	if ((g_systemStates& ESystemStates::IsPaused) != 0)
	{
		ExecuteDefaultTrigger(EDefaultTriggerType::PauseAll);
	}
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}      // namespace CryAudio
