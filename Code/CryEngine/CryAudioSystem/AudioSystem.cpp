// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioSystem.h"
#include "AudioCVars.h"
#include "ATLAudioObject.h"
#include "PropagationProcessor.h"
#include <CrySystem/ITimer.h>
#include <CryString/CryPath.h>

using namespace CryAudio;

///////////////////////////////////////////////////////////////////////////
void CMainThread::Init(CSystem* const pSystem)
{
	m_pSystem = pSystem;
}

//////////////////////////////////////////////////////////////////////////
void CMainThread::ThreadEntry()
{
	while (!m_bQuit)
	{
		m_pSystem->InternalUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainThread::SignalStopWork()
{
	m_bQuit = true;
}

///////////////////////////////////////////////////////////////////////////
void CMainThread::Activate()
{
	if (!gEnv->pThreadManager->SpawnThread(this, "MainAudioThread"))
	{
		CryFatalError("Error spawning \"MainAudioThread\" thread.");
	}

	m_pSystem->m_mainAudioThreadId = CryGetCurrentThreadId();
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
CSystem::CSystem()
	: m_bSystemInitialized(false)
	, m_lastUpdateTime()
	, m_deltaTime(0.0f)
	, m_atl()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CryAudio::CSystem");
	m_mainThread.Init(this);
}

//////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::AddRequestListener(void (* func)(SRequestInfo const* const), void* const pObjectToListenTo, EnumFlagsType const eventMask)
{
	SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> requestData(pObjectToListenTo, func, eventMask);
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RemoveRequestListener(void (* func)(SRequestInfo const* const), void* const pObjectToListenTo)
{
	SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> requestData(pObjectToListenTo, func);
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_ExecuteBlocking;
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
		m_atl.NotifyListener(request);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	DrawAudioDebugData();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PushRequest(CAudioRequest const& request)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AUDIO);

	if (m_atl.CanProcessRequests())
	{
		m_requestQueue.enqueue(request);

		if ((request.flags & eRequestFlags_ExecuteBlocking) > 0)
		{
			// If sleeping, wake up the audio thread to start processing requests again
			m_audioThreadWakeupEvent.Set();

			m_mainEvent.Wait();
			m_mainEvent.Reset();

			if ((request.flags & eRequestFlags_CallbackOnExternalOrCallingThread) > 0)
			{
				m_atl.NotifyListener(m_syncRequest);
			}
		}
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Discarded PushRequest due to ATL not allowing for new ones!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UpdateTime()
{
	CTimeValue const currentAsyncTime(gEnv->pTimer->GetAsyncTime());
	m_deltaTime += (currentAsyncTime - m_lastUpdateTime).GetMilliSeconds();
	m_lastUpdateTime = currentAsyncTime;
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

			SAudioManagerRequestData<eAudioManagerRequestType_ReleasePendingRays> requestData;
			CAudioRequest request(&requestData);
			request.flags = eRequestFlags_ExecuteBlocking;
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
				gEnv->pInput->AddConsoleEventListener(&m_atl);
			}

			break;
		}
	case ESYSTEM_EVENT_FULL_SHUTDOWN:
	case ESYSTEM_EVENT_FAST_SHUTDOWN:
		{
			if (gEnv->pInput != nullptr)
			{
				gEnv->pInput->RemoveConsoleEventListener(&m_atl);
			}

			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::InternalUpdate()
{
	bool bWorkDone = false;

	{
		CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: Internal Update");

		UpdateTime();
		m_atl.Update(m_deltaTime);
		m_deltaTime = 0.0f;

		bWorkDone = ProcessRequests(m_requestQueue);
	}

	if (bWorkDone == false)
	{
		CRY_PROFILE_REGION_WAITING(PROFILE_AUDIO, "Wait - Audio Update");

		if (m_audioThreadWakeupEvent.Wait(10))
		{
			// Only reset if the event was signalled, not timed-out!
			m_audioThreadWakeupEvent.Reset();
		}
	}
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::Initialize()
{
	if (!m_bSystemInitialized)
	{
		if (m_mainThread.IsActive())
		{
			m_mainThread.Deactivate();
		}

		m_configPath = CryFixedStringT<MaxFilePathLength>((PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "ace" CRY_NATIVE_PATH_SEPSTR).c_str());
		m_atl.Initialize(this);
		m_mainThread.Activate();
		m_bSystemInitialized = true;
	}

	return m_bSystemInitialized;
}

///////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
	SAudioManagerRequestData<eAudioManagerRequestType_ReleaseAudioImpl> releaseImplData;
	CAudioRequest releaseImplRequest(&releaseImplData);
	releaseImplRequest.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(releaseImplRequest);

	m_mainThread.Deactivate();
	bool const bSuccess = m_atl.ShutDown();
	m_bSystemInitialized = false;

	delete this;

	g_audioCVars.UnregisterVariables();

}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetImpl(Impl::IAudioImpl* const pIImpl, SRequestUserData const& userData /*= SAudioRequestUserData::GetEmptyObject()*/)
{
	SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> requestData(pIImpl);
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
	SAudioObjectRequestData<eAudioObjectRequestType_LoadTrigger> requestData(triggerId);
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
	SAudioObjectRequestData<eAudioObjectRequestType_UnloadTrigger> requestData(triggerId);
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
	SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTriggerEx> requestData(triggerData);
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
	SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> requestData(triggerId);
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
		SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> requestData(triggerId);
		CAudioRequest request(&requestData);
		request.flags = userData.flags;
		request.pOwner = userData.pOwner;
		request.pUserData = userData.pUserData;
		request.pUserDataOwner = userData.pUserDataOwner;
		PushRequest(request);
	}
	else
	{
		SAudioObjectRequestData<eAudioObjectRequestType_StopAllTriggers> requestData;
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
	SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> requestData(parameterId, value);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> requestData(audioSwitchId, audioSwitchStateId);
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
	SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> requestData(playFileInfo.szFile, playFileInfo.usedTriggerForPlayback, playFileInfo.bLocalized);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = (userData.pOwner != nullptr) ? userData.pOwner : this;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::StopFile(char const* const szFile, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioObjectRequestData<eAudioObjectRequestType_StopFile> requestData(szFile);
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
  bool bSuccessfulyStarted,
  SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> requestData(standaloneFile, bSuccessfulyStarted);
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
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> requestData(standaloneFile);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReportFinishedEvent(
  CATLEvent& audioEvent,
  bool const bSuccess,
  SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> requestData(audioEvent, bSuccess);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LostFocus(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_LoseFocus> requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GotFocus(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_GetFocus> requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::MuteAll(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_MuteAll> requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnmuteAll(SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_UnmuteAll> requestData;
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
	SAudioManagerRequestData<eAudioManagerRequestType_StopAllSounds> requestData;
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RefreshAudioSystem(char const* const szLevelName, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> requestData(szLevelName);
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
	SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> requestData(szFolderPath, dataScope);
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
	SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> requestData(szFolderPath, dataScope);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::PreloadSingleRequest(PreloadRequestId const audioPreloadRequestId, bool const bAutoLoadOnly, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> requestData(audioPreloadRequestId, bAutoLoadOnly);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnloadSingleRequest(PreloadRequestId const audioPreloadRequestId, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> requestData(audioPreloadRequestId);
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
	SAudioManagerRequestData<eAudioManagerRequestType_RetriggerAudioControls> requestData;
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
	SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> requestData(szFolderPath, szLevelName);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioTriggerId(char const* const szAudioTriggerName, ControlId& audioTriggerId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioTriggerId(szAudioTriggerName, audioTriggerId);
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioParameterId(char const* const szParameterName, ControlId& parameterId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioParameterId(szParameterName, parameterId);
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioSwitchId(char const* const szAudioSwitchName, ControlId& audioSwitchId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioSwitchId(szAudioSwitchName, audioSwitchId);
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioSwitchStateId(
  ControlId const audioSwitchId,
  char const* const szSwitchStateName,
  SwitchStateId& audioSwitchStateId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioSwitchStateId(audioSwitchId, szSwitchStateName, audioSwitchStateId);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, PreloadRequestId& audioPreloadRequestId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioPreloadRequestId(szAudioPreloadRequestName, audioPreloadRequestId);
}

///////////////////////////////////////////////////////////////////////////
bool CSystem::GetAudioEnvironmentId(char const* const szAudioEnvironmentName, EnvironmentId& audioEnvironmentId) const
{
	// TODO: Create blocking requests that do this or re-evaluate the feasibility of this method!
	CRY_ASSERT(m_mainAudioThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioEnvironmentId(szAudioEnvironmentName, audioEnvironmentId);
}

///////////////////////////////////////////////////////////////////////////
char const* CSystem::GetConfigPath() const
{
	return m_configPath.c_str();
}

///////////////////////////////////////////////////////////////////////////
IListener* CSystem::CreateListener()
{
	CATLListener* pListener = nullptr;
	SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioListener> requestData(&pListener);
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request);

	return static_cast<IListener*>(pListener);
}

///////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseListener(IListener* const pIListener)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	SAudioListenerRequestData<eAudioListenerRequestType_ReleaseListener> requestData(static_cast<CATLListener*>(pIListener));
	CAudioRequest request(&requestData);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
IObject* CSystem::CreateObject(SCreateObjectData const& objectData /*= SCreateObjectData::GetEmptyObject()*/, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	CATLAudioObject* const pObject = new CATLAudioObject;
	SAudioObjectRequestData<eAudioObjectRequestType_RegisterObject> requestData(objectData);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pObject = pObject;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	PushRequest(request);
	return static_cast<IObject*>(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ReleaseObject(IObject* const pIObject)
{
	SAudioObjectRequestData<eAudioObjectRequestType_ReleaseObject> requestData;
	CAudioRequest request(&requestData);
	request.pObject = static_cast<CATLAudioObject*>(pIObject);
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetAudioFileData(char const* const szFilename, SFileData& audioFileData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SAudioManagerRequestData<eAudioManagerRequestType_GetAudioFileData> requestData(szFilename, audioFileData);
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetAudioTriggerData(ControlId const audioTriggerId, STriggerData& audioTriggerData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_atl.GetAudioTriggerData(audioTriggerId, audioTriggerData);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLoadLevel(char const* const szLevelName)
{
	// Requests need to be blocking so data is available for next preloading request!
	CryFixedStringT<MaxFilePathLength> audioLevelPath(m_configPath.c_str());
	audioLevelPath += "levels" CRY_NATIVE_PATH_SEPSTR;
	audioLevelPath += szLevelName;
	SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> requestData1(audioLevelPath, eDataScope_LevelSpecific);
	CAudioRequest request1(&requestData1);
	request1.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request1);

	SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> requestData2(audioLevelPath, eDataScope_LevelSpecific);
	CAudioRequest request2(&requestData2);
	request2.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request2);

	PreloadRequestId audioPreloadRequestId = InvalidPreloadRequestId;

	if (m_atl.GetAudioPreloadRequestId(szLevelName, audioPreloadRequestId))
	{
		SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> requestData3(audioPreloadRequestId, true);
		CAudioRequest request3(&requestData3);
		request3.flags = eRequestFlags_ExecuteBlocking;
		PushRequest(request3);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnUnloadLevel()
{
	SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> requestData1(eDataScope_LevelSpecific);
	CAudioRequest request1(&requestData1);
	request1.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request1);

	SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> requestData2(eDataScope_LevelSpecific);
	CAudioRequest request2(&requestData2);
	request2.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request2);

	SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> requestData3(eDataScope_LevelSpecific);
	CAudioRequest request3(&requestData3);
	request3.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request3);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageChanged()
{
	SAudioManagerRequestData<eAudioManagerRequestType_ChangeLanguage> requestData;
	CAudioRequest request(&requestData);
	request.flags = eRequestFlags_ExecuteBlocking;
	PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ProcessRequest(CAudioRequest& request)
{
	m_atl.ProcessRequest(request);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::ProcessRequests(AudioRequests& requestQueue)
{
	bool bSuccess = false;
	CAudioRequest request;

	while (requestQueue.dequeue(request))
	{
		if (request.status == eRequestStatus_None)
		{
			request.status = eRequestStatus_Pending;
			ProcessRequest(request);
			bSuccess = true;
		}
		else
		{
			// TODO: handle pending requests!
		}

		if (request.status != eRequestStatus_Pending)
		{
			if ((request.flags & eRequestFlags_CallbackOnAudioThread) > 0)
			{
				m_atl.NotifyListener(request);

				if ((request.flags & eRequestFlags_ExecuteBlocking) > 0)
				{
					m_mainEvent.Set();
				}
			}
			else if ((request.flags & eRequestFlags_CallbackOnExternalOrCallingThread) > 0)
			{
				if ((request.flags & eRequestFlags_ExecuteBlocking) > 0)
				{
					m_syncRequest = request;
					m_mainEvent.Set();
				}
				else
				{
					m_syncCallbacks.enqueue(request);
				}
			}
			else if ((request.flags & eRequestFlags_ExecuteBlocking) > 0)
			{
				m_mainEvent.Set();
			}
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
void CSystem::DrawAudioDebugData()
{
	if (g_audioCVars.m_drawAudioDebug > 0)
	{
		SAudioManagerRequestData<eAudioManagerRequestType_DrawDebugInfo> requestData;
		CAudioRequest request(&requestData);
		request.flags = eRequestFlags_ExecuteBlocking;
		PushRequest(request);
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
