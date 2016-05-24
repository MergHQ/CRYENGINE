// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioSystemImplementation.h>
#include <STLSoundAllocator.h>

namespace SDLMixer
{
typedef uint TSampleID;
typedef uint TListenerID;
}

namespace CryAudio
{
namespace Impl
{

typedef std::vector<int, STLSoundAllocator<int>> TChannelList;

struct SATLTriggerImplData_sdlmixer final : public IAudioTrigger
{
	explicit SATLTriggerImplData_sdlmixer()
		: nSampleID(0)
		, fAttenuationMinDistance(0.0f)
		, fAttenuationMaxDistance(100.0f)
		, nVolume(128)
		, nLoopCount(1)
		, bPanningEnabled(true)
		, bStartEvent(true)
	{}

	SDLMixer::TSampleID nSampleID;
	float               fAttenuationMinDistance;
	float               fAttenuationMaxDistance;
	int                 nVolume;
	int                 nLoopCount;
	bool                bPanningEnabled;
	bool                bStartEvent;
};

struct SATLRtpcImplData_sdlmixer final : public IAudioRtpc
{
	// Empty implementation so that the engine has something
	// to refer to since RTPCs are not currently supported by
	// the SDL Mixer implementation
};

struct SATLSwitchStateImplData_sdlmixer final : public IAudioSwitchState
{
	// Empty implementation so that the engine has something
	// to refer to since switches are not currently supported by
	// the SDL Mixer implementation
};

struct SATLEnvironmentImplData_sdlmixer final : public IAudioEnvironment
{
	// Empty implementation so that the engine has something
	// to refer to since environments are not currently supported by
	// the SDL Mixer implementation
};

struct SATLEventData_sdlmixer final : public IAudioEvent
{
	explicit SATLEventData_sdlmixer(AudioEventId const nPassedID)
		: nEventID(nPassedID)
		, pStaticData(nullptr)
	{}

	void Reset()
	{
		channels.clear();
		pStaticData = nullptr;
	}
	const AudioEventId                  nEventID;
	TChannelList                        channels;
	const SATLTriggerImplData_sdlmixer* pStaticData;
};

class CAudioStandaloneFile_sdlmixer final : public IAudioStandaloneFile
{
public:
	CAudioStandaloneFile_sdlmixer() : fileId(INVALID_AUDIO_STANDALONE_FILE_ID), fileInstanceId(INVALID_AUDIO_STANDALONE_FILE_ID) {}

	void Reset()
	{
		fileId = INVALID_AUDIO_STANDALONE_FILE_ID;
		fileInstanceId = INVALID_AUDIO_STANDALONE_FILE_ID;
		channels.clear();
		fileName.clear();
	}

	AudioStandaloneFileId fileId;                       // ID unique to the file, only needed for the 'finished' request
	AudioStandaloneFileId fileInstanceId;               // ID unique to the file instance, only needed for the 'finished' request
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> fileName;
	TChannelList                                channels;
};

typedef std::vector<SATLEventData_sdlmixer*, STLSoundAllocator<SATLEventData_sdlmixer*>>               TEventInstanceList;
typedef std::vector<CAudioStandaloneFile_sdlmixer*, STLSoundAllocator<CAudioStandaloneFile_sdlmixer*>> TStandAloneFileInstanceList;

struct SATLAudioObjectData_sdlmixer final : public IAudioObject
{
	SATLAudioObjectData_sdlmixer(AudioObjectId nID, bool bIsGlobal)
		: nObjectID(nID)
		, bGlobal(bIsGlobal)
		, bPositionChanged(false) {}

	const AudioObjectId         nObjectID;
	CAudioObjectTransformation  position;
	TEventInstanceList          events;
	TStandAloneFileInstanceList standaloneFiles;
	bool                        bGlobal;
	bool                        bPositionChanged;
};

struct SATLListenerData_sdlmixer final : public IAudioListener
{
	explicit SATLListenerData_sdlmixer(const SDLMixer::TListenerID nID)
		: nListenerID(nID)
	{}

	const SDLMixer::TListenerID nListenerID;
};

struct SATLAudioFileEntryData_sdlmixer final : public IAudioFileEntry
{
	SATLAudioFileEntryData_sdlmixer()
	{}

	SDLMixer::TSampleID nSampleID;
};

struct SATLAudioStandaloneFile_sdlmixer final : public IAudioStandaloneFile
{
	// Empty implementation so that the engine has something
	// to refer to since support for standalone audio files
	// is currently not implemented to the SDL Mixer implementation
};

}
}
