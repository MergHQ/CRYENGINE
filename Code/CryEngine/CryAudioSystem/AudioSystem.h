// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATL.h"
#include <CrySystem/TimeValue.h>
#include <CryThreading/IThreadManager.h>

// Forward declarations.
class CAudioSystem;
class CAudioProxy;

class CAudioThread : public IThread
{
public:

	CAudioThread();
	~CAudioThread();

	void Init(CAudioSystem* const pAudioSystem);

	// Start accepting work on thread
	virtual void ThreadEntry();

	// Signals the thread that it should not accept anymore work and exit
	void SignalStopWork();

	bool IsActive();
	void Activate();
	void Deactivate();

private:
	CAudioSystem* m_pAudioSystem;
	volatile bool m_bQuit;
};

class CAudioSystem final : public IAudioSystem, ISystemEventListener
{
public:

	CAudioSystem();
	virtual ~CAudioSystem();

	// IAudioSystem
	virtual bool         Initialize() override;
	virtual void         Release() override;
	virtual void         PushRequest(SAudioRequest const& audioRequest) override;
	virtual void         AddRequestListener(void (* func)(SAudioRequestInfo const* const), void* const pObjectToListenTo, EAudioRequestType const requestType = eAudioRequestType_AudioAllRequests, AudioEnumFlagsType const specificRequestMask = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS) override;
	virtual void         RemoveRequestListener(void (* func)(SAudioRequestInfo const* const), void* const pObjectToListenTo) override;

	virtual void         ExternalUpdate() override;

	virtual bool         GetAudioTriggerId(char const* const szAudioTriggerName, AudioControlId& audioTriggerId) const override;
	virtual bool         GetAudioRtpcId(char const* const szAudioRtpcName, AudioControlId& audioRtpcId) const override;
	virtual bool         GetAudioSwitchId(char const* const szAudioSwitchName, AudioControlId& audioSwitchId) const override;
	virtual bool         GetAudioSwitchStateId(AudioControlId const audioSwitchId, char const* const szSwitchStateName, AudioSwitchStateId& audioSwitchStateId) const override;
	virtual bool         GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, AudioPreloadRequestId& audioPreloadRequestId) const override;
	virtual bool         GetAudioEnvironmentId(char const* const szAudioEnvironmentName, AudioEnvironmentId& audioEnvironmentId) const override;

	virtual bool         ReserveAudioListenerId(AudioObjectId& audioObjectId) override;
	virtual bool         ReleaseAudioListenerId(AudioObjectId const audioObjectId) override;

	virtual void         OnCVarChanged(ICVar* const pCvar)  override {}
	virtual char const*  GetConfigPath() const override;

	virtual IAudioProxy* GetFreeAudioProxy() override;
	// If INCLUDE_AUDIO_PRODUCTION_CODE is not defined, these two methods always return nullptr
	virtual char const*  GetAudioControlName(EAudioControlType const audioControlType, AudioIdType const audioControlId) override;
	virtual char const*  GetAudioControlName(EAudioControlType const audioControlType, AudioIdType const audioControlId1, AudioIdType const audioControlId2) override;
	virtual void         GetAudioDebugData(SAudioDebugData& audioDebugData) const override;
	virtual void         GetAudioFileData(char const* const szFilename, SAudioFileData& audioFileData) override;
	// ~IAudioSystem

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	void InternalUpdate();
	void FreeAudioProxy(IAudioProxy* const pIAudioProxy);

private:

	typedef std::deque<CAudioRequestInternal, STLSoundAllocator<CAudioRequestInternal>> TAudioRequests;
	typedef std::vector<CAudioProxy*, STLSoundAllocator<CAudioProxy*>>                  TAudioProxies;

	void        PushRequestInternal(CAudioRequestInternal const& request);
	void        UpdateTime();
	bool        ProcessRequests(TAudioRequests& requestQueue);
	void        ProcessRequest(CAudioRequestInternal& request);
	bool        ExecuteSyncCallbacks(TAudioRequests& requestQueue);
	void        ExtractSyncCallbacks(TAudioRequests& requestQueue, TAudioRequests& syncCallbacksQueue);
	uint32      MoveAudioRequests(TAudioRequests& from, TAudioRequests& to);

	static void OnAudioEvent(SAudioRequestInfo const* const pAudioRequestInfo);

	bool         m_bSystemInitialized;
	CTimeValue   m_lastUpdateTime;
	float        m_deltaTime;
	CAudioThread m_mainAudioThread;

	enum EAudioRequestQueueType : AudioEnumFlagsType
	{
		eAudioRequestQueueType_Asynch = 0,
		eAudioRequestQueueType_Synch  = 1,

		eAudioRequestQueueType_Count
	};

	enum EAudioRequestQueuePriority : AudioEnumFlagsType
	{
		eAudioRequestQueuePriority_High   = 0,
		eAudioRequestQueuePriority_Normal = 1,
		eAudioRequestQueuePriority_Low    = 2,

		eAudioRequestQueuePriority_Count
	};

	enum EAudioRequestQueueIndex : AudioEnumFlagsType
	{
		eAudioRequestQueueIndex_One = 0,
		eAudioRequestQueueIndex_Two = 1,

		eAudioRequestQueueIndex_Count
	};

	TAudioRequests                              m_requestQueues[eAudioRequestQueueType_Count][eAudioRequestQueuePriority_Count][eAudioRequestQueueIndex_Count];
	TAudioRequests                              m_syncCallbacks;
	TAudioRequests                              m_syncCallbacksPending;
	TAudioRequests                              m_internalRequests[eAudioRequestQueueIndex_Count];

	CAudioTranslationLayer                      m_atl;

	CryEvent                                    m_mainEvent;
	CryCriticalSection                          m_mainCS;
	CryCriticalSection                          m_syncCallbacksPendingCS;
	CryCriticalSection                          m_internalRequestsCS[eAudioRequestQueueIndex_Count];

	TAudioProxies                               m_audioProxies;
	TAudioProxies                               m_audioProxiesToBeFreed;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> m_configPath;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryCriticalSection m_debugNameStoreCS;
	CATLDebugNameStore m_debugNameStore;

	void DrawAudioDebugData();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	PREVENT_OBJECT_COPY(CAudioSystem);
};
