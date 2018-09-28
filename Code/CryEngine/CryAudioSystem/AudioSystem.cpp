// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioSystem.h"
#include "AudioImpl.h"
#include "PoolObject_impl.h"
#include "Common/Logger.h"
#include "Managers.h"
#include "AudioObjectManager.h"
#include "AudioEventManager.h"
#include "FileCacheManager.h"
#include "AudioListenerManager.h"
#include "AudioEventListenerManager.h"
#include "AudioXMLProcessor.h"
#include "AudioStandaloneFileManager.h"
#include "AudioCVars.h"
#include "ATLAudioObject.h"
#include "PropagationProcessor.h"
#include <CrySystem/ITimer.h>
#include <CryString/CryPath.h>
#include <CryEntitySystem/IEntitySystem.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "DebugColor.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

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

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Trigger Pool");
	CTrigger::CreateAllocator(g_poolSizes.triggers);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Parameter Pool");
	CParameter::CreateAllocator(g_poolSizes.parameters);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Switch Pool");
	CATLSwitch::CreateAllocator(g_poolSizes.switches);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System SwitchState Pool");
	CATLSwitchState::CreateAllocator(g_poolSizes.states);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Environment Pool");
	CATLAudioEnvironment::CreateAllocator(g_poolSizes.environments);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Preload Pool");
	CATLPreloadRequest::CreateAllocator(g_poolSizes.preloads);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Settings Pool");
	CSetting::CreateAllocator(g_poolSizes.settings);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Trigger Connection Pool");
	CATLTriggerImpl::CreateAllocator(g_poolSizes.triggerConnections);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Parameter Connection Pool");
	CParameterImpl::CreateAllocator(g_poolSizes.parameterConnections);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System State Connection Pool");
	CSwitchStateImpl::CreateAllocator(g_poolSizes.stateConnections);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Environment Connection Pool");
	CATLEnvironmentImpl::CreateAllocator(g_poolSizes.environmentConnections);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Setting Connection Pool");
	CSettingImpl::CreateAllocator(g_poolSizes.settingConnections);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CTrigger::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CATLSwitch::FreeMemoryPool();
	CATLSwitchState::FreeMemoryPool();
	CATLAudioEnvironment::FreeMemoryPool();
	CATLPreloadRequest::FreeMemoryPool();
	CSetting::FreeMemoryPool();

	CATLTriggerImpl::FreeMemoryPool();
	CParameterImpl::FreeMemoryPool();
	CSwitchStateImpl::FreeMemoryPool();
	CATLEnvironmentImpl::FreeMemoryPool();
	CSettingImpl::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CMainThread::ThreadEntry()
{
	while (m_doWork)
	{
		g_system.InternalUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainThread::SignalStopWork()
{
	m_doWork = false;
}

///////////////////////////////////////////////////////////////////////////
void CMainThread::Activate()
{
	if (!gEnv->pThreadManager->SpawnThread(this, "AudioMainThread"))
	{
		CryFatalError(R"(Error spawning "AudioMainThread" thread.)");
	}
}

///////////////////////////////////////////////////////////////////////////
void CMainThread::Deactivate()
{
	SignalStopWork();
	gEnv->pThreadManager->JoinThread(this, eJM_Join);
}

///////////////////////////////////////////////////////////////////////////
bool CMainThread::IsActive()
{
	// JoinThread returns true if thread is not running.
	// JoinThread returns false if thread is still running
	return !gEnv->pThreadManager->JoinThread(this, eJM_TryJoin);
}

//////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
	CRY_ASSERT_MESSAGE(g_pIImpl == nullptr, "<Audio> The implementation must get destroyed before the audio system is destructed.");
	CRY_ASSERT_MESSAGE(g_pObject == nullptr, "<Audio> The global object must get destroyed before the audio system is destructed.");
}

//////////////////////////////////////////////////////////////////////////
void CSystem::AddRequestListener(void (*func)(SRequestInfo const* const), void* const pObjectToListenTo, ESystemEvents const eventMask)
{
	SManagerRequestData<EManagerRequestType::AddRequestListener> const requestData(pObjectToListenTo, func, eventMask);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RemoveRequestListener(void (*func)(SRequestInfo const* const), void* const pObjectToListenTo)
{
	SManagerRequestData<EManagerRequestType::RemoveRequestListener> const requestData(pObjectToListenTo, func);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExternalUpdate()
{
	CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: External Update");

	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	CAudioRequest request;

	while (m_syncCallbacks.dequeue(request))
	{
		NotifyListener(request);

		if (request.pObject == nullptr)
		{
			g_pObject->DecrementSyncCallbackCounter();
		}
		else
		{
			request.pObject->DecrementSyncCallbackCounter();
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	DrawDebug();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_accumulatedFrameTime += gEnv->pTimer->GetFrameTime();
	++m_externalThreadFrameId;

	// If sleeping, wake up the audio thread to start processing requests again.
	m_audioThreadWakeupEvent.Set();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PushRequest(CAudioRequest const& request)
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
		Log(ELogType::Warning, "Discarded PushRequest due to ATL not allowing for new ones!");
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
				CryFixedStringT<MaxFilePathLength> audioLevelPath(g_configPath);
				audioLevelPath += s_szLevelsFolderName;
				audioLevelPath += "/";
				audioLevelPath += levelNameOnly;

				SManagerRequestData<EManagerRequestType::ParseControlsData> const requestData1(audioLevelPath, EDataScope::LevelSpecific);
				CAudioRequest const request1(&requestData1);
				PushRequest(request1);

				SManagerRequestData<EManagerRequestType::ParsePreloadsData> const requestData2(audioLevelPath, EDataScope::LevelSpecific);
				CAudioRequest const request2(&requestData2);
				PushRequest(request2);

				PreloadRequestId const preloadRequestId = StringToId(levelNameOnly.c_str());
				SManagerRequestData<EManagerRequestType::PreloadSingleRequest> const requestData3(preloadRequestId, true);
				CAudioRequest const request3(&requestData3);
				PushRequest(request3);

				SManagerRequestData<EManagerRequestType::AutoLoadSetting> const requestData4(EDataScope::LevelSpecific);
				CAudioRequest const request4(&requestData4);
				PushRequest(request4);
			}

			break;
		}
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			// This event is issued in Editor and Game mode.
			CPropagationProcessor::s_bCanIssueRWIs = false;

			SManagerRequestData<EManagerRequestType::ReleasePendingRays> const requestData1;
			CAudioRequest const request1(&requestData1);
			PushRequest(request1);

			SManagerRequestData<EManagerRequestType::UnloadAFCMDataByScope> const requestData2(EDataScope::LevelSpecific);
			CAudioRequest const request2(&requestData2);
			PushRequest(request2);

			SManagerRequestData<EManagerRequestType::ClearControlsData> const requestData3(EDataScope::LevelSpecific);
			CAudioRequest const request3(&requestData3);
			PushRequest(request3);

			SManagerRequestData<EManagerRequestType::ClearPreloadsData> const requestData4(EDataScope::LevelSpecific);
			CAudioRequest const request4(&requestData4);
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
	case ESYSTEM_EVENT_FULL_SHUTDOWN:
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
			g_pObject->GetImplDataPtr()->Update(m_accumulatedFrameTime);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			g_previewObject.GetImplDataPtr()->Update(m_accumulatedFrameTime);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			g_objectManager.Update(m_accumulatedFrameTime);
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
			g_pObject->GetImplDataPtr()->Update(0.0f);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			g_previewObject.GetImplDataPtr()->Update(0.0f);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			g_objectManager.Update(0.0f);
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
#if defined(ENABLE_AUDIO_LOGGING)
		REGISTER_CVAR2("s_LoggingOptions", &m_loggingOptions, AlphaBits64("abc"), VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
		               "Toggles the logging of audio related messages.\n"
		               "Usage: s_LoggingOptions [0ab...] (flags can be combined)\n"
		               "Default: abc\n"
		               "0: Logging disabled\n"
		               "a: Errors\n"
		               "b: Warnings\n"
		               "c: Comments\n");
#endif // ENABLE_AUDIO_LOGGING

		g_cvars.RegisterVariables();

		if (g_cvars.m_audioObjectPoolSize < 1)
		{
			g_cvars.m_audioObjectPoolSize = 1;
			Cry::Audio::Log(ELogType::Warning, R"(Audio Object pool size must be at least 1. Forcing the cvar "s_AudioObjectPoolSize" to 1!)");
		}

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Object Pool");
		CATLAudioObject::CreateAllocator(g_cvars.m_audioObjectPoolSize);

		if (g_cvars.m_audioEventPoolSize < 1)
		{
			g_cvars.m_audioEventPoolSize = 1;
			Cry::Audio::Log(ELogType::Warning, R"(Audio Event pool size must be at least 1. Forcing the cvar "s_AudioEventPoolSize" to 1!)");
		}

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Event Pool");
		CATLEvent::CreateAllocator(g_cvars.m_audioEventPoolSize);

		if (g_cvars.m_audioStandaloneFilePoolSize < 1)
		{
			g_cvars.m_audioStandaloneFilePoolSize = 1;
			Cry::Audio::Log(ELogType::Warning, R"(Audio Standalone File pool size must be at least 1. Forcing the cvar "s_AudioStandaloneFilePoolSize" to 1!)");
		}

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "Audio System Standalone File Pool");
		CATLStandaloneFile::CreateAllocator(g_cvars.m_audioStandaloneFilePoolSize);

		// Add the callback for the obstruction calculation.
		gEnv->pPhysicalWorld->AddEventClient(
			EventPhysRWIResult::id,
			&CPropagationProcessor::OnObstructionTest,
			1);

		m_objectPoolSize = static_cast<uint32>(g_cvars.m_audioObjectPoolSize);
		m_eventPoolSize = static_cast<uint32>(g_cvars.m_audioEventPoolSize);

		g_objectManager.Initialize(m_objectPoolSize);
		g_eventManager.Initialize(m_eventPoolSize);
		g_fileCacheManager.Initialize();

		CRY_ASSERT_MESSAGE(!m_mainThread.IsActive(), "AudioSystem thread active before initialization!");
		m_mainThread.Activate();
		AddRequestListener(&CSystem::OnCallback, nullptr, ESystemEvents::TriggerExecuted | ESystemEvents::TriggerFinished);
		m_isInitialized = true;
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "AudioSystem has already been initialized!");
	}

	return m_isInitialized;
}

