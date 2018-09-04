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
	SAudioManagerRequestData<EAudioManagerRequestType::AddRequestListener> requestData(pObjectToListenTo, func, eventMask);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RemoveRequestListener(void (*func)(SRequestInfo const* const), void* const pObjectToListenTo)
{
	SAudioManagerRequestData<EAudioManagerRequestType::RemoveRequestListener> requestData(pObjectToListenTo, func);
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

	if ((m_flags& EInternalStates::ImplShuttingDown) == 0)
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
			string const levelNameOnly = PathUtil::GetFileName(reinterpret_cast<const char*>(wparam));

			if (!levelNameOnly.empty() && levelNameOnly.compareNoCase("Untitled") != 0)
			{
				OnLoadLevel(levelNameOnly.c_str());
			}

			break;
		}
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			// This event is issued in Editor and Game mode.
			CPropagationProcessor::s_bCanIssueRWIs = false;

			SAudioManagerRequestData<EAudioManagerRequestType::ReleasePendingRays> requestData;
			CAudioRequest request(&requestData);
			request.flags = ERequestFlags::ExecuteBlocking;
			PushRequest(request);

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
					g_loseFocusTrigger.Execute();
				}
				else
				{
					// got focus
					g_getFocusTrigger.Execute();
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
					g_loseFocusTrigger.Execute();
				}
				else
				{
					// got focus
					g_getFocusTrigger.Execute();
				}
			}

			break;
		}
	case ESYSTEM_EVENT_AUDIO_MUTE:
		{
			g_muteAllTrigger.Execute();

			break;
		}
	case ESYSTEM_EVENT_AUDIO_UNMUTE:
		{
			g_unmuteAllTrigger.Execute();

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
		CATLAudioObject::CreateAllocator(g_cvars.m_audioObjectPoolSize);

		if (g_cvars.m_audioEventPoolSize < 1)
		{
			g_cvars.m_audioEventPoolSize = 1;
			Cry::Audio::Log(ELogType::Warning, R"(Audio Event pool size must be at least 1. Forcing the cvar "s_AudioEventPoolSize" to 1!)");
		}
		CATLEvent::CreateAllocator(g_cvars.m_audioEventPoolSize);

		if (g_cvars.m_audioStandaloneFilePoolSize < 1)
		{
			g_cvars.m_audioStandaloneFilePoolSize = 1;
			Cry::Audio::Log(ELogType::Warning, R"(Audio Standalone File pool size must be at least 1. Forcing the cvar "s_AudioStandaloneFilePoolSize" to 1!)");
		}
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

		SAudioManagerRequestData<EAudioManagerRequestType::ReleaseAudioImpl> releaseImplData;
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
	SAudioManagerRequestData<EAudioManagerRequestType::SetAudioImpl> requestData(pIImpl);
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
	SAudioObjectRequestData<EAudioObjectRequestType::LoadTrigger> requestData(triggerId);
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
	SAudioObjectRequestData<EAudioObjectRequestType::UnloadTrigger> requestData(triggerId);
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
	SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTriggerEx> requestData(triggerData);
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
	SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> requestData(triggerId);
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
		SAudioObjectRequestData<EAudioObjectRequestType::StopTrigger> requestData(triggerId);
		CAudioRequest request(&requestData);
		request.flags = userData.flags;
		request.pOwner = userData.pOwner;
		request.pUserData = userData.pUserData;
		request.pUserDataOwner = userData.pUserDataOwner;
		PushRequest(request);
	}
	else
	{
		SAudioObjectRequestData<EAudioObjectRequestType::StopAllTriggers> requestData;
		CAudioRequest request(&requestData);
		request.flags = userData.flags;
		request.pOwner = userData.pOwner;
		request.pUserData = userData.pUserData;
		request.pUserDataOwner = userData.pUserDataOwner;
		PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<EAudioObjectRequestType::SetParameter> requestData(parameterId, value);
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
	SAudioObjectRequestData<EAudioObjectRequestType::SetSwitchState> requestData(switchId, switchStateId);
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
	SAudioObjectRequestData<EAudioObjectRequestType::PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
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
	SAudioObjectRequestData<EAudioObjectRequestType::StopFile> requestData(szName);
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
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedFile> requestData(standaloneFile, bSuccessfullyStarted);
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
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStoppedFile> requestData(standaloneFile);
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
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedEvent> requestData(event, bSuccess);
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
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportVirtualizedEvent> requestData(event);
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
	SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportPhysicalizedEvent> requestData(event);
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
	SAudioManagerRequestData<EAudioManagerRequestType::StopAllSounds> requestData;
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
	SAudioManagerRequestData<EAudioManagerRequestType::RefreshAudioSystem> requestData(szLevelName);
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
	SAudioManagerRequestData<EAudioManagerRequestType::ParseControlsData> requestData(szFolderPath, dataScope);
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
	SAudioManagerRequestData<EAudioManagerRequestType::ParsePreloadsData> requestData(szFolderPath, dataScope);
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
	SAudioManagerRequestData<EAudioManagerRequestType::PreloadSingleRequest> requestData(id, bAutoLoadOnly);
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
	SAudioManagerRequestData<EAudioManagerRequestType::UnloadSingleRequest> requestData(id);
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
	SAudioManagerRequestData<EAudioManagerRequestType::RetriggerAudioControls> requestData;
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
	SAudioManagerRequestData<EAudioManagerRequestType::ReloadControlsData> requestData(szFolderPath, szLevelName);
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
	return m_configPath.c_str();
}

