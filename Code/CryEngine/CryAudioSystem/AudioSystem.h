// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATL.h"
#include <CrySystem/TimeValue.h>
#include <CryThreading/IThreadManager.h>
#include <concqueue/concqueue.hpp>

namespace CryAudio
{
// Forward declarations.
class CSystem;

class CMainThread final : public ::IThread
{
public:

	CMainThread() = default;
	CMainThread(CMainThread const&) = delete;
	CMainThread(CMainThread&&) = delete;
	CMainThread& operator=(CMainThread const&) = delete;
	CMainThread& operator=(CMainThread&&) = delete;

	void         Init(CSystem* const pSystem);

	// IThread
	virtual void ThreadEntry();
	// ~IThread

	// Signals the thread that it should not accept anymore work and exit
	void SignalStopWork();

	bool IsActive();
	void Activate();
	void Deactivate();

private:

	CSystem*      m_pSystem = nullptr;
	volatile bool m_bQuit = false;
};

class CSystem final : public IAudioSystem, ::ISystemEventListener
{
public:

	CSystem();
	virtual ~CSystem() override;

	CSystem(CSystem const&) = delete;
	CSystem(CSystem&&) = delete;
	CSystem& operator=(CSystem const&) = delete;
	CSystem& operator=(CSystem&&) = delete;

	// CryAudio::IAudioSystem
	virtual void          Release() override;
	virtual void          SetImpl(Impl::IAudioImpl* const pIImpl, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          LoadTrigger(ControlId const triggerId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          UnloadTrigger(ControlId const triggerId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ExecuteTriggerEx(SExecuteTriggerData const& triggerData, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          StopTrigger(ControlId const triggerId = InvalidControlId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          StopFile(char const* const szFile, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ReportStartedFile(CATLStandaloneFile& standaloneFile, bool bSuccessfulyStarted, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ReportStoppedFile(CATLStandaloneFile& standaloneFile, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ReportFinishedEvent(CATLEvent& audioEvent, bool const bSuccess, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          LostFocus(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          GotFocus(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          MuteAll(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          UnmuteAll(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          StopAllSounds(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          RefreshAudioSystem(char const* const szLevelName, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          PreloadSingleRequest(PreloadRequestId const audioPreloadRequestId, bool const bAutoLoadOnly, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          UnloadSingleRequest(PreloadRequestId const audioPreloadRequestId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ReloadControlsData(char const* const szFolderPath, char const* const szLevelName, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          AddRequestListener(void (* func)(SRequestInfo const* const), void* const pObjectToListenTo, ESystemEvents const eventMask) override;
	virtual void          RemoveRequestListener(void (* func)(SRequestInfo const* const), void* const pObjectToListenTo) override;
	virtual void          ExternalUpdate() override;
	virtual bool          GetAudioTriggerId(char const* const szAudioTriggerName, ControlId& audioTriggerId) const override;
	virtual bool          GetAudioParameterId(char const* const szParameterName, ControlId& parameterId) const override;
	virtual bool          GetAudioSwitchId(char const* const szAudioSwitchName, ControlId& audioSwitchId) const override;
	virtual bool          GetAudioSwitchStateId(ControlId const audioSwitchId, char const* const szSwitchStateName, SwitchStateId& audioSwitchStateId) const override;
	virtual bool          GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, PreloadRequestId& audioPreloadRequestId) const override;
	virtual bool          GetAudioEnvironmentId(char const* const szAudioEnvironmentName, EnvironmentId& audioEnvironmentId) const override;
	virtual char const*   GetConfigPath() const override;
	virtual IListener*    CreateListener() override;
	virtual void          ReleaseListener(IListener* const pIListener) override;
	virtual IObject*      CreateObject(SCreateObjectData const& objectData = SCreateObjectData::GetEmptyObject(), SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void          ReleaseObject(IObject* const pIObject) override;
	virtual void          GetAudioFileData(char const* const szFilename, SFileData& audioFileData) override;
	virtual void          GetAudioTriggerData(ControlId const audioTriggerId, STriggerData& audioTriggerData) override;
	virtual void          OnLoadLevel(char const* const szLevelName) override;
	virtual void          OnUnloadLevel() override;
	virtual void          OnLanguageChanged() override;
	virtual IProfileData* GetProfileData() const override;
	// ~CryAudio::IAudioSystem

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	bool Initialize();
	void PushRequest(CAudioRequest const& request);
	void ParseControlsData(char const* const szFolderPath, EDataScope const dataScope, SRequestUserData const& userData = SRequestUserData::GetEmptyObject());
	void ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope, SRequestUserData const& userData = SRequestUserData::GetEmptyObject());
	void RetriggerAudioControls(SRequestUserData const& userData = SRequestUserData::GetEmptyObject());

	void InternalUpdate();

	threadID m_mainAudioThreadId;

private:

	typedef ConcQueue<UnboundMPSC, CAudioRequest> AudioRequests;
	typedef ConcQueue<UnboundSPSC, CAudioRequest> AudioRequestsSyncCallbacks;

	void UpdateTime();
	bool ProcessRequests(AudioRequests& requestQueue);
	void ProcessRequest(CAudioRequest& request);

	bool                               m_bSystemInitialized;
	CTimeValue                         m_lastUpdateTime;
	float                              m_deltaTime;
	CMainThread                        m_mainThread;

	CAudioTranslationLayer             m_atl;
	AudioRequests                      m_requestQueue;
	AudioRequestsSyncCallbacks         m_syncCallbacks;
	CAudioRequest                      m_syncRequest;
	CryEvent                           m_mainEvent;
	CryEvent                           m_audioThreadWakeupEvent;
	CryFixedStringT<MaxFilePathLength> m_configPath;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	void DrawAudioDebugData();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