///////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
	if (m_isInitialized)
	{
		RemoveRequestListener(&CSystem::OnCallback, nullptr);

		SManagerRequestData<EManagerRequestType::ReleaseAudioImpl> const releaseImplData;
		CAudioRequest releaseImplRequest(&releaseImplData);
		releaseImplRequest.flags = ERequestFlags::ExecuteBlocking;
		PushRequest(releaseImplRequest);

		m_mainThread.Deactivate();

		if (gEnv->pPhysicalWorld != nullptr)
		{
			// remove the callback for the obstruction calculation
			gEnv->pPhysicalWorld->RemoveEventClient(
				EventPhysRWIResult::id,
				&CPropagationProcessor::OnObstructionTest,
				1);
		}

		g_objectManager.Terminate();
		g_listenerManager.Terminate();
		g_cvars.UnregisterVariables();

		CATLAudioObject::FreeMemoryPool();
		CATLEvent::FreeMemoryPool();
		CATLStandaloneFile::FreeMemoryPool();

#if defined(ENABLE_AUDIO_LOGGING)
		IConsole* const pIConsole = gEnv->pConsole;

		if (pIConsole != nullptr)
		{
			pIConsole->UnregisterVariable("s_LoggingOptions");
		}
#endif // ENABLE_AUDIO_LOGGING

		m_isInitialized = false;
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "AudioSystem has already been released or was never properly initialized!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetImpl(Impl::IImpl* const pIImpl, SRequestUserData const& userData /*= SAudioRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::SetAudioImpl> const requestData(pIImpl);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::LoadTrigger> const requestData(triggerId);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::UnloadTrigger> const requestData(triggerId);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteTriggerEx(SExecuteTriggerData const& triggerData, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::ExecuteTriggerEx> const requestData(triggerData);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteDefaultTrigger(EDefaultTriggerType const type, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::ExecuteDefaultTrigger> const requestData(type);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::ExecuteTrigger> const requestData(triggerId);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopTrigger(ControlId const triggerId /* = CryAudio::InvalidControlId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (triggerId != InvalidControlId)
	{
		SObjectRequestData<EObjectRequestType::StopTrigger> const requestData(triggerId);
		CAudioRequest request(&requestData);
		request.flags = userData.flags;
		request.pOwner = userData.pOwner;
		request.pUserData = userData.pUserData;
		request.pUserDataOwner = userData.pUserDataOwner;
		PushRequest(request);
	}
	else
	{
		SObjectRequestData<EObjectRequestType::StopAllTriggers> const requestData;
		CAudioRequest request(&requestData);
		request.flags = userData.flags;
		request.pOwner = userData.pOwner;
		request.pUserData = userData.pUserData;
		request.pUserDataOwner = userData.pUserDataOwner;
		PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecutePreviewTrigger(Impl::ITriggerInfo const& triggerInfo)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SManagerRequestData<EManagerRequestType::ExecutePreviewTrigger> const requestData(triggerInfo);
	CAudioRequest const request(&requestData);
	PushRequest(request);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopPreviewTrigger()
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SManagerRequestData<EManagerRequestType::StopPreviewTrigger> const requestData;
	CAudioRequest const request(&requestData);
	PushRequest(request);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::SetParameter> const requestData(parameterId, value);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetSwitchState(ControlId const switchId, SwitchStateId const switchStateId, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SObjectRequestData<EObjectRequestType::SetSwitchState> const requestData(switchId, switchStateId);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SObjectRequestData<EObjectRequestType::PlayFile> const requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = (userData.pOwner != nullptr) ? userData.pOwner : &g_system;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopFile(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SObjectRequestData<EObjectRequestType::StopFile> const requestData(szName);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportStartedFile(
	CATLStandaloneFile& standaloneFile,
	bool const bSuccessfullyStarted,
	SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const requestData(standaloneFile, bSuccessfullyStarted);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportStoppedFile(CATLStandaloneFile& standaloneFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const requestData(standaloneFile);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportFinishedEvent(
	CATLEvent& event,
	bool const bSuccess,
	SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedEvent> const requestData(event, bSuccess);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportVirtualizedEvent(CATLEvent& event, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportVirtualizedEvent> const requestData(event);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportPhysicalizedEvent(CATLEvent& event, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SCallbackManagerRequestData<ECallbackManagerRequestType::ReportPhysicalizedEvent> const requestData(event);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopAllSounds(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::StopAllSounds> const requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Refresh(char const* const szLevelName, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::RefreshAudioSystem> const requestData(szLevelName);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ParseControlsData(
	char const* const szFolderPath,
	EDataScope const dataScope,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::ParseControlsData> const requestData(szFolderPath, dataScope);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ParsePreloadsData(
	char const* const szFolderPath,
	EDataScope const dataScope,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::ParsePreloadsData> const requestData(szFolderPath, dataScope);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PreloadSingleRequest(PreloadRequestId const id, bool const bAutoLoadOnly, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::PreloadSingleRequest> const requestData(id, bAutoLoadOnly);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadSingleRequest(PreloadRequestId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::UnloadSingleRequest> const requestData(id);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::AutoLoadSetting(EDataScope const dataScope, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::AutoLoadSetting> const requestData(dataScope);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadSetting(ControlId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::LoadSetting> const requestData(id);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadSetting(ControlId const id, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SManagerRequestData<EManagerRequestType::UnloadSetting> const requestData(id);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RetriggerAudioControls(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::RetriggerAudioControls> const requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReloadControlsData(
	char const* const szFolderPath,
	char const* const szLevelName,
	SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SManagerRequestData<EManagerRequestType::ReloadControlsData> const requestData(szFolderPath, szLevelName);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

///////////////////////////////////////////////////////////////////////////
char const* CSystem::GetConfigPath() const
{
	return g_configPath.c_str();
}

///////////////////////////////////////////////////////////////////////////
CryAudio::IListener* CSystem::CreateListener(CObjectTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	CATLListener* pListener = nullptr;
	SManagerRequestData<EManagerRequestType::RegisterListener> const requestData(&pListener, transformation, szName);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request);

	return static_cast<CryAudio::IListener*>(pListener);
}

///////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseListener(CryAudio::IListener* const pIListener)
{
	SManagerRequestData<EManagerRequestType::ReleaseListener> const requestData(static_cast<CATLListener*>(pIListener));
	CAudioRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
CryAudio::IObject* CSystem::CreateObject(SCreateObjectData const& objectData /*= SCreateObjectData::GetEmptyObject()*/)
{
	CATLAudioObject* pObject = nullptr;
	SManagerRequestData<EManagerRequestType::RegisterObject> const requestData(&pObject, objectData);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request);
	return static_cast<CryAudio::IObject*>(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseObject(CryAudio::IObject* const pIObject)
{
	SManagerRequestData<EManagerRequestType::ReleaseObject> const requestData(static_cast<CATLAudioObject*>(pIObject));
	CAudioRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetFileData(char const* const szName, SFileData& fileData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SManagerRequestData<EManagerRequestType::GetAudioFileData> const requestData(szName, fileData);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetTriggerData(ControlId const triggerId, STriggerData& triggerData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTrigger const* const pTrigger = stl::find_in_map(g_triggers, triggerId, nullptr);

	if (pTrigger != nullptr)
	{
		triggerData.radius = pTrigger->GetRadius();
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseImpl()
{
	// Reject new requests during shutdown.
	g_systemStates |= ESystemStates::ImplShuttingDown;

	// Release middleware specific data before its shutdown.
	g_fileManager.ReleaseImplData();
	g_listenerManager.ReleaseImplData();
	g_eventManager.ReleaseImplData();
	g_objectManager.ReleaseImplData();

	g_pIImpl->DestructObject(g_pObject->GetImplDataPtr());
	g_pObject->SetImplDataPtr(nullptr);

	delete g_pObject;
	g_pObject = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	g_pIImpl->DestructObject(g_previewObject.GetImplDataPtr());
	g_previewObject.Release();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	g_xmlProcessor.ClearPreloadsData(EDataScope::All);
	g_xmlProcessor.ClearControlsData(EDataScope::All);

	g_pIImpl->ShutDown();
	g_pIImpl->Release();
	g_pIImpl = nullptr;

	// Release engine specific data after impl shut down to prevent dangling data accesses during shutdown.
	// Note: The object and listener managers are an exception as we need their data to survive in case the middleware is swapped out.
	g_eventManager.Release();
	g_fileManager.Release();

	FreeMemoryPools();

	g_systemStates &= ~ESystemStates::ImplShuttingDown;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageChanged()
{
	SManagerRequestData<EManagerRequestType::ChangeLanguage> const requestData;
	CAudioRequest const request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Log(ELogType const type, char const* const szFormat, ...)
{
#if defined(ENABLE_AUDIO_LOGGING)
	if (szFormat != nullptr && szFormat[0] != '\0' && gEnv->pLog->GetVerbosityLevel() != -1)
	{
		CRY_PROFILE_REGION(PROFILE_AUDIO, "CSystem::Log");

		char buffer[256];
		va_list ArgList;
		va_start(ArgList, szFormat);
		cry_vsprintf(buffer, szFormat, ArgList);
		va_end(ArgList);

		ELoggingOptions const loggingOptions = static_cast<ELoggingOptions>(m_loggingOptions);

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
#endif // ENABLE_AUDIO_LOGGING
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ProcessRequests(AudioRequests& requestQueue)
{
	while (requestQueue.dequeue(m_request))
	{
		if (m_request.status == ERequestStatus::None)
		{
			m_request.status = ERequestStatus::Pending;
			ProcessRequest(m_request);
		}
		else
		{
			// TODO: handle pending requests!
		}

		if (m_request.status != ERequestStatus::Pending)
		{
			if ((m_request.flags & ERequestFlags::CallbackOnAudioThread) != 0)
			{
				NotifyListener(m_request);

				if ((m_request.flags & ERequestFlags::ExecuteBlocking) != 0)
				{
					m_mainEvent.Set();
				}
			}
			else if ((m_request.flags & ERequestFlags::CallbackOnExternalOrCallingThread) != 0)
			{
				if ((m_request.flags & ERequestFlags::ExecuteBlocking) != 0)
				{
					m_syncRequest = m_request;
					m_mainEvent.Set();
				}
				else
				{
					if (m_request.pObject == nullptr)
					{
						g_pObject->IncrementSyncCallbackCounter();
					}
					else
					{
						m_request.pObject->IncrementSyncCallbackCounter();
					}
					m_syncCallbacks.enqueue(m_request);
				}
			}
			else if ((m_request.flags & ERequestFlags::ExecuteBlocking) != 0)
			{
				m_mainEvent.Set();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ProcessRequest(CAudioRequest& request)
{
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
		case ERequestType::CallbackManagerRequest:
			{
				result = ProcessCallbackManagerRequest(request);

				break;
			}
		case ERequestType::ManagerRequest:
			{
				result = ProcessManagerRequest(request);

				break;
			}
		default:
			{
				CRY_ASSERT_MESSAGE(false, "Unknown request type: %u", request.GetData()->requestType);

				break;
			}
		}
	}

	request.status = result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessManagerRequest(CAudioRequest const& request)
{
	ERequestStatus result = ERequestStatus::Failure;
	SManagerRequestDataBase const* const pBase = static_cast<SManagerRequestDataBase const*>(request.GetData());

	switch (pBase->managerRequestType)
	{
	case EManagerRequestType::AddRequestListener:
		{
			SManagerRequestData<EManagerRequestType::AddRequestListener> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::AddRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.AddRequestListener(pRequestData);

			break;
		}
	case EManagerRequestType::RemoveRequestListener:
		{
			SManagerRequestData<EManagerRequestType::RemoveRequestListener> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::RemoveRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.RemoveRequestListener(pRequestData->func, pRequestData->pObjectToListenTo);

			break;
		}
	case EManagerRequestType::SetAudioImpl:
		{
			SManagerRequestData<EManagerRequestType::SetAudioImpl> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::SetAudioImpl> const*>(request.GetData());
			result = HandleSetImpl(pRequestData->pIImpl);

			break;
		}
	case EManagerRequestType::RefreshAudioSystem:
		{
			SManagerRequestData<EManagerRequestType::RefreshAudioSystem> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::RefreshAudioSystem> const*>(request.GetData());
			result = HandleRefresh(pRequestData->levelName);

			break;
		}
	case EManagerRequestType::ExecuteTriggerEx:
		{
			SManagerRequestData<EManagerRequestType::ExecuteTriggerEx> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::ExecuteTriggerEx> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				auto const pNewObject = new CATLAudioObject(pRequestData->transformation);
				g_objectManager.RegisterObject(pNewObject);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pNewObject->Init(pRequestData->name.c_str(), g_pIImpl->ConstructObject(pRequestData->transformation, pRequestData->name.c_str()), pRequestData->entityId);
#else
				pNewObject->Init(nullptr, g_pIImpl->ConstructObject(pRequestData->transformation, nullptr), pRequestData->entityId);
#endif      // INCLUDE_AUDIO_PRODUCTION_CODE

				if (pRequestData->setCurrentEnvironments)
				{
					SetCurrentEnvironmentsOnObject(pNewObject, INVALID_ENTITYID);
				}

				SetOcclusionType(*pNewObject, pRequestData->occlusionType);
				pTrigger->Execute(*pNewObject, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				pNewObject->RemoveFlag(EObjectFlags::InUse);
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EManagerRequestType::ExecuteDefaultTrigger:
		{
			SManagerRequestData<EManagerRequestType::ExecuteDefaultTrigger> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::ExecuteDefaultTrigger> const*>(request.GetData());

			switch (pRequestData->triggerType)
			{
			case EDefaultTriggerType::LoseFocus:
				{
					g_loseFocusTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::GetFocus:
				{
					g_getFocusTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::MuteAll:
				{
					g_muteAllTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::UnmuteAll:
				{
					g_unmuteAllTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::PauseAll:
				{
					g_pauseAllTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			case EDefaultTriggerType::ResumeAll:
				{
					g_resumeAllTrigger.Execute();
					result = ERequestStatus::Success;

					break;
				}
			default:
				break;
			}

			break;
		}
	case EManagerRequestType::StopAllSounds:
		{
			result = g_pIImpl->StopAllSounds();

			break;
		}
	case EManagerRequestType::ParseControlsData:
		{
			SManagerRequestData<EManagerRequestType::ParseControlsData> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::ParseControlsData> const*>(request.GetData());
			g_xmlProcessor.ParseControlsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ParsePreloadsData:
		{
			SManagerRequestData<EManagerRequestType::ParsePreloadsData> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::ParsePreloadsData> const*>(request.GetData());
			g_xmlProcessor.ParsePreloadsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ClearControlsData:
		{
			SManagerRequestData<EManagerRequestType::ClearControlsData> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::ClearControlsData> const*>(request.GetData());
			g_xmlProcessor.ClearControlsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ClearPreloadsData:
		{
			SManagerRequestData<EManagerRequestType::ClearPreloadsData> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::ClearPreloadsData> const*>(request.GetData());
			g_xmlProcessor.ClearPreloadsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::PreloadSingleRequest:
		{
			SManagerRequestData<EManagerRequestType::PreloadSingleRequest> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::PreloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryLoadRequest(pRequestData->audioPreloadRequestId, ((request.flags & ERequestFlags::ExecuteBlocking) != 0), pRequestData->bAutoLoadOnly);

			break;
		}
	case EManagerRequestType::UnloadSingleRequest:
		{
			SManagerRequestData<EManagerRequestType::UnloadSingleRequest> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::UnloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryUnloadRequest(pRequestData->audioPreloadRequestId);

			break;
		}
	case EManagerRequestType::AutoLoadSetting:
		{
			SManagerRequestData<EManagerRequestType::AutoLoadSetting> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::AutoLoadSetting> const*>(request.GetData());

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
	case EManagerRequestType::LoadSetting:
		{
			SManagerRequestData<EManagerRequestType::LoadSetting> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::LoadSetting> const*>(request.GetData());

			CSetting const* const pSetting = stl::find_in_map(g_settings, pRequestData->id, nullptr);

			if (pSetting != nullptr)
			{
				pSetting->Load();
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EManagerRequestType::UnloadSetting:
		{
			SManagerRequestData<EManagerRequestType::UnloadSetting> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::UnloadSetting> const*>(request.GetData());

			CSetting const* const pSetting = stl::find_in_map(g_settings, pRequestData->id, nullptr);

			if (pSetting != nullptr)
			{
				pSetting->Unload();
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EManagerRequestType::UnloadAFCMDataByScope:
		{
			SManagerRequestData<EManagerRequestType::UnloadAFCMDataByScope> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::UnloadAFCMDataByScope> const*>(request.GetData());
			result = g_fileCacheManager.UnloadDataByScope(pRequestData->dataScope);

			break;
		}
	case EManagerRequestType::ReleaseAudioImpl:
		{
			ReleaseImpl();
			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ChangeLanguage:
		{
			SetImplLanguage();

			g_fileCacheManager.UpdateLocalizedFileCacheEntries();
			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ExecutePreviewTrigger:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			SManagerRequestData<EManagerRequestType::ExecutePreviewTrigger> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::ExecutePreviewTrigger> const*>(request.GetData());

			g_previewTrigger.Execute(pRequestData->triggerInfo);
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EManagerRequestType::StopPreviewTrigger:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			g_previewTrigger.Stop();
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EManagerRequestType::RetriggerAudioControls:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			HandleRetriggerControls();
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EManagerRequestType::ReleasePendingRays:
		{
			g_objectManager.ReleasePendingRays();
			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ReloadControlsData:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			SManagerRequestData<EManagerRequestType::ReloadControlsData> const* const pRequestData = static_cast<SManagerRequestData<EManagerRequestType::ReloadControlsData> const*>(request.GetData());

			for (auto const pObject : g_objectManager.GetObjects())
			{
				for (auto const pEvent : pObject->GetActiveEvents())
				{
					CRY_ASSERT_MESSAGE((pEvent != nullptr) && (pEvent->IsPlaying() || pEvent->IsVirtual()), "Invalid event during EAudioManagerRequestType::ReloadControlsData");
					pEvent->Stop();
				}
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
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
			break;
		}
	case EManagerRequestType::DrawDebugInfo:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			HandleDrawDebug();
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EManagerRequestType::GetAudioFileData:
		{
			SManagerRequestData<EManagerRequestType::GetAudioFileData> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::GetAudioFileData> const* const>(request.GetData());
			g_pIImpl->GetFileData(pRequestData->name.c_str(), pRequestData->fileData);
			break;
		}
	case EManagerRequestType::GetImplInfo:
		{
			SManagerRequestData<EManagerRequestType::GetImplInfo> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::GetImplInfo> const* const>(request.GetData());
			g_pIImpl->GetInfo(pRequestData->implInfo);
			break;
		}
	case EManagerRequestType::RegisterListener:
		{
			SManagerRequestData<EManagerRequestType::RegisterListener> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::RegisterListener> const*>(request.GetData());
			*pRequestData->ppListener = g_listenerManager.CreateListener(pRequestData->transformation, pRequestData->name.c_str());
			break;
		}
	case EManagerRequestType::ReleaseListener:
		{
			SManagerRequestData<EManagerRequestType::ReleaseListener> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::ReleaseListener> const* const>(request.GetData());

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				g_listenerManager.ReleaseListener(pRequestData->pListener);
				result = ERequestStatus::Success;
			}

			break;
		}
	case EManagerRequestType::RegisterObject:
		{
			SManagerRequestData<EManagerRequestType::RegisterObject> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::RegisterObject> const*>(request.GetData());

			auto const pNewObject = new CATLAudioObject(pRequestData->transformation);
			g_objectManager.RegisterObject(pNewObject);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pNewObject->Init(pRequestData->name.c_str(), g_pIImpl->ConstructObject(pRequestData->transformation, pRequestData->name.c_str()), pRequestData->entityId);
#else
			pNewObject->Init(nullptr, g_pIImpl->ConstructObject(pRequestData->transformation, nullptr), pRequestData->entityId);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE

			if (pRequestData->setCurrentEnvironments)
			{
				SetCurrentEnvironmentsOnObject(pNewObject, INVALID_ENTITYID);
			}

			SetOcclusionType(*pNewObject, pRequestData->occlusionType);
			*pRequestData->ppObject = pNewObject;
			result = ERequestStatus::Success;

			break;
		}
	case EManagerRequestType::ReleaseObject:
		{
			SManagerRequestData<EManagerRequestType::ReleaseObject> const* const pRequestData =
				static_cast<SManagerRequestData<EManagerRequestType::ReleaseObject> const* const>(request.GetData());

			CRY_ASSERT(pRequestData->pObject != nullptr);

			if (pRequestData->pObject != nullptr)
			{
				if (pRequestData->pObject != g_pObject)
				{
					pRequestData->pObject->RemoveFlag(EObjectFlags::InUse);
					result = ERequestStatus::Success;
				}
				else
				{
					Cry::Audio::Log(ELogType::Warning, "ATL received a request to release the GlobalAudioObject");
				}
			}

			break;
		}
	case EManagerRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown manager request type: %u", pBase->managerRequestType);
			result = ERequestStatus::FailureInvalidRequest;

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessCallbackManagerRequest(CAudioRequest& request)
{
	ERequestStatus result = ERequestStatus::Failure;
	SCallbackManagerRequestDataBase const* const pBase =
		static_cast<SCallbackManagerRequestDataBase const* const>(request.GetData());

	switch (pBase->callbackManagerRequestType)
	{
	case ECallbackManagerRequestType::ReportStartedEvent:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> const* const>(request.GetData());
			CATLEvent& audioEvent = pRequestData->audioEvent;

			audioEvent.m_state = EEventState::PlayingDelayed;

			if (audioEvent.m_pAudioObject != g_pObject)
			{
				g_objectManager.ReportStartedEvent(&audioEvent);
			}
			else
			{
				g_pObject->ReportStartedEvent(&audioEvent);
			}

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackManagerRequestType::ReportFinishedEvent:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedEvent> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedEvent> const* const>(request.GetData());
			CATLEvent& event = pRequestData->audioEvent;

			if (event.m_pAudioObject != g_pObject)
			{
				g_objectManager.ReportFinishedEvent(&event, pRequestData->bSuccess);
			}
			else
			{
				g_pObject->ReportFinishedEvent(&event, pRequestData->bSuccess);
			}

			g_eventManager.DestructEvent(&event);

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackManagerRequestType::ReportVirtualizedEvent:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportVirtualizedEvent> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportVirtualizedEvent> const* const>(request.GetData());

			pRequestData->audioEvent.m_state = EEventState::Virtual;
			CATLAudioObject* const pObject = pRequestData->audioEvent.m_pAudioObject;

			if ((pObject->GetFlags() & EObjectFlags::Virtual) == 0)
			{
				bool isVirtual = true;

				for (auto const pEvent : pObject->GetActiveEvents())
				{
					if (pEvent->m_state != EEventState::Virtual)
					{
						isVirtual = false;
						break;
					}
				}

				if (isVirtual)
				{
					pObject->SetFlag(EObjectFlags::Virtual);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
					pObject->ResetObstructionRays();
#endif      // INCLUDE_AUDIO_PRODUCTION_CODE
				}
			}

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackManagerRequestType::ReportPhysicalizedEvent:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportPhysicalizedEvent> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportPhysicalizedEvent> const* const>(request.GetData());

			pRequestData->audioEvent.m_state = EEventState::Playing;
			pRequestData->audioEvent.m_pAudioObject->RemoveFlag(EObjectFlags::Virtual);

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackManagerRequestType::ReportStartedFile:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const* const>(request.GetData());

			CATLStandaloneFile& audioStandaloneFile = pRequestData->audioStandaloneFile;

			g_objectManager.GetStartedStandaloneFileRequestData(&audioStandaloneFile, request);
			audioStandaloneFile.m_state = (pRequestData->bSuccess) ? EStandaloneFileState::Playing : EStandaloneFileState::None;

			result = (pRequestData->bSuccess) ? ERequestStatus::Success : ERequestStatus::Failure;

			break;
		}
	case ECallbackManagerRequestType::ReportStoppedFile:
		{
			SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const* const pRequestData =
				static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const* const>(request.GetData());

			CATLStandaloneFile& audioStandaloneFile = pRequestData->audioStandaloneFile;

			g_objectManager.GetStartedStandaloneFileRequestData(&audioStandaloneFile, request);

			if (audioStandaloneFile.m_pAudioObject != g_pObject)
			{
				g_objectManager.ReportFinishedStandaloneFile(&audioStandaloneFile);
			}
			else
			{
				g_pObject->ReportFinishedStandaloneFile(&audioStandaloneFile);
			}

			g_fileManager.ReleaseStandaloneFile(&audioStandaloneFile);

			result = ERequestStatus::Success;

			break;
		}
	case ECallbackManagerRequestType::ReportFinishedTriggerInstance:
	case ECallbackManagerRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
	default:
		{
			result = ERequestStatus::FailureInvalidRequest;
			Cry::Audio::Log(ELogType::Warning, "Unknown callback manager request type: %u", pBase->callbackManagerRequestType);

			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessObjectRequest(CAudioRequest const& request)
{
	ERequestStatus result = ERequestStatus::Failure;
	CATLAudioObject* const pObject = (request.pObject != nullptr) ? request.pObject : g_pObject;

	SObjectRequestDataBase const* const pBase =
		static_cast<SObjectRequestDataBase const* const>(request.GetData());

	switch (pBase->objectRequestType)
	{
	case EObjectRequestType::LoadTrigger:
		{
			SObjectRequestData<EObjectRequestType::LoadTrigger> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::LoadTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->LoadAsync(*pObject, true);
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EObjectRequestType::UnloadTrigger:
		{
			SObjectRequestData<EObjectRequestType::UnloadTrigger> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::UnloadTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->LoadAsync(*pObject, false);
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EObjectRequestType::PlayFile:
		{
			SObjectRequestData<EObjectRequestType::PlayFile> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::PlayFile> const* const>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				if (pRequestData->usedAudioTriggerId != InvalidControlId)
				{
					CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->usedAudioTriggerId, nullptr);

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
			SObjectRequestData<EObjectRequestType::StopFile> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::StopFile> const* const>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				pObject->HandleStopFile(pRequestData->file.c_str());
				result = ERequestStatus::Success;
			}

			break;
		}
	case EObjectRequestType::ExecuteTrigger:
		{
			SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				pTrigger->Execute(*pObject, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EObjectRequestType::StopTrigger:
		{
			SObjectRequestData<EObjectRequestType::StopTrigger> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::StopTrigger> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->HandleStopTrigger(pTrigger);
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
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
			CRY_ASSERT_MESSAGE(pObject != g_pObject, "Received a request to set a transformation on the global object.");

			SObjectRequestData<EObjectRequestType::SetTransformation> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetTransformation> const* const>(request.GetData());

			pObject->HandleSetTransformation(pRequestData->transformation);
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetParameter:
		{
			SObjectRequestData<EObjectRequestType::SetParameter> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetParameter> const* const>(request.GetData());

			CParameter const* const pParameter = stl::find_in_map(g_parameters, pRequestData->parameterId, nullptr);

			if (pParameter != nullptr)
			{
				pParameter->Set(*pObject, pRequestData->value);
				result = ERequestStatus::Success;
			}
			else
			{
				result = ERequestStatus::FailureInvalidControlId;
			}

			break;
		}
	case EObjectRequestType::SetSwitchState:
		{
			result = ERequestStatus::FailureInvalidControlId;
			SObjectRequestData<EObjectRequestType::SetSwitchState> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetSwitchState> const* const>(request.GetData());

			CATLSwitch const* const pSwitch = stl::find_in_map(g_switches, pRequestData->audioSwitchId, nullptr);

			if (pSwitch != nullptr)
			{
				CATLSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), pRequestData->audioSwitchStateId, nullptr);

				if (pState != nullptr)
				{
					pState->Set(*pObject);
					result = ERequestStatus::Success;
				}
			}

			break;
		}
	case EObjectRequestType::SetOcclusionType:
		{
			CRY_ASSERT_MESSAGE(pObject != g_pObject, "Received a request to set the occlusion type on the global object.");

			SObjectRequestData<EObjectRequestType::SetOcclusionType> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetOcclusionType> const* const>(request.GetData());

			SetOcclusionType(*pObject, pRequestData->occlusionType);
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetOcclusionRayOffset:
		{
			CRY_ASSERT_MESSAGE(pObject != g_pObject, "Received a request to set the occlusion ray offset on the global object.");

			SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> const* const>(request.GetData());

			pObject->HandleSetOcclusionRayOffset(pRequestData->occlusionRayOffset);
			result = ERequestStatus::Success;

			break;
		}
	case EObjectRequestType::SetCurrentEnvironments:
		{
			SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> const* const>(request.GetData());

			SetCurrentEnvironmentsOnObject(pObject, pRequestData->entityToIgnore);

			break;
		}
	case EObjectRequestType::SetEnvironment:
		{
			if (pObject != g_pObject)
			{
				SObjectRequestData<EObjectRequestType::SetEnvironment> const* const pRequestData =
					static_cast<SObjectRequestData<EObjectRequestType::SetEnvironment> const* const>(request.GetData());

				CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(g_environments, pRequestData->audioEnvironmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					pObject->HandleSetEnvironment(pEnvironment, pRequestData->amount);
					result = ERequestStatus::Success;
				}
				else
				{
					result = ERequestStatus::FailureInvalidControlId;
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Warning, "ATL received a request to set an environment on a global object");
			}

			break;
		}
	case EObjectRequestType::ProcessPhysicsRay:
		{
			SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const* const>(request.GetData());

			pObject->ProcessPhysicsRay(pRequestData->pAudioRayInfo);
			result = ERequestStatus::Success;
			break;
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	case EObjectRequestType::SetName:
		{
			SObjectRequestData<EObjectRequestType::SetName> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::SetName> const* const>(request.GetData());

			result = pObject->HandleSetName(pRequestData->name.c_str());

			if (result == ERequestStatus::SuccessNeedsRefresh)
			{
				pObject->ForceImplementationRefresh(true);
				result = ERequestStatus::Success;
			}

			break;
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	case EObjectRequestType::ToggleAbsoluteVelocityTracking:
		{
			SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> const* const>(request.GetData());

			if (pRequestData->isEnabled)
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(Impl::EObjectFunctionality::TrackAbsoluteVelocity, true);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pObject->SetFlag(EObjectFlags::TrackAbsoluteVelocity);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
			}
			else
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(Impl::EObjectFunctionality::TrackAbsoluteVelocity, false);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pObject->RemoveFlag(EObjectFlags::TrackAbsoluteVelocity);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
			}

			result = ERequestStatus::Success;
			break;
		}
	case EObjectRequestType::ToggleRelativeVelocityTracking:
		{
			SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> const* const pRequestData =
				static_cast<SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> const* const>(request.GetData());

			if (pRequestData->isEnabled)
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(Impl::EObjectFunctionality::TrackRelativeVelocity, true);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pObject->SetFlag(EObjectFlags::TrackRelativeVelocity);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
			}
			else
			{
				pObject->GetImplDataPtr()->ToggleFunctionality(Impl::EObjectFunctionality::TrackRelativeVelocity, false);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pObject->RemoveFlag(EObjectFlags::TrackRelativeVelocity);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
			}

			result = ERequestStatus::Success;
			break;
		}
	case EObjectRequestType::None:
		{
			result = ERequestStatus::Success;
			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown object request type: %u", pBase->objectRequestType);
			result = ERequestStatus::FailureInvalidRequest;
			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessListenerRequest(SRequestData const* const pPassedRequestData)
{
	ERequestStatus result = ERequestStatus::Failure;
	SListenerRequestDataBase const* const pBase =
		static_cast<SListenerRequestDataBase const* const>(pPassedRequestData);

	switch (pBase->listenerRequestType)
	{
	case EListenerRequestType::SetTransformation:
		{
			SListenerRequestData<EListenerRequestType::SetTransformation> const* const pRequestData =
				static_cast<SListenerRequestData<EListenerRequestType::SetTransformation> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				pRequestData->pListener->HandleSetTransformation(pRequestData->transformation);
			}

			result = ERequestStatus::Success;
		}
		break;
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	case EListenerRequestType::SetName:
		{
			SListenerRequestData<EListenerRequestType::SetName> const* const pRequestData =
				static_cast<SListenerRequestData<EListenerRequestType::SetName> const* const>(pPassedRequestData);

			pRequestData->pListener->HandleSetName(pRequestData->name.c_str());
			result = ERequestStatus::Success;
		}
		break;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	case EListenerRequestType::None:
		result = ERequestStatus::Success;
		break;
	default:
		result = ERequestStatus::FailureInvalidRequest;
		Cry::Audio::Log(ELogType::Warning, "Unknown listener request type: %u", pBase->listenerRequestType);
		break;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::NotifyListener(CAudioRequest const& request)
{
	ESystemEvents audioSystemEvent = ESystemEvents::None;
	CATLStandaloneFile* pStandaloneFile = nullptr;
	ControlId audioControlID = InvalidControlId;
	CATLEvent* pAudioEvent = nullptr;

	switch (request.GetData()->requestType)
	{
	case ERequestType::ManagerRequest:
		{
			SManagerRequestDataBase const* const pBase = static_cast<SManagerRequestDataBase const* const>(request.GetData());

			switch (pBase->managerRequestType)
			{
			case EManagerRequestType::SetAudioImpl:
				audioSystemEvent = ESystemEvents::ImplSet;
				break;
			}

			break;
		}
	case ERequestType::CallbackManagerRequest:
		{
			SCallbackManagerRequestDataBase const* const pBase = static_cast<SCallbackManagerRequestDataBase const* const>(request.GetData());

			switch (pBase->callbackManagerRequestType)
			{
			case ECallbackManagerRequestType::ReportFinishedTriggerInstance:
				{
					SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedTriggerInstance> const* const pRequestData = static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedTriggerInstance> const* const>(pBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = ESystemEvents::TriggerFinished;

					break;
				}
			case ECallbackManagerRequestType::ReportStartedEvent:
				{
					SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> const* const pRequestData = static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> const* const>(pBase);
					pAudioEvent = &pRequestData->audioEvent;

					break;
				}
			case ECallbackManagerRequestType::ReportStartedFile:
				{
					SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const* const pRequestData = static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const* const>(pBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = ESystemEvents::FileStarted;

					break;
				}
			case ECallbackManagerRequestType::ReportStoppedFile:
				{
					SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const* const pRequestData = static_cast<SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const* const>(pBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = ESystemEvents::FileStopped;

					break;
				}
			}

			break;
		}
	case ERequestType::ObjectRequest:
		{
			SObjectRequestDataBase const* const pBase = static_cast<SObjectRequestDataBase const* const>(request.GetData());

			switch (pBase->objectRequestType)
			{
			case EObjectRequestType::ExecuteTrigger:
				{
					SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const pRequestData = static_cast<SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const>(pBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = ESystemEvents::TriggerExecuted;

					break;
				}
			case EObjectRequestType::PlayFile:
				audioSystemEvent = ESystemEvents::FilePlay;
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
			CryFatalError("Unknown request type during CAudioTranslationLayer::NotifyListener!");

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
	case ERequestStatus::FailureInvalidControlId:
	case ERequestStatus::FailureInvalidRequest:
	case ERequestStatus::PartialSuccess:
		{
			result = ERequestResult::Failure;
			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(false, "Invalid request status '%u'. Cannot be converted to a request result.", request.status);
			result = ERequestResult::Failure;
			break;
		}
	}

	SRequestInfo const requestInfo(
		result,
		request.pOwner,
		request.pUserData,
		request.pUserDataOwner,
		audioSystemEvent,
		audioControlID,
		static_cast<IObject*>(request.pObject),
		pStandaloneFile,
		pAudioEvent);

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
		Cry::Audio::Log(ELogType::Warning, "nullptr passed to SetImpl, will run with the null implementation");

		auto const pImpl = new Impl::Null::CImpl();
		CRY_ASSERT(pImpl != nullptr);
		g_pIImpl = static_cast<Impl::IImpl*>(pImpl);
	}

	g_xmlProcessor.ParseSystemData();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if ((g_systemStates& ESystemStates::PoolsAllocated) == 0)
	{
		// Don't allocate again after impl switch.
		AllocateMemoryPools();
		g_systemStates |= ESystemStates::PoolsAllocated;
	}
#else
	AllocateMemoryPools();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	result = g_pIImpl->Init(m_objectPoolSize, m_eventPoolSize);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Get impl info again (was done in ParseSystemData) to set the impl name, because
	// it's not guaranteed that it already existed in the impl constructor.
	g_pIImpl->GetInfo(g_implInfo);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	if (result != ERequestStatus::Success)
	{
		// The impl failed to initialize, allow it to shut down and release then fall back to the null impl.
		Cry::Audio::Log(ELogType::Error, "Failed to set the AudioImpl %s. Will run with the null implementation.", m_implInfo.name.c_str());

		// There's no need to call Shutdown when the initialization failed as
		// we expect the implementation to clean-up itself if it couldn't be initialized

		g_pIImpl->Release(); // Release the engine specific data.

		auto const pImpl = new Impl::Null::CImpl();
		CRY_ASSERT(pImpl != nullptr);
		g_pIImpl = static_cast<Impl::IImpl*>(pImpl);
	}

	if (g_pObject == nullptr)
	{
		g_pObject = new CATLAudioObject(CObjectTransformation::GetEmptyObject());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		g_pObject->m_name = "Global Object";
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	CRY_ASSERT_MESSAGE(g_pObject->GetImplDataPtr() == nullptr, "<Audio> The global object's impl-data must be nullptr during initialization.");
	g_pObject->SetImplDataPtr(g_pIImpl->ConstructGlobalObject());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	g_previewObject.m_name = "Preview Object";
	CRY_ASSERT_MESSAGE(g_previewObject.GetImplDataPtr() == nullptr, "<Audio> The preview object's impl-data must be nullptr during initialization.");
	g_previewObject.SetImplDataPtr(g_pIImpl->ConstructObject(g_previewObject.GetTransformation()));
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	g_objectManager.OnAfterImplChanged();
	g_eventManager.OnAfterImplChanged();
	g_listenerManager.OnAfterImplChanged();

	SetImplLanguage();

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::HandleRefresh(char const* const szLevelName)
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

	g_pIImpl->OnRefresh();

	g_xmlProcessor.ParseSystemData();
	g_xmlProcessor.ParseControlsData(g_configPath.c_str(), EDataScope::Global);
	g_xmlProcessor.ParsePreloadsData(g_configPath.c_str(), EDataScope::Global);

	// The global preload might not exist if no preloads have been created, for that reason we don't check the result of this call
	result = g_fileCacheManager.TryLoadRequest(GlobalPreloadRequestId, true, true);

	AutoLoadSetting(EDataScope::Global);

	if (szLevelName != nullptr && szLevelName[0] != '\0')
	{
		CryFixedStringT<MaxFilePathLength> levelPath = g_configPath;
		levelPath += s_szLevelsFolderName;
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

	return ERequestStatus::Success;
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
void CSystem::SetCurrentEnvironmentsOnObject(CATLAudioObject* const pObject, EntityId const entityToIgnore)
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
				CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(g_environments, areaInfo.audioEnvironmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					pObject->HandleSetEnvironment(pEnvironment, areaInfo.amount);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetOcclusionType(CATLAudioObject& object, EOcclusionType const occlusionType) const
{
	switch (occlusionType)
	{
	case EOcclusionType::Ignore:
		{
			object.HandleSetOcclusionType(EOcclusionType::Ignore);
			object.SetObstructionOcclusion(0.0f, 0.0f);

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
			Cry::Audio::Log(ELogType::Warning, "Unknown occlusion type during SetOcclusionType: %u", occlusionType);

			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnCallback(SRequestInfo const* const pRequestInfo)
{
	if (gEnv->mMainThreadId == CryGetCurrentThreadId() && pRequestInfo->pAudioObject != nullptr)
	{
		IEntity* const pIEntity = gEnv->pEntitySystem->GetEntity(pRequestInfo->pAudioObject->GetEntityId());

		if (pIEntity != nullptr)
		{
			SEntityEvent eventData;  //converting audio events to entityEvents
			eventData.nParam[0] = reinterpret_cast<intptr_t>(pRequestInfo);

			if (pRequestInfo->systemEvent == ESystemEvents::TriggerExecuted)
			{
				eventData.event = ENTITY_EVENT_AUDIO_TRIGGER_STARTED;
				pIEntity->SendEvent(eventData);
			}

			//if the trigger failed to start or has finished, we (also) send ENTITY_EVENT_AUDIO_TRIGGER_ENDED
			if (pRequestInfo->systemEvent == ESystemEvents::TriggerFinished
			    || (pRequestInfo->systemEvent == ESystemEvents::TriggerExecuted && pRequestInfo->requestResult != ERequestResult::Success))
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
	SManagerRequestData<EManagerRequestType::GetImplInfo> const requestData(implInfo);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request);
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
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
	if (g_cvars.m_drawAudioDebug > 0)
	{
		SubmitLastIRenderAuxGeomForRendering();

		SManagerRequestData<EManagerRequestType::DrawDebugInfo> const requestData;
		CAudioRequest const request(&requestData);
		PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void DrawMemoryPoolInfo(
	IRenderAuxGeom* const pAuxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType)
{
	CryFixedStringT<MaxMiscStringLength> memUsedString;

	if (mem.nUsed < 1024)
	{
		memUsedString.Format("%" PRISIZE_T " Byte", mem.nUsed);
	}
	else
	{
		memUsedString.Format("%" PRISIZE_T " KiB", mem.nUsed >> 10);
	}

	CryFixedStringT<MaxMiscStringLength> memAllocString;

	if (mem.nAlloc < 1024)
	{
		memAllocString.Format("%" PRISIZE_T " Byte", mem.nAlloc);
	}
	else
	{
		memAllocString.Format("%" PRISIZE_T " KiB", mem.nAlloc >> 10);
	}

	posY += Debug::g_systemLineHeight;
	pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false,
	                      "[%s] In Use: %" PRISIZE_T " | Constructed: %" PRISIZE_T " (%s) | Memory Pool: %s",
	                      szType, pool.nUsed, pool.nAlloc, memUsedString.c_str(), memAllocString.c_str());
}

//////////////////////////////////////////////////////////////////////////
void CSystem::HandleDrawDebug()
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);
	IRenderAuxGeom* const pAuxGeom = gEnv->pRenderer ? gEnv->pRenderer->GetOrCreateIRenderAuxGeom() : nullptr;

	if (pAuxGeom != nullptr)
	{
		if ((g_cvars.m_drawAudioDebug & Debug::objectMask) != 0)
		{
			// Needs to be called first so that the rest of the labels are printed on top.
			// (Draw2dLabel doesn't provide a way to set which labels are printed on top)
			g_objectManager.DrawPerObjectDebugInfo(*pAuxGeom);
		}

		float posX = 8.0f;
		float posY = 4.0f;

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::HideMemoryInfo) == 0)
		{
			CryModuleMemoryInfo memInfo;
			ZeroStruct(memInfo);
			CryGetMemoryInfoForModule(&memInfo);

			CryFixedStringT<MaxMiscStringLength> memInfoString;
			auto const memAlloc = static_cast<uint32>(memInfo.allocated - memInfo.freed);

			if (memAlloc < 1024)
			{
				memInfoString.Format("u Byte", memAlloc);
			}
			else
			{
				memInfoString.Format("%u KiB", memAlloc >> 10);
			}

			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemHeaderFontSize, Debug::g_globalColorHeader.data(), false,
			                      "Audio System (Total Memory: %s)", memInfoString.c_str());

			if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::DetailedMemoryInfo) != 0)
			{
				{
					auto& allocator = CATLAudioObject::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects");
				}

				{
					auto& allocator = CATLEvent::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events");
				}

				{
					auto& allocator = CATLStandaloneFile::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Files");
				}

				if (g_debugPoolSizes.triggers > 0)
				{
					auto& allocator = CTrigger::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers");
				}

				if (g_debugPoolSizes.parameters > 0)
				{
					auto& allocator = CParameter::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters");
				}

				if (g_debugPoolSizes.switches > 0)
				{
					auto& allocator = CATLSwitch::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Switches");
				}

				if (g_debugPoolSizes.states > 0)
				{
					auto& allocator = CATLSwitchState::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SwitchStates");
				}

				if (g_debugPoolSizes.environments > 0)
				{
					auto& allocator = CATLAudioEnvironment::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Environments");
				}

				if (g_debugPoolSizes.preloads > 0)
				{
					auto& allocator = CATLPreloadRequest::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Preloads");
				}

				if (g_debugPoolSizes.settings > 0)
				{
					auto& allocator = CSetting::GetAllocator();
					DrawMemoryPoolInfo(pAuxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Settings");
				}
			}

			size_t const numObjects = g_objectManager.GetNumAudioObjects();
			size_t const numActiveObjects = g_objectManager.GetNumActiveAudioObjects();
			size_t const numEvents = g_eventManager.GetNumConstructed();
			size_t const numListeners = g_listenerManager.GetNumActiveListeners();
			size_t const numEventListeners = g_eventListenerManager.GetNumEventListeners();
			static float const SMOOTHING_ALPHA = 0.2f;
			static float syncRays = 0;
			static float asyncRays = 0;
			syncRays += (CPropagationProcessor::s_totalSyncPhysRays - syncRays) * SMOOTHING_ALPHA;
			asyncRays += (CPropagationProcessor::s_totalAsyncPhysRays - asyncRays) * SMOOTHING_ALPHA * 0.1f;

			posY += Debug::g_systemLineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextSecondary.data(), false,
			                      "Objects: %3" PRISIZE_T "/%3" PRISIZE_T " | Events: %3" PRISIZE_T " | EventListeners %3" PRISIZE_T " | Listeners: %" PRISIZE_T " | SyncRays: %3.1f AsyncRays: %3.1f",
			                      numActiveObjects, numObjects, numEvents, numEventListeners, numListeners, syncRays, asyncRays);

			if (g_pIImpl != nullptr)
			{
				posY += Debug::g_systemHeaderLineHeight;
				g_pIImpl->DrawDebugInfo(*pAuxGeom, posX, posY);
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

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::Spheres) != 0)
		{
			debugDraw += "Spheres, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectLabel) != 0)
		{
			debugDraw += "Labels, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectTriggers) != 0)
		{
			debugDraw += "Triggers, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectStates) != 0)
		{
			debugDraw += "States, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectParameters) != 0)
		{
			debugDraw += "Parameters, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectEnvironments) != 0)
		{
			debugDraw += "Environments, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectDistance) != 0)
		{
			debugDraw += "Distances, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::OcclusionRayLabels) != 0)
		{
			debugDraw += "Occlusion Ray Labels, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::OcclusionRays) != 0)
		{
			debugDraw += "Occlusion Rays, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::OcclusionRayOffset) != 0)
		{
			debugDraw += "Occlusion Ray Offset, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ListenerOcclusionPlane) != 0)
		{
			debugDraw += "Listener Occlusion Plane, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectStandaloneFiles) != 0)
		{
			debugDraw += "Object Standalone Files, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ObjectImplInfo) != 0)
		{
			debugDraw += "Object Middleware Info, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::StandaloneFiles) != 0)
		{
			debugDraw += "Standalone Files, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ActiveEvents) != 0)
		{
			debugDraw += "Active Events, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ActiveObjects) != 0)
		{
			debugDraw += "Active Objects, ";
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::FileCacheManagerInfo) != 0)
		{
			debugDraw += "File Cache Manager, ";
		}

		if (!debugDraw.IsEmpty())
		{
			debugDraw.erase(debugDraw.length() - 2, 2);
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false, "Debug Draw: %s", debugDraw.c_str());
			posY += Debug::g_systemLineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false, "Debug Filter: %s | Debug Distance: %s", debugFilter.c_str(), debugDistance.c_str());

			posY += Debug::g_systemHeaderLineHeight;
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::FileCacheManagerInfo) != 0)
		{
			g_fileCacheManager.DrawDebugInfo(*pAuxGeom, posX, posY);
			posX += 600.0f;
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ActiveObjects) != 0)
		{
			g_objectManager.DrawDebugInfo(*pAuxGeom, posX, posY);
			posX += 300.0f;
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::ActiveEvents) != 0)
		{
			g_eventManager.DrawDebugInfo(*pAuxGeom, posX, posY);
			posX += 600.0f;
		}

		if ((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::StandaloneFiles) != 0)
		{
			g_fileManager.DrawDebugInfo(*pAuxGeom, posX, posY);
		}
	}

	g_system.ScheduleIRenderAuxGeomForRendering(pAuxGeom);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::HandleRetriggerControls()
{
	for (auto const pObject : g_objectManager.GetObjects())
	{
		pObject->ForceImplementationRefresh(true);
	}

	g_pObject->ForceImplementationRefresh(false);

	g_previewObject.ForceImplementationRefresh(false);

	if ((g_systemStates& ESystemStates::IsMuted) != 0)
	{
		ExecuteDefaultTrigger(EDefaultTriggerType::MuteAll);
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