///////////////////////////////////////////////////////////////////////////
CryAudio::IListener* CSystem::CreateListener(CObjectTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	CATLListener* pListener = nullptr;
	SAudioListenerRequestData<EAudioListenerRequestType::RegisterListener> requestData(&pListener, transformation, szName);
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request);

	return static_cast<CryAudio::IListener*>(pListener);
}

///////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseListener(CryAudio::IListener* const pIListener)
{
	SAudioListenerRequestData<EAudioListenerRequestType::ReleaseListener> requestData(static_cast<CATLListener*>(pIListener));
	CAudioRequest request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
CryAudio::IObject* CSystem::CreateObject(SCreateObjectData const& objectData /*= SCreateObjectData::GetEmptyObject()*/, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	CATLAudioObject* const pObject = new CATLAudioObject(objectData.transformation);
	SAudioObjectRequestData<EAudioObjectRequestType::RegisterObject> requestData(objectData);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pObject = pObject;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
	return static_cast<CryAudio::IObject*>(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseObject(CryAudio::IObject* const pIObject)
{
	SAudioObjectRequestData<EAudioObjectRequestType::ReleaseObject> requestData;
	CAudioRequest request(&requestData);
	request.pObject = static_cast<CATLAudioObject*>(pIObject);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetFileData(char const* const szName, SFileData& fileData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SAudioManagerRequestData<EAudioManagerRequestType::GetAudioFileData> requestData(szName, fileData);
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
void CSystem::OnLoadLevel(char const* const szLevelName)
{
	// Requests need to be blocking so data is available for next preloading request!
	CryFixedStringT<MaxFilePathLength> audioLevelPath(m_configPath);
	audioLevelPath += s_szLevelsFolderName;
	audioLevelPath += "/";
	audioLevelPath += szLevelName;
	SAudioManagerRequestData<EAudioManagerRequestType::ParseControlsData> requestData1(audioLevelPath, EDataScope::LevelSpecific);
	CAudioRequest request1(&requestData1);
	request1.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request1);

	SAudioManagerRequestData<EAudioManagerRequestType::ParsePreloadsData> requestData2(audioLevelPath, EDataScope::LevelSpecific);
	CAudioRequest request2(&requestData2);
	request2.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request2);

	PreloadRequestId const preloadRequestId = StringToId(szLevelName);
	SAudioManagerRequestData<EAudioManagerRequestType::PreloadSingleRequest> requestData3(preloadRequestId, true);
	CAudioRequest request3(&requestData3);
	request3.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request3);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnUnloadLevel()
{
	SAudioManagerRequestData<EAudioManagerRequestType::UnloadAFCMDataByScope> requestData1(EDataScope::LevelSpecific);
	CAudioRequest request1(&requestData1);
	request1.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request1);

	SAudioManagerRequestData<EAudioManagerRequestType::ClearControlsData> requestData2(EDataScope::LevelSpecific);
	CAudioRequest request2(&requestData2);
	request2.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request2);

	SAudioManagerRequestData<EAudioManagerRequestType::ClearPreloadsData> requestData3(EDataScope::LevelSpecific);
	CAudioRequest request3(&requestData3);
	request3.flags = ERequestFlags::ExecuteBlocking;
	PushRequest(request3);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseImpl()
{
	// Reject new requests during shutdown.
	m_flags |= EInternalStates::ImplShuttingDown;

	// Release middleware specific data before its shutdown.
	g_fileManager.ReleaseImplData();
	g_listenerManager.ReleaseImplData();
	g_eventManager.ReleaseImplData();
	g_objectManager.ReleaseImplData();

	g_pIImpl->DestructObject(g_pObject->GetImplDataPtr());
	g_pObject->SetImplDataPtr(nullptr);

	delete g_pObject;
	g_pObject = nullptr;

	g_xmlProcessor.ClearPreloadsData(EDataScope::All);
	g_xmlProcessor.ClearControlsData(EDataScope::All);

	g_pIImpl->ShutDown();
	g_pIImpl->Release();
	g_pIImpl = nullptr;

	// Release engine specific data after impl shut down to prevent dangling data accesses during shutdown.
	// Note: The object and listener managers are an exception as we need their data to survive in case the middleware is swapped out.
	g_eventManager.Release();
	g_fileManager.Release();

	m_flags &= ~EInternalStates::ImplShuttingDown;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageChanged()
{
	SAudioManagerRequestData<EAudioManagerRequestType::ChangeLanguage> requestData;
	CAudioRequest request(&requestData);
	request.flags = ERequestFlags::ExecuteBlocking;
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
		switch (request.GetData()->type)
		{
		case EAudioRequestType::AudioObjectRequest:
			{
				result = ProcessObjectRequest(request);

				break;
			}
		case EAudioRequestType::AudioListenerRequest:
			{
				result = ProcessListenerRequest(request.GetData());

				break;
			}
		case EAudioRequestType::AudioCallbackManagerRequest:
			{
				result = ProcessCallbackManagerRequest(request);

				break;
			}
		case EAudioRequestType::AudioManagerRequest:
			{
				result = ProcessManagerRequest(request);

				break;
			}
		default:
			{
				CRY_ASSERT_MESSAGE(false, "Unknown audio request type: %u", static_cast<int>(request.GetData()->type));

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
	SAudioManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioManagerRequestDataBase const*>(request.GetData());

	switch (pRequestDataBase->type)
	{
	case EAudioManagerRequestType::AddRequestListener:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::AddRequestListener> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::AddRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.AddRequestListener(pRequestData);

			break;
		}
	case EAudioManagerRequestType::RemoveRequestListener:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::RemoveRequestListener> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::RemoveRequestListener> const*>(request.GetData());
			result = g_eventListenerManager.RemoveRequestListener(pRequestData->func, pRequestData->pObjectToListenTo);

			break;
		}
	case EAudioManagerRequestType::SetAudioImpl:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::SetAudioImpl> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::SetAudioImpl> const*>(request.GetData());
			result = HandleSetImpl(pRequestData->pIImpl);

			break;
		}
	case EAudioManagerRequestType::RefreshAudioSystem:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::RefreshAudioSystem> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::RefreshAudioSystem> const*>(request.GetData());
			result = HandleRefresh(pRequestData->levelName);

			break;
		}
	case EAudioManagerRequestType::StopAllSounds:
		{
			result = g_pIImpl->StopAllSounds();

			break;
		}
	case EAudioManagerRequestType::ParseControlsData:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::ParseControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::ParseControlsData> const*>(request.GetData());
			g_xmlProcessor.ParseControlsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::ParsePreloadsData:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::ParsePreloadsData> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::ParsePreloadsData> const*>(request.GetData());
			g_xmlProcessor.ParsePreloadsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::ClearControlsData:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::ClearControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::ClearControlsData> const*>(request.GetData());
			g_xmlProcessor.ClearControlsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::ClearPreloadsData:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::ClearPreloadsData> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::ClearPreloadsData> const*>(request.GetData());
			g_xmlProcessor.ClearPreloadsData(pRequestData->dataScope);

			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::PreloadSingleRequest:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::PreloadSingleRequest> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::PreloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryLoadRequest(pRequestData->audioPreloadRequestId, ((request.flags & ERequestFlags::ExecuteBlocking) != 0), pRequestData->bAutoLoadOnly);

			break;
		}
	case EAudioManagerRequestType::UnloadSingleRequest:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::UnloadSingleRequest> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::UnloadSingleRequest> const*>(request.GetData());
			result = g_fileCacheManager.TryUnloadRequest(pRequestData->audioPreloadRequestId);

			break;
		}
	case EAudioManagerRequestType::UnloadAFCMDataByScope:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::UnloadAFCMDataByScope> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::UnloadAFCMDataByScope> const*>(request.GetData());
			result = g_fileCacheManager.UnloadDataByScope(pRequestData->dataScope);

			break;
		}
	case EAudioManagerRequestType::ReleaseAudioImpl:
		{
			ReleaseImpl();
			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::ChangeLanguage:
		{
			SetImplLanguage();

			g_fileCacheManager.UpdateLocalizedFileCacheEntries();
			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::RetriggerAudioControls:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			HandleRetriggerControls();
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EAudioManagerRequestType::ReleasePendingRays:
		{
			g_objectManager.ReleasePendingRays();
			result = ERequestStatus::Success;

			break;
		}
	case EAudioManagerRequestType::ReloadControlsData:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			SAudioManagerRequestData<EAudioManagerRequestType::ReloadControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<EAudioManagerRequestType::ReloadControlsData> const*>(request.GetData());

			for (auto const pObject : g_objectManager.GetObjects())
			{
				for (auto const pEvent : pObject->GetActiveEvents())
				{
					CRY_ASSERT_MESSAGE((pEvent != nullptr) && (pEvent->IsPlaying() || pEvent->IsVirtual()), "Invalid event during EAudioManagerRequestType::ReloadControlsData");
					pEvent->Stop();
				}
			}

			g_xmlProcessor.ClearControlsData(EDataScope::All);
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
	case EAudioManagerRequestType::DrawDebugInfo:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			HandleDrawDebug();
			result = ERequestStatus::Success;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case EAudioManagerRequestType::GetAudioFileData:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::GetAudioFileData> const* const pRequestData =
				static_cast<SAudioManagerRequestData<EAudioManagerRequestType::GetAudioFileData> const* const>(request.GetData());
			g_pIImpl->GetFileData(pRequestData->name.c_str(), pRequestData->fileData);
			break;
		}
	case EAudioManagerRequestType::GetImplInfo:
		{
			SAudioManagerRequestData<EAudioManagerRequestType::GetImplInfo> const* const pRequestData =
				static_cast<SAudioManagerRequestData<EAudioManagerRequestType::GetImplInfo> const* const>(request.GetData());
			g_pIImpl->GetInfo(pRequestData->implInfo);
			break;
		}
	case EAudioManagerRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "ATL received an unknown AudioManager request: %u", pRequestDataBase->type);
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
	SAudioCallbackManagerRequestDataBase const* const pRequestDataBase =
		static_cast<SAudioCallbackManagerRequestDataBase const* const>(request.GetData());

	switch (pRequestDataBase->type)
	{
	case EAudioCallbackManagerRequestType::ReportStartedEvent:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedEvent> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedEvent> const* const>(request.GetData());
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
	case EAudioCallbackManagerRequestType::ReportFinishedEvent:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedEvent> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedEvent> const* const>(request.GetData());
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
	case EAudioCallbackManagerRequestType::ReportVirtualizedEvent:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportVirtualizedEvent> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportVirtualizedEvent> const* const>(request.GetData());

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
	case EAudioCallbackManagerRequestType::ReportPhysicalizedEvent:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportPhysicalizedEvent> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportPhysicalizedEvent> const* const>(request.GetData());

			pRequestData->audioEvent.m_state = EEventState::Playing;
			pRequestData->audioEvent.m_pAudioObject->RemoveFlag(EObjectFlags::Virtual);

			result = ERequestStatus::Success;

			break;
		}
	case EAudioCallbackManagerRequestType::ReportStartedFile:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedFile> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedFile> const* const>(request.GetData());

			CATLStandaloneFile& audioStandaloneFile = pRequestData->audioStandaloneFile;

			g_objectManager.GetStartedStandaloneFileRequestData(&audioStandaloneFile, request);
			audioStandaloneFile.m_state = (pRequestData->bSuccess) ? EAudioStandaloneFileState::Playing : EAudioStandaloneFileState::None;

			result = (pRequestData->bSuccess) ? ERequestStatus::Success : ERequestStatus::Failure;

			break;
		}
	case EAudioCallbackManagerRequestType::ReportStoppedFile:
		{
			SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStoppedFile> const* const pRequestData =
				static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStoppedFile> const* const>(request.GetData());

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
	case EAudioCallbackManagerRequestType::ReportFinishedTriggerInstance:
	case EAudioCallbackManagerRequestType::None:
		{
			result = ERequestStatus::Success;

			break;
		}
	default:
		{
			result = ERequestStatus::FailureInvalidRequest;
			Cry::Audio::Log(ELogType::Warning, "ATL received an unknown AudioCallbackManager request: %u", pRequestDataBase->type);

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

	SAudioObjectRequestDataBase const* const pBaseRequestData =
		static_cast<SAudioObjectRequestDataBase const* const>(request.GetData());

	switch (pBaseRequestData->type)
	{
	case EAudioObjectRequestType::LoadTrigger:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::LoadTrigger> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::LoadTrigger> const* const>(request.GetData());

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
	case EAudioObjectRequestType::UnloadTrigger:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::UnloadTrigger> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::UnloadTrigger> const* const>(request.GetData());

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
	case EAudioObjectRequestType::PlayFile:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::PlayFile> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::PlayFile> const* const>(request.GetData());

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
	case EAudioObjectRequestType::StopFile:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::StopFile> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::StopFile> const* const>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				pObject->HandleStopFile(pRequestData->file.c_str());
				result = ERequestStatus::Success;
			}

			break;
		}
	case EAudioObjectRequestType::ExecuteTrigger:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> const* const>(request.GetData());

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
	case EAudioObjectRequestType::ExecuteTriggerEx:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTriggerEx> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTriggerEx> const* const>(request.GetData());

			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
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
	case EAudioObjectRequestType::StopTrigger:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::StopTrigger> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::StopTrigger> const* const>(request.GetData());

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
	case EAudioObjectRequestType::StopAllTriggers:
		{
			pObject->StopAllTriggers();
			result = ERequestStatus::Success;

			break;
		}
	case EAudioObjectRequestType::SetTransformation:
		{
			CRY_ASSERT_MESSAGE(pObject != g_pObject, "Received a request to set a transformation on the global object.");

			SAudioObjectRequestData<EAudioObjectRequestType::SetTransformation> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetTransformation> const* const>(request.GetData());

			pObject->HandleSetTransformation(pRequestData->transformation);
			result = ERequestStatus::Success;

			break;
		}
	case EAudioObjectRequestType::SetParameter:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::SetParameter> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetParameter> const* const>(request.GetData());

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
	case EAudioObjectRequestType::SetSwitchState:
		{
			result = ERequestStatus::FailureInvalidControlId;
			SAudioObjectRequestData<EAudioObjectRequestType::SetSwitchState> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetSwitchState> const* const>(request.GetData());

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
	case EAudioObjectRequestType::SetOcclusionType:
		{
			CRY_ASSERT_MESSAGE(pObject != g_pObject, "Received a request to set the occlusion type on the global object.");

			SAudioObjectRequestData<EAudioObjectRequestType::SetOcclusionType> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetOcclusionType> const* const>(request.GetData());

			SetOcclusionType(*pObject, pRequestData->occlusionType);
			result = ERequestStatus::Success;

			break;
		}
	case EAudioObjectRequestType::SetCurrentEnvironments:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::SetCurrentEnvironments> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetCurrentEnvironments> const* const>(request.GetData());

			SetCurrentEnvironmentsOnObject(pObject, pRequestData->entityToIgnore);

			break;
		}
	case EAudioObjectRequestType::SetEnvironment:
		{
			if (pObject != g_pObject)
			{
				SAudioObjectRequestData<EAudioObjectRequestType::SetEnvironment> const* const pRequestData =
					static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetEnvironment> const* const>(request.GetData());

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
	case EAudioObjectRequestType::RegisterObject:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::RegisterObject> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::RegisterObject> const*>(request.GetData());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pObject->Init(pRequestData->name.c_str(), g_pIImpl->ConstructObject(pRequestData->transformation, pRequestData->name.c_str()), pRequestData->entityId);
#else
			pObject->Init(nullptr, g_pIImpl->ConstructObject(pRequestData->transformation, nullptr), pRequestData->entityId);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pObject->HandleSetTransformation(pRequestData->transformation);

			if (pRequestData->setCurrentEnvironments)
			{
				SetCurrentEnvironmentsOnObject(pObject, INVALID_ENTITYID);
			}

			SetOcclusionType(*pObject, pRequestData->occlusionType);
			g_objectManager.RegisterObject(pObject);
			result = ERequestStatus::Success;

			break;
		}
	case EAudioObjectRequestType::ReleaseObject:
		{
			if (pObject != g_pObject)
			{
				pObject->RemoveFlag(EObjectFlags::InUse);
				result = ERequestStatus::Success;
			}
			else
			{
				Cry::Audio::Log(ELogType::Warning, "ATL received a request to release the GlobalAudioObject");
			}

			break;
		}
	case EAudioObjectRequestType::ProcessPhysicsRay:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::ProcessPhysicsRay> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ProcessPhysicsRay> const* const>(request.GetData());

			pObject->ProcessPhysicsRay(pRequestData->pAudioRayInfo);
			result = ERequestStatus::Success;
			break;
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	case EAudioObjectRequestType::SetName:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::SetName> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::SetName> const* const>(request.GetData());

			result = pObject->HandleSetName(pRequestData->name.c_str());

			if (result == ERequestStatus::SuccessNeedsRefresh)
			{
				pObject->ForceImplementationRefresh(true);
				result = ERequestStatus::Success;
			}

			break;
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	case EAudioObjectRequestType::ToggleAbsoluteVelocityTracking:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::ToggleAbsoluteVelocityTracking> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ToggleAbsoluteVelocityTracking> const* const>(request.GetData());

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
	case EAudioObjectRequestType::ToggleRelativeVelocityTracking:
		{
			SAudioObjectRequestData<EAudioObjectRequestType::ToggleRelativeVelocityTracking> const* const pRequestData =
				static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ToggleRelativeVelocityTracking> const* const>(request.GetData());

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
	case EAudioObjectRequestType::None:
		{
			result = ERequestStatus::Success;
			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "ATL received an unknown AudioObject request type: %u", pBaseRequestData->type);
			result = ERequestStatus::FailureInvalidRequest;
			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSystem::ProcessListenerRequest(SAudioRequestData const* const pPassedRequestData)
{
	ERequestStatus result = ERequestStatus::Failure;
	SAudioListenerRequestDataBase const* const pBaseRequestData =
		static_cast<SAudioListenerRequestDataBase const* const>(pPassedRequestData);

	switch (pBaseRequestData->type)
	{
	case EAudioListenerRequestType::SetTransformation:
		{
			SAudioListenerRequestData<EAudioListenerRequestType::SetTransformation> const* const pRequestData =
				static_cast<SAudioListenerRequestData<EAudioListenerRequestType::SetTransformation> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				pRequestData->pListener->HandleSetTransformation(pRequestData->transformation);
			}

			result = ERequestStatus::Success;
		}
		break;
	case EAudioListenerRequestType::RegisterListener:
		{
			SAudioListenerRequestData<EAudioListenerRequestType::RegisterListener> const* const pRequestData =
				static_cast<SAudioListenerRequestData<EAudioListenerRequestType::RegisterListener> const*>(pPassedRequestData);
			*pRequestData->ppListener = g_listenerManager.CreateListener(pRequestData->transformation, pRequestData->name.c_str());
		}
		break;
	case EAudioListenerRequestType::ReleaseListener:
		{
			SAudioListenerRequestData<EAudioListenerRequestType::ReleaseListener> const* const pRequestData =
				static_cast<SAudioListenerRequestData<EAudioListenerRequestType::ReleaseListener> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				g_listenerManager.ReleaseListener(pRequestData->pListener);
				result = ERequestStatus::Success;
			}
		}
		break;
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	case EAudioListenerRequestType::SetName:
		{
			SAudioListenerRequestData<EAudioListenerRequestType::SetName> const* const pRequestData =
				static_cast<SAudioListenerRequestData<EAudioListenerRequestType::SetName> const* const>(pPassedRequestData);

			pRequestData->pListener->HandleSetName(pRequestData->name.c_str());
			result = ERequestStatus::Success;
		}
		break;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	case EAudioListenerRequestType::None:
		result = ERequestStatus::Success;
		break;
	default:
		result = ERequestStatus::FailureInvalidRequest;
		Cry::Audio::Log(ELogType::Warning, "ATL received an unknown AudioListener request type: %u", pPassedRequestData->type);
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
	switch (request.GetData()->type)
	{
	case EAudioRequestType::AudioManagerRequest:
		{
			SAudioManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioManagerRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case EAudioManagerRequestType::SetAudioImpl:
				audioSystemEvent = ESystemEvents::ImplSet;
				break;
			}

			break;
		}
	case EAudioRequestType::AudioCallbackManagerRequest:
		{
			SAudioCallbackManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioCallbackManagerRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case EAudioCallbackManagerRequestType::ReportFinishedTriggerInstance:
				{
					SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedTriggerInstance> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportFinishedTriggerInstance> const* const>(pRequestDataBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = ESystemEvents::TriggerFinished;

					break;
				}
			case EAudioCallbackManagerRequestType::ReportStartedEvent:
				{
					SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedEvent> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedEvent> const* const>(pRequestDataBase);
					pAudioEvent = &pRequestData->audioEvent;

					break;
				}
			case EAudioCallbackManagerRequestType::ReportStartedFile:
				{
					SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedFile> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStartedFile> const* const>(pRequestDataBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = ESystemEvents::FileStarted;

					break;
				}
			case EAudioCallbackManagerRequestType::ReportStoppedFile:
				{
					SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStoppedFile> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<EAudioCallbackManagerRequestType::ReportStoppedFile> const* const>(pRequestDataBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = ESystemEvents::FileStopped;

					break;
				}
			}

			break;
		}
	case EAudioRequestType::AudioObjectRequest:
		{
			SAudioObjectRequestDataBase const* const pRequestDataBase = static_cast<SAudioObjectRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case EAudioObjectRequestType::ExecuteTrigger:
				{
					SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> const* const pRequestData = static_cast<SAudioObjectRequestData<EAudioObjectRequestType::ExecuteTrigger> const* const>(pRequestDataBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = ESystemEvents::TriggerExecuted;

					break;
				}
			case EAudioObjectRequestType::PlayFile:
				audioSystemEvent = ESystemEvents::FilePlay;
				break;
			}

			break;
		}
	case EAudioRequestType::AudioListenerRequest:
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

	result = g_pIImpl->Init(m_objectPoolSize, m_eventPoolSize);

	g_pIImpl->GetInfo(m_implInfo);
	m_configPath = AUDIO_SYSTEM_DATA_ROOT "/";
	m_configPath += (m_implInfo.folderName + "/" + s_szConfigFolderName + "/").c_str();

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

	g_xmlProcessor.ParseControlsData(m_configPath.c_str(), EDataScope::Global);
	g_xmlProcessor.ParsePreloadsData(m_configPath.c_str(), EDataScope::Global);

	// The global preload might not exist if no preloads have been created, for that reason we don't check the result of this call
	result = g_fileCacheManager.TryLoadRequest(GlobalPreloadRequestId, true, true);

	if (szLevelName != nullptr && szLevelName[0] != '\0')
	{
		CryFixedStringT<MaxFilePathLength> levelPath = m_configPath;
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
	SAudioManagerRequestData<EAudioManagerRequestType::GetImplInfo> requestData(implInfo);
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

		SAudioManagerRequestData<EAudioManagerRequestType::DrawDebugInfo> requestData;
		CAudioRequest request(&requestData);
		PushRequest(request);
	}
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

		if (!((g_cvars.m_drawAudioDebug & Debug::EDrawFilter::HideMemoryInfo) != 0))
		{
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemHeaderFontSize, Debug::g_globalColorHeader.data(), false, "Audio System");

			CryModuleMemoryInfo memInfo;
			ZeroStruct(memInfo);
			CryGetMemoryInfoForModule(&memInfo);

			posY += Debug::g_systemHeaderLineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false,
			                      "Total Memory Used: %uKiB",
			                      static_cast<uint32>((memInfo.allocated - memInfo.freed) / 1024));

			{
				CPoolObject<CATLAudioObject, stl::PSyncMultiThread>::Allocator& allocator = CATLAudioObject::GetAllocator();
				posY += Debug::g_systemLineHeight;
				auto mem = allocator.GetTotalMemory();
				auto pool = allocator.GetCounts();
				pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false, "[Objects] In Use: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
			}

			{
				CPoolObject<CATLEvent, stl::PSyncNone>::Allocator& allocator = CATLEvent::GetAllocator();
				posY += Debug::g_systemLineHeight;
				auto mem = allocator.GetTotalMemory();
				auto pool = allocator.GetCounts();
				pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false, "[Events] In Use: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
			}

			{
				CPoolObject<CATLStandaloneFile, stl::PSyncNone>::Allocator& allocator = CATLStandaloneFile::GetAllocator();
				posY += Debug::g_systemLineHeight;
				auto mem = allocator.GetTotalMemory();
				auto pool = allocator.GetCounts();
				pAuxGeom->Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::g_systemColorTextPrimary.data(), false, "[Files] In Use: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
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
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
