// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include "FileCacheManager.h"
#include "AudioListenerManager.h"
#include "AudioEventListenerManager.h"
#include "AudioStandaloneFileManager.h"
#include "AudioEventManager.h"
#include "AudioObjectManager.h"
#include "AudioXMLProcessor.h"
#include <CryInput/IInput.h>

class CAudioTranslationLayer final : public IInputEventListener
{
public:

	CAudioTranslationLayer();
	virtual ~CAudioTranslationLayer();

	CAudioTranslationLayer(CAudioTranslationLayer const&) = delete;
	CAudioTranslationLayer(CAudioTranslationLayer&&) = delete;
	CAudioTranslationLayer& operator=(CAudioTranslationLayer const&) = delete;
	CAudioTranslationLayer& operator=(CAudioTranslationLayer&&) = delete;

	// IInputEventListener
	virtual bool OnInputEvent(SInputEvent const& event);
	// ~IInputEventListener

	bool                Initialize();
	bool                ShutDown();
	void                ProcessRequest(CAudioRequestInternal& request);
	void                Update(float const deltaTime);
	bool                GetAudioTriggerId(char const* const szAudioTriggerName, AudioControlId& audioTriggerId) const;
	bool                GetAudioRtpcId(char const* const szAudioRtpcName, AudioControlId& audioRtpcId) const;
	bool                GetAudioSwitchId(char const* const szAudioSwitchName, AudioControlId& audioSwitchId) const;
	bool                GetAudioSwitchStateId(AudioControlId const switchId, char const* const szAudioSwitchStateName, AudioSwitchStateId& audioSwitchStateId) const;
	bool                GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, AudioPreloadRequestId& audioPreloadRequestId) const;
	bool                GetAudioEnvironmentId(char const* const szAudioEnvironmentName, AudioEnvironmentId& audioEnvironmentId) const;
	bool                ReserveAudioObject(CATLAudioObject*& pAudioObject);
	bool                ReleaseAudioObject(CATLAudioObject* const pAudioObject);
	CATLListener*       CreateAudioListener();
	void                Release(CATLListener* const pAudioObject);
	bool                CanProcessRequests() const { return (m_flags & eAudioInternalStates_AudioMiddlewareShuttingDown) == 0; }

	EAudioRequestStatus ParseControlsData(char const* const szFolderPath, EAudioDataScope const dataScope);
	EAudioRequestStatus ClearControlsData(EAudioDataScope const dataScope);
	EAudioRequestStatus ParsePreloadsData(char const* const szFolderPath, EAudioDataScope const dataScope);
	EAudioRequestStatus ClearPreloadsData(EAudioDataScope const dataScope);

	void                NotifyListener(CAudioRequestInternal const& request);

