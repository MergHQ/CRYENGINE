// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>
#include <STLSoundAllocator.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
typedef uint                                     SampleId;
typedef uint                                     ListenerId;
typedef std::vector<int, STLSoundAllocator<int>> ChannelList;

struct SAudioTrigger final : public IAudioTrigger
{
	SAudioTrigger() = default;
	virtual ~SAudioTrigger() override = default;

	SAudioTrigger(SAudioTrigger const&) = delete;
	SAudioTrigger(SAudioTrigger&&) = delete;
	SAudioTrigger& operator=(SAudioTrigger const&) = delete;
	SAudioTrigger& operator=(SAudioTrigger&&) = delete;

	SampleId sampleId = 0;
	float    attenuationMinDistance = 0.0f;
	float    attenuationMaxDistance = 100.0f;
	int      volume = 128;
	int      loopCount = 1;
	bool     bPanningEnabled = true;
	bool     bStartEvent = true;
};

struct SAudioParameter final : public IAudioRtpc
{
	// Empty implementation so that the engine has something
	// to refer to since RTPCs are not currently supported by
	// the SDL Mixer implementation
	SAudioParameter() = default;
	virtual ~SAudioParameter() override = default;

	SAudioParameter(SAudioParameter const&) = delete;
	SAudioParameter(SAudioParameter&&) = delete;
	SAudioParameter& operator=(SAudioParameter const&) = delete;
	SAudioParameter& operator=(SAudioParameter&&) = delete;
};

struct SAudioSwitchState final : public IAudioSwitchState
{
	// Empty implementation so that the engine has something
	// to refer to since switches are not currently supported by
	// the SDL Mixer implementation
	SAudioSwitchState() = default;
	virtual ~SAudioSwitchState() override = default;

	SAudioSwitchState(SAudioSwitchState const&) = delete;
	SAudioSwitchState(SAudioSwitchState&&) = delete;
	SAudioSwitchState& operator=(SAudioSwitchState const&) = delete;
	SAudioSwitchState& operator=(SAudioSwitchState&&) = delete;
};

struct SAudioEnvironment final : public IAudioEnvironment
{
	// Empty implementation so that the engine has something
	// to refer to since environments are not currently supported by
	// the SDL Mixer implementation
	SAudioEnvironment() = default;
	virtual ~SAudioEnvironment() override = default;

	SAudioEnvironment(SAudioEnvironment const&) = delete;
	SAudioEnvironment(SAudioEnvironment&&) = delete;
	SAudioEnvironment& operator=(SAudioEnvironment const&) = delete;
	SAudioEnvironment& operator=(SAudioEnvironment&&) = delete;
};

struct SAudioEvent final : public IAudioEvent
{
	explicit SAudioEvent(AudioEventId const passedId)
		: eventId(passedId)
		, pStaticData(nullptr)
	{}

	virtual ~SAudioEvent() override = default;

	SAudioEvent(SAudioEvent const&) = delete;
	SAudioEvent(SAudioEvent&&) = delete;
	SAudioEvent& operator=(SAudioEvent const&) = delete;
	SAudioEvent& operator=(SAudioEvent&&) = delete;

	void         Reset()
	{
		channels.clear();
		pStaticData = nullptr;
	}

	const AudioEventId   eventId;
	ChannelList          channels;
	const SAudioTrigger* pStaticData;
};

class CAudioStandaloneFile final : public IAudioStandaloneFile
{
public:

	CAudioStandaloneFile() = default;
	virtual ~CAudioStandaloneFile() override = default;

	CAudioStandaloneFile(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile(CAudioStandaloneFile&&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile&&) = delete;

	void                  Reset()
	{
		fileId = INVALID_AUDIO_STANDALONE_FILE_ID;
		fileInstanceId = INVALID_AUDIO_STANDALONE_FILE_ID;
		channels.clear();
		fileName.clear();
	}

	AudioStandaloneFileId                       fileId = INVALID_AUDIO_STANDALONE_FILE_ID;               // ID unique to the file, only needed for the 'finished' request
	AudioStandaloneFileId                       fileInstanceId = INVALID_AUDIO_STANDALONE_FILE_ID;       // ID unique to the file instance, only needed for the 'finished' request
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> fileName;
	ChannelList channels;
};

typedef std::vector<SAudioEvent*, STLSoundAllocator<SAudioEvent*>>                   EventInstanceList;
typedef std::vector<CAudioStandaloneFile*, STLSoundAllocator<CAudioStandaloneFile*>> StandAloneFileInstanceList;

struct SAudioObject final : public IAudioObject
{
	explicit SAudioObject(uint32 id, bool bIsGlobal)
		: audioObjectId(id)
		, bGlobal(bIsGlobal)
		, bPositionChanged(false)
	{}

	virtual ~SAudioObject() override = default;

	SAudioObject(SAudioObject const&) = delete;
	SAudioObject(SAudioObject&&) = delete;
	SAudioObject& operator=(SAudioObject const&) = delete;
	SAudioObject& operator=(SAudioObject&&) = delete;

	const uint32               audioObjectId;
	CAudioObjectTransformation position;
	EventInstanceList          events;
	StandAloneFileInstanceList standaloneFiles;
	bool                       bGlobal;
	bool                       bPositionChanged;
};

struct SAudioListener final : public IAudioListener
{
	explicit SAudioListener(const ListenerId id)
		: listenerId(id)
	{}

	virtual ~SAudioListener() override = default;

	SAudioListener(SAudioListener const&) = delete;
	SAudioListener(SAudioListener&&) = delete;
	SAudioListener& operator=(SAudioListener const&) = delete;
	SAudioListener& operator=(SAudioListener&&) = delete;

	const ListenerId listenerId;
};

struct SAudioFileEntry final : public IAudioFileEntry
{
	SAudioFileEntry() = default;
	virtual ~SAudioFileEntry() override = default;

	SAudioFileEntry(SAudioFileEntry const&) = delete;
	SAudioFileEntry(SAudioFileEntry&&) = delete;
	SAudioFileEntry& operator=(SAudioFileEntry const&) = delete;
	SAudioFileEntry& operator=(SAudioFileEntry&&) = delete;

	SampleId sampleId;
};
}
}
}
