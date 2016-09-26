// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioSystem.h"
#include "AudioCVars.h"
#include "AudioProxy.h"
#include "PropagationProcessor.h"
#include <CrySystem/ITimer.h>
#include <CryString/CryPath.h>

///////////////////////////////////////////////////////////////////////////
void CAudioThread::Init(CAudioSystem* const pAudioSystem)
{
	m_pAudioSystem = pAudioSystem;
}

//////////////////////////////////////////////////////////////////////////
void CAudioThread::ThreadEntry()
{
	while (!m_bQuit)
	{
		m_pAudioSystem->InternalUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioThread::SignalStopWork()
{
	m_bQuit = true;
}

///////////////////////////////////////////////////////////////////////////
void CAudioThread::Activate()
{
	if (!gEnv->pThreadManager->SpawnThread(this, "MainAudioThread"))
	{
		CryFatalError("Error spawning \"MainAudioThread\" thread.");
	}
}

///////////////////////////////////////////////////////////////////////////
void CAudioThread::Deactivate()
{
	SignalStopWork();
	gEnv->pThreadManager->JoinThread(this, eJM_Join);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioThread::IsActive()
{
	// JoinThread returns true if thread is not running.
	// JoinThread returns false if thread is still running
	return !gEnv->pThreadManager->JoinThread(this, eJM_TryJoin);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystem::CAudioSystem()
	: m_bSystemInitialized(false)
	, m_lastUpdateTime()
	, m_deltaTime(0.0f)
	, m_allowedThreadId(gEnv->mMainThreadId)
	, m_atl()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	m_audioProxies.reserve(g_audioCVars.m_audioObjectPoolSize);
	m_audioProxiesToBeFreed.reserve(16);
	m_mainAudioThread.Init(this);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystem::~CAudioSystem()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::PushRequest(SAudioRequest const& audioRequest)
{
	if (m_atl.CanProcessRequests())
	{
		CAudioRequestInternal const request(audioRequest);
		PushRequestInternal(request);
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Discarded PushRequest due to ATL not allowing for new ones!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::AddRequestListener(
  void (* func)(SAudioRequestInfo const* const),
  void* const pObjectToListenTo,
  EAudioRequestType const requestType /* = eART_AUDIO_ALL_REQUESTS */,
  AudioEnumFlagsType const specificRequestMask /* = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS */)
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());

	SAudioManagerRequestDataInternal<eAudioManagerRequestType_AddRequestListener> requestData(pObjectToListenTo, func, requestType, specificRequestMask);
	CAudioRequestInternal request(&requestData);
	request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequestInternal(request);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::RemoveRequestListener(void (* func)(SAudioRequestInfo const* const), void* const pObjectToListenTo)
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());

	SAudioManagerRequestDataInternal<eAudioManagerRequestType_RemoveRequestListener> requestData(pObjectToListenTo, func);
	CAudioRequestInternal request(&requestData);
	request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
	request.pOwner = pObjectToListenTo; // This makes sure that the listener is notified.
	PushRequestInternal(request);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::ExternalUpdate()
{
	CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: External Update");

	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());

	if (!m_syncCallbacksPending.empty())
	{
		CRY_ASSERT(m_syncCallbacks.empty());

		m_syncCallbacksPendingCS.Lock();
		m_syncCallbacks.swap(m_syncCallbacksPending);
		m_syncCallbacksPendingCS.Unlock();

		TAudioRequests::const_iterator iter(m_syncCallbacks.begin());
		TAudioRequests::const_iterator const iterEnd(m_syncCallbacks.end());

		for (; iter != iterEnd; ++iter)
		{
			m_atl.NotifyListener(*iter);
		}

		m_syncCallbacks.clear();
	}

	if (m_internalRequestsCS[eAudioRequestQueueIndex_Two].TryLock())
	{
		ExecuteSyncCallbacks(m_internalRequests[eAudioRequestQueueIndex_Two]);
		m_internalRequestsCS[eAudioRequestQueueIndex_Two].Unlock();
	}

	if (!m_audioProxiesToBeFreed.empty())
	{
		TAudioProxies::const_iterator iter(m_audioProxiesToBeFreed.begin());
		TAudioProxies::const_iterator const iterEnd(m_audioProxiesToBeFreed.end());

		for (; iter != iterEnd; ++iter)
		{
			POOL_FREE(*iter);
		}

		m_audioProxiesToBeFreed.clear();
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	DrawAudioDebugData();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::PushRequestInternal(CAudioRequestInternal const& request)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AUDIO);

	if ((request.flags & eAudioRequestFlags_ThreadSafePush) == 0)
	{
		CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());

		bool const bIsAsync = (request.flags & eAudioRequestFlags_SyncCallback) == 0;

		AudioEnumFlagsType const queueType = bIsAsync ? eAudioRequestQueueType_Asynch : eAudioRequestQueueType_Synch;
		AudioEnumFlagsType queuePriority = eAudioRequestQueuePriority_Low;

		if ((request.flags & eAudioRequestFlags_PriorityHigh) > 0)
		{
			queuePriority = eAudioRequestQueuePriority_High;
		}
		else if ((request.flags & eAudioRequestFlags_PriorityNormal) > 0)
		{
			queuePriority = eAudioRequestQueuePriority_Normal;
		}

		{
			CryAutoLock<CryCriticalSection> lock(m_mainCS);
			m_requestQueues[queueType][queuePriority][eAudioRequestQueueIndex_One].push_back(request);
		}

		if ((request.flags & eAudioRequestFlags_ExecuteBlocking) > 0)
		{
			m_mainEvent.Wait();
			m_mainEvent.Reset();

			if (!bIsAsync)
			{
				TAudioRequests syncCallbackRequests;

				{
					CryAutoLock<CryCriticalSection> lock(m_mainCS);
					ExtractSyncCallbacks(m_requestQueues[eAudioRequestQueueType_Synch][queuePriority][eAudioRequestQueueIndex_Two], syncCallbackRequests);
				}

				// Notify the listeners from producer thread but do not keep m_mainCS locked.
				if (!syncCallbackRequests.empty())
				{
					for (auto const& syncCallbackRequest : syncCallbackRequests)
					{
						m_atl.NotifyListener(syncCallbackRequest);
					}

					syncCallbackRequests.clear();
				}
			}
		}
	}
	else
	{
		CryAutoLock<CryCriticalSection> lock(m_internalRequestsCS[eAudioRequestQueueIndex_One]);
		m_internalRequests[eAudioRequestQueueIndex_One].push_back(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::UpdateTime()
{
	CTimeValue const currentAsyncTime(gEnv->pTimer->GetAsyncTime());
	m_deltaTime += (currentAsyncTime - m_lastUpdateTime).GetMilliSeconds();
	m_lastUpdateTime = currentAsyncTime;
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			const string levelNameOnly = (const char*)wparam;

			if (!levelNameOnly.empty() && levelNameOnly.compareNoCase("Untitled") != 0)
			{
				CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> audioLevelPath(gEnv->pAudioSystem->GetConfigPath());
				audioLevelPath += "levels" CRY_NATIVE_PATH_SEPSTR;
				audioLevelPath += levelNameOnly;

				SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> requestData1(audioLevelPath, eAudioDataScope_LevelSpecific);
				SAudioRequest request;
				request.flags = (eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking); // Needs to be blocking so data is available for next preloading request!
				request.pData = &requestData1;
				PushRequest(request);

				SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> requestData2(audioLevelPath, eAudioDataScope_LevelSpecific);
				request.pData = &requestData2;
				PushRequest(request);

				AudioPreloadRequestId audioPreloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;

				if (GetAudioPreloadRequestId(levelNameOnly.c_str(), audioPreloadRequestId))
				{
					SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> requestData3(audioPreloadRequestId, true);
					request.pData = &requestData3;
					PushRequest(request);
				}
			}

			break;
		}
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			// This event is issued in Editor and Game mode.
			CPropagationProcessor::s_bCanIssueRWIs = false;

			SAudioManagerRequestDataInternal<eAudioManagerRequestType_ReleasePendingRays> requestData;
			CAudioRequestInternal request(&requestData);
			request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
			PushRequestInternal(request);

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
void CAudioSystem::InternalUpdate()
{
	uint32 flag = 0;

	{
		CRY_PROFILE_REGION(PROFILE_AUDIO, "Audio: Internal Update");

		UpdateTime();
		m_atl.Update(m_deltaTime);
		m_deltaTime = 0.0f;

		if (m_internalRequestsCS[eAudioRequestQueueIndex_Two].TryLock())
		{
			{
				CryAutoLock<CryCriticalSection> lock(m_internalRequestsCS[eAudioRequestQueueIndex_One]);
				MoveAudioRequests(m_internalRequests[eAudioRequestQueueIndex_One], m_internalRequests[eAudioRequestQueueIndex_Two]);
			}

			ProcessRequests(m_internalRequests[eAudioRequestQueueIndex_Two]);
			m_internalRequestsCS[eAudioRequestQueueIndex_Two].Unlock();
		}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		{
			CryAutoLock<CryCriticalSection> lock(m_debugNameStoreCS);
			m_debugNameStore.SyncChanges(m_atl.GetDebugStore());
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

		// The execution time of this block defines the maximum amount of time the producer will be locked.
		{
			CryAutoLock<CryCriticalSection> lock(m_mainCS);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_Two]);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_Two]);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_Two]);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_Two]);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_Two]);
			flag |= MoveAudioRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_One], m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_Two]);
		}

		// Process HIGH if not empty, else NORMAL, else LOW.
		bool const bAsynch =
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_Two]) ||
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_Two]) ||
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Asynch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_Two]);

		bool const bSynch =
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_High][eAudioRequestQueueIndex_Two]) ||
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Normal][eAudioRequestQueueIndex_Two]) ||
		  ProcessRequests(m_requestQueues[eAudioRequestQueueType_Synch][eAudioRequestQueuePriority_Low][eAudioRequestQueueIndex_Two]);
	}

	if (flag == 0)
	{
		CRY_PROFILE_REGION_WAITING(PROFILE_AUDIO, "Wait - Audio Update");

		CrySleep(10);
	}
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::Initialize()
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());

	if (!m_bSystemInitialized)
	{
		if (m_mainAudioThread.IsActive())
		{
			m_mainAudioThread.Deactivate();
		}

		m_configPath = CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH>((PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "ace" CRY_NATIVE_PATH_SEPSTR).c_str());
		m_atl.Initialize();
		m_mainAudioThread.Activate();

		for (int i = 0; i < g_audioCVars.m_audioObjectPoolSize; ++i)
		{
			POOL_NEW_CREATE(CAudioProxy, pAudioProxy);
			m_audioProxies.push_back(pAudioProxy);
		}

		AddRequestListener(&CAudioSystem::OnAudioEvent, nullptr, eAudioRequestType_AudioManagerRequest, eAudioManagerRequestType_ReserveAudioObjectId);
		m_bSystemInitialized = true;
	}

	return m_bSystemInitialized;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystem::Release()
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());

	for (auto* const pAudioProxy : m_audioProxies)
	{
		POOL_FREE(pAudioProxy);
	}

	for (auto* const pAudioProxy : m_audioProxiesToBeFreed)
	{
		POOL_FREE(pAudioProxy);
	}

	stl::free_container(m_audioProxies);
	stl::free_container(m_audioProxiesToBeFreed);

	SAudioManagerRequestDataInternal<eAudioManagerRequestType_ReleaseAudioImpl> releaseImplData;
	CAudioRequestInternal releaseImplRequest(&releaseImplData);
	releaseImplRequest.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
	PushRequestInternal(releaseImplRequest);

	RemoveRequestListener(&CAudioSystem::OnAudioEvent, nullptr);

	m_mainAudioThread.Deactivate();
	bool const bSuccess = m_atl.ShutDown();
	m_bSystemInitialized = false;

	POOL_FREE(this);

	g_audioCVars.UnregisterVariables();

	// The AudioSystem must be the last object that is freed from the audio memory pool before it is destroyed and the first that is allocated from it!
	uint8* pMemSystem = g_audioMemoryPoolPrimary.Data();
	g_audioMemoryPoolPrimary.UnInitMem();
	delete[] pMemSystem;
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioTriggerId(char const* const szAudioTriggerName, AudioControlId& audioTriggerId) const
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioTriggerId(szAudioTriggerName, audioTriggerId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioRtpcId(char const* const szAudioRtpcName, AudioControlId& audioRtpcId) const
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioRtpcId(szAudioRtpcName, audioRtpcId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioSwitchId(char const* const szAudioSwitchName, AudioControlId& audioSwitchId) const
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioSwitchId(szAudioSwitchName, audioSwitchId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioSwitchStateId(
  AudioControlId const audioSwitchId,
  char const* const szSwitchStateName,
  AudioSwitchStateId& audioSwitchStateId) const
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioSwitchStateId(audioSwitchId, szSwitchStateName, audioSwitchStateId);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, AudioPreloadRequestId& audioPreloadRequestId) const
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioPreloadRequestId(szAudioPreloadRequestName, audioPreloadRequestId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::GetAudioEnvironmentId(char const* const szAudioEnvironmentName, AudioEnvironmentId& audioEnvironmentId) const
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	return m_atl.GetAudioEnvironmentId(szAudioEnvironmentName, audioEnvironmentId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::ReserveAudioListenerId(AudioObjectId& audioObjectId)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	return m_atl.ReserveAudioListenerId(audioObjectId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioSystem::ReleaseAudioListenerId(AudioObjectId const audioObjectId)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	return m_atl.ReleaseAudioListenerId(audioObjectId);
}

///////////////////////////////////////////////////////////////////////////
char const* CAudioSystem::GetConfigPath() const
{
	return m_configPath.c_str();
}

//////////////////////////////////////////////////////////////////////////
IAudioProxy* CAudioSystem::GetFreeAudioProxy()
{
	CRY_ASSERT(m_allowedThreadId == CryGetCurrentThreadId());
	CAudioProxy* pAudioProxy = nullptr;

	if (!m_audioProxies.empty())
	{
		pAudioProxy = m_audioProxies.back();
		m_audioProxies.pop_back();
	}
	else
	{
		POOL_NEW(CAudioProxy, pAudioProxy);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		if (pAudioProxy == nullptr)
		{
			CryFatalError("<Audio>: Failed to create new AudioProxy instance!");
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	return static_cast<IAudioProxy*>(pAudioProxy);
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::FreeAudioProxy(IAudioProxy* const pIAudioProxy)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	CAudioProxy* const pAudioProxy = static_cast<CAudioProxy*>(pIAudioProxy);

	if (m_audioProxies.size() < g_audioCVars.m_audioObjectPoolSize)
	{
		m_audioProxies.push_back(pAudioProxy);
	}
	else
	{
		m_audioProxiesToBeFreed.push_back(pAudioProxy);
	}
}

///////////////////////////////////////////////////////////////////////////
char const* CAudioSystem::GetAudioControlName(EAudioControlType const audioControlType, AudioIdType const audioControlId)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryAutoLock<CryCriticalSection> lock(m_debugNameStoreCS);
	char const* szResult = nullptr;

	switch (audioControlType)
	{
	case eAudioControlType_AudioObject:
		szResult = m_debugNameStore.LookupAudioObjectName(audioControlId);
		break;
	case eAudioControlType_Trigger:
		szResult = m_debugNameStore.LookupAudioTriggerName(audioControlId);
		break;
	case eAudioControlType_Rtpc:
		szResult = m_debugNameStore.LookupAudioRtpcName(audioControlId);
		break;
	case eAudioControlType_Switch:
		szResult = m_debugNameStore.LookupAudioSwitchName(audioControlId);
		break;
	case eAudioControlType_Preload:
		szResult = m_debugNameStore.LookupAudioPreloadRequestName(audioControlId);
		break;
	case eAudioControlType_Environment:
		szResult = m_debugNameStore.LookupAudioEnvironmentName(audioControlId);
		break;
	case eAudioControlType_SwitchState:
		g_audioLogger.Log(eAudioLogType_Warning, "GetAudioConstrolName() for eACT_SWITCH_STATE was called with one AudioEntityID, needs two: SwitchID and StateID");
		break;
	case eAudioControlType_None: // fall-through
	default:
		break;
	}

	return szResult;

#else  // INCLUDE_AUDIO_PRODUCTION_CODE
	return nullptr;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
char const* CAudioSystem::GetAudioControlName(EAudioControlType const audioControlType, AudioIdType const audioControlId1, AudioIdType const audioControlId2)
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryAutoLock<CryCriticalSection> lock(m_debugNameStoreCS);
	char const* szResult = nullptr;

	switch (audioControlType)
	{
	case eAudioControlType_AudioObject:
	case eAudioControlType_Trigger:
	case eAudioControlType_Rtpc:
	case eAudioControlType_Switch:
	case eAudioControlType_Preload:
	case eAudioControlType_Environment:
		g_audioLogger.Log(eAudioLogType_Warning, "GetAudioConstrolName() was called with two AudioEntityIDs for a control that requires only one");
		break;
	case eAudioControlType_SwitchState:
		szResult = m_debugNameStore.LookupAudioSwitchStateName(audioControlId1, audioControlId2);
		break;
	case eAudioControlType_None: // fall-through
	default:
		break;
	}

	return szResult;

#else  // INCLUDE_AUDIO_PRODUCTION_CODE
	return nullptr;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::GetAudioDebugData(SAudioDebugData& audioDebugData) const
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryAutoLock<CryCriticalSection> lock(m_debugNameStoreCS);
	m_atl.GetDebugStore().GetAudioDebugData(audioDebugData);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::GetAudioFileData(char const* const szFilename, SAudioFileData& audioFileData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	SAudioManagerRequestDataInternal<eAudioManagerRequestType_GetAudioFileData> requestData(szFilename, audioFileData);
	CAudioRequestInternal request(&requestData);
	request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
	PushRequestInternal(request);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::GetAudioTriggerData(AudioControlId const audioTriggerId, SAudioTriggerData& audioTriggerData)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_atl.GetAudioTriggerData(audioTriggerId, audioTriggerData);
#endif
}

//////////////////////////////////////////////////////////////////////////
bool CAudioSystem::ExecuteSyncCallbacks(TAudioRequests& requestQueue)
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);

	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());
	bool bSuccess = false;

	if (!requestQueue.empty())
	{
		bSuccess = true;

		// Don't use iterators because "NotifyListener" can invalidate
		// the rRequestQueue container, or change its size.
		for (size_t i = 0, increment = 1; i < requestQueue.size(); i += increment)
		{
			increment = 1;
			CAudioRequestInternal const& request = requestQueue[i];

			if ((request.flags & eAudioRequestFlags_SyncCallback) > 0 && (request.status == eAudioRequestStatus_Success || request.status == eAudioRequestStatus_Failure))
			{
				increment = 0;
				m_atl.NotifyListener(request);
				requestQueue.erase(requestQueue.begin() + i);
			}
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::ExtractSyncCallbacks(TAudioRequests& requestQueue, TAudioRequests& syncCallbacksQueue)
{
	if (!requestQueue.empty())
	{
		TAudioRequests::iterator iter(requestQueue.begin());
		TAudioRequests::const_iterator iterEnd(requestQueue.end());

		while (iter != iterEnd)
		{
			CAudioRequestInternal const& refRequest = (*iter);

			if ((refRequest.flags & eAudioRequestFlags_SyncCallback) > 0 && (refRequest.status == eAudioRequestStatus_Success || refRequest.status == eAudioRequestStatus_Failure))
			{
				syncCallbacksQueue.push_back(refRequest);
				iter = requestQueue.erase(iter);
				iterEnd = requestQueue.end();
				continue;
			}

			++iter;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
uint32 CAudioSystem::MoveAudioRequests(TAudioRequests& from, TAudioRequests& to)
{
	if (!from.empty())
	{
		to.insert(to.end(), std::make_move_iterator(from.begin()), std::make_move_iterator(from.end()));
		from.clear();
		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::OnAudioEvent(SAudioRequestInfo const* const pAudioRequestInfo)
{
	if (pAudioRequestInfo->requestResult == eAudioRequestResult_Success &&
	    pAudioRequestInfo->audioRequestType == eAudioRequestType_AudioManagerRequest &&
	    pAudioRequestInfo->pOwner != nullptr)
	{
		EAudioManagerRequestType const audioManagerRequestType = static_cast<EAudioManagerRequestType const>(pAudioRequestInfo->specificAudioRequest);

		if (audioManagerRequestType == eAudioManagerRequestType_ReserveAudioObjectId)
		{
			CAudioProxy* const pAudioProxy = static_cast<CAudioProxy* const>(pAudioRequestInfo->pOwner);
			pAudioProxy->ExecuteQueuedCommands();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystem::ProcessRequest(CAudioRequestInternal& request)
{
	m_atl.ProcessRequest(request);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioSystem::ProcessRequests(TAudioRequests& requestQueue)
{
	bool bSuccess = false;

	if (!requestQueue.empty())
	{
		// Don't use iterators because "NotifyListener" can invalidate
		// the rRequestQueue container, or change its size.
		for (size_t i = 0, increment = 1; i < requestQueue.size(); i += increment)
		{
			increment = 1;
			CAudioRequestInternal& request = requestQueue[i];

			if ((request.internalInfoFlags & eAudioRequestInfoFlags_WaitingForRemoval) == 0)
			{
				if (request.status == eAudioRequestStatus_None)
				{
					request.status = eAudioRequestStatus_Pending;
					ProcessRequest(request);
					bSuccess = true;
				}
				else
				{
					// TODO: handle pending requests!
				}

				if (request.status != eAudioRequestStatus_Pending)
				{
					if ((request.flags & eAudioRequestFlags_SyncCallback) == 0)
					{
						m_atl.NotifyListener(request);

						if ((request.flags & eAudioRequestFlags_ExecuteBlocking) != 0)
						{
							m_mainEvent.Set();
						}

						requestQueue.erase(requestQueue.begin() + i);
						increment = 0;
					}
					else
					{
						if ((request.flags & eAudioRequestFlags_ExecuteBlocking) != 0)
						{
							request.internalInfoFlags |= eAudioRequestInfoFlags_WaitingForRemoval;
							m_mainEvent.Set();
						}
						else
						{
							m_syncCallbacksPendingCS.Lock();
							m_syncCallbacksPending.push_back(request);
							m_syncCallbacksPendingCS.Unlock();

							requestQueue.erase(requestQueue.begin() + i);
							increment = 0;
						}
					}
				}
			}
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
void CAudioSystem::DrawAudioDebugData()
{
	CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());

	if (g_audioCVars.m_drawAudioDebug > 0)
	{
		SAudioManagerRequestDataInternal<eAudioManagerRequestType_DrawDebugInfo> requestData;
		CAudioRequestInternal request(&requestData);
		request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
		PushRequestInternal(request);
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