private:

	EAudioRequestStatus ProcessAudioManagerRequest(CAudioRequestInternal const& request);
	EAudioRequestStatus ProcessAudioCallbackManagerRequest(CAudioRequestInternal& request);
	EAudioRequestStatus ProcessAudioObjectRequest(CAudioRequestInternal const& request);
	EAudioRequestStatus ProcessAudioListenerRequest(SAudioRequestDataInternal const* const pPassedRequestData);
	EAudioRequestStatus SetImpl(CryAudio::Impl::IAudioImpl* const pImpl);
	void                ReleaseImpl();
	EAudioRequestStatus PrepUnprepTriggerAsync(
	  CATLAudioObject* const pAudioObject,
	  CATLTrigger const* const pTrigger,
	  bool const bPrepare);
	EAudioRequestStatus PlayFile(
	  CATLAudioObject* const pAudioObject,
	  char const* const szFile,
	  AudioControlId const audioTriggerId,
	  bool const bLocalized,
	  void* const pOwner,
	  void* const pUserData,
	  void* const pUserDataOwner);

	EAudioRequestStatus StopFile(CATLAudioObject* const pAudioObject, char const* const szFile);
	EAudioRequestStatus ActivateTrigger(
	  CATLAudioObject* const pAudioObject,
	  CATLTrigger const* const pTrigger,
	  float const timeUntilRemovalMS,
	  void* const pOwner = nullptr,
	  void* const pUserData = nullptr,
	  void* const pUserDataOwner = nullptr,
	  AudioEnumFlagsType const flags = INVALID_AUDIO_ENUM_FLAG_TYPE);
	EAudioRequestStatus StopTrigger(CATLAudioObject* const pAudioObject, CATLTrigger const* const pTrigger);
	EAudioRequestStatus StopAllTriggers(CATLAudioObject* const pAudioObject);
	EAudioRequestStatus SetSwitchState(
	  CATLAudioObject* const pAudioObject,
	  CATLSwitch const* const pSwitch,
	  CATLSwitchState const* const pState);
	EAudioRequestStatus SetRtpc(
	  CATLAudioObject* const pAudioObject,
	  CATLRtpc const* const pRtpc,
	  float const value);
	EAudioRequestStatus SetEnvironment(
	  CATLAudioObject* const pAudioObject,
	  CATLAudioEnvironment const* const pEnvironment,
	  float const amount);
	EAudioRequestStatus ResetEnvironments(CATLAudioObject* const pAudioObject);
	EAudioRequestStatus ActivateInternalTrigger(
	  CATLAudioObject* const pAudioObject,
	  CryAudio::Impl::IAudioTrigger const* const pAudioTrigger,
	  CryAudio::Impl::IAudioEvent* const pAudioEvent);
	EAudioRequestStatus StopInternalEvent(CATLAudioObject* const pAudioObject, CryAudio::Impl::IAudioEvent const* const pAudioEvent);
	EAudioRequestStatus StopAllInternalEvents(CATLAudioObject* const pAudioObject);
	EAudioRequestStatus SetInternalRtpc(
	  CATLAudioObject* const pAudioObject,
	  CryAudio::Impl::IAudioRtpc const* const pAudioRtpc,
	  float const value);
	EAudioRequestStatus SetInternalSwitchState(CATLAudioObject* const pAudioObject, CryAudio::Impl::IAudioSwitchState const* const pAudioSwitchState);
	EAudioRequestStatus SetInternalEnvironment(
	  CATLAudioObject* const pAudioObject,
	  CryAudio::Impl::IAudioEnvironment const* const pAudioEnvironment,
	  float const amount);
	EAudioRequestStatus RefreshAudioSystem(char const* const szLevelName);
	void                SetImplLanguage();

	enum EAudioInternalStates : AudioEnumFlagsType
	{
		eAudioInternalStates_None                        = 0,
		eAudioInternalStates_IsMuted                     = BIT(0),
		eAudioInternalStates_AudioMiddlewareShuttingDown = BIT(1),
	};

	// ATLObject containers
	AudioTriggerLookup        m_triggers;
	AudioRtpcLookup           m_parameters;
	AudioSwitchLookup         m_switches;
	AudioPreloadRequestLookup m_preloadRequests;
	AudioEnvironmentLookup    m_environments;
	AudioStandaloneFileLookup m_audioStandaloneFiles;

	CATLAudioObject*          m_pGlobalAudioObject;

	AudioTriggerInstanceId    m_triggerInstanceIDCounter;

	// Components
	CAudioStandaloneFileManager m_audioStandaloneFileMgr;
	CAudioEventManager          m_audioEventMgr;
	CAudioObjectManager         m_audioObjectMgr;
	CAudioListenerManager       m_audioListenerMgr;
	CFileCacheManager           m_fileCacheMgr;
	CAudioEventListenerManager  m_audioEventListenerMgr;
	CAudioXMLProcessor          m_xmlProcessor;

	// Utility members
	uint32                      m_lastMainThreadFrameId;
	volatile AudioEnumFlagsType m_flags;
	CryAudio::Impl::IAudioImpl* m_pImpl;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	bool ReserveAudioObject(CATLAudioObject*& audioObject, char const* const szAudioObjectName);
	void DrawAudioSystemDebugInfo();
	void GetAudioTriggerData(AudioControlId const audioTriggerId, SAudioTriggerData& audioTriggerData) const;

private:

	void DrawAudioObjectDebugInfo(IRenderAuxGeom& auxGeom);
	void DrawATLComponentDebugInfo(IRenderAuxGeom& auxGeom, float posX, float const posY);
	void RetriggerAudioControls();

	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> m_implNameString;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
