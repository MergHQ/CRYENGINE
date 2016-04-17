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

typedef std::set<int, std::less<int>, STLSoundAllocator<int>> TChannelSet;

struct SATLTriggerImplData_sdlmixer final : public IAudioTrigger
{
	explicit SATLTriggerImplData_sdlmixer()
		: nSampleID(0)
		, fAttenuationMinDistance(0)
		, fAttenuationMaxDistance(100)
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
	TChannelSet                         channels;
	const SATLTriggerImplData_sdlmixer* pStaticData;
};
typedef std::set<SATLEventData_sdlmixer*, std::less<SATLEventData_sdlmixer*>, STLSoundAllocator<SATLEventData_sdlmixer*>> TEventInstanceSet;

struct SATLAudioObjectData_sdlmixer final : public IAudioObject
{
	SATLAudioObjectData_sdlmixer(AudioObjectId nID, bool bIsGlobal)
		: nObjectID(nID)
		, bGlobal(bIsGlobal)
		, bPositionChanged(false) {}

	const AudioObjectId        nObjectID;
	CAudioObjectTransformation position;
	TEventInstanceSet          events;
	bool                       bGlobal;
	bool                       bPositionChanged;
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
