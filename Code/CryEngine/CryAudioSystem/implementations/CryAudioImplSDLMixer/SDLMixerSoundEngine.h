// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SDLMixerSoundEngineTypes.h"

namespace SDLMixer
{
namespace SoundEngine
{
typedef void (* TFnEventCallback)(AudioEventId);
typedef void (* TFnStandaloneFileCallback)(AudioStandaloneFileId const, char const*);

// Global events
bool Init();
void Release();
void Refresh();
void Update();

void Pause();
void Resume();

void Mute();
void UnMute();
void Stop();

// Load / Unload samples
const TSampleID LoadSample(const string& sampleFilePath, bool bOnlyMetadata);
const TSampleID LoadSampleFromMemory(void* pMemory, const size_t nSize, const string& sSamplePath, const TSampleID nID = 0);
void            UnloadSample(const TSampleID nID);

// Events
CryAudio::Impl::SATLTriggerImplData_sdlmixer* CreateEventData();
bool                                          ExecuteEvent(CryAudio::Impl::SATLAudioObjectData_sdlmixer* const pAudioObject, CryAudio::Impl::SATLTriggerImplData_sdlmixer const* const pEventStaticData, CryAudio::Impl::SATLEventData_sdlmixer* const pEventInstance);
bool                                          PlayFile(CryAudio::Impl::SATLAudioObjectData_sdlmixer* const pAudioObject, CryAudio::Impl::CAudioStandaloneFile_sdlmixer* const pEventInstance, const CryAudio::Impl::SATLTriggerImplData_sdlmixer* const pUsedTrigger, const char* const szFilePath);
bool                                          StopFile(CryAudio::Impl::SATLAudioObjectData_sdlmixer* const pAudioObject, AudioStandaloneFileId const fileInstanceID);

// stops an specific event instance
bool StopEvent(CryAudio::Impl::SATLEventData_sdlmixer const* const pEventInstance);
// stops all the events associated with this trigger
bool StopTrigger(CryAudio::Impl::SATLTriggerImplData_sdlmixer const* const pEventData);

// Listeners
bool SetListenerPosition(const TListenerID nListenerID, const CAudioObjectTransformation& position);

// Audio Objects
bool RegisterAudioObject(CryAudio::Impl::SATLAudioObjectData_sdlmixer* pAudioObjectData);
bool UnregisterAudioObject(CryAudio::Impl::SATLAudioObjectData_sdlmixer* pAudioObjectData);
bool SetAudioObjectPosition(CryAudio::Impl::SATLAudioObjectData_sdlmixer* pAudioObjectData, const CAudioObjectTransformation& position);

// Callbacks
void RegisterEventFinishedCallback(TFnEventCallback pCallbackFunction);
void RegisterStandaloneFileFinishedCallback(TFnStandaloneFileCallback pCallbackFunction);
}
}
