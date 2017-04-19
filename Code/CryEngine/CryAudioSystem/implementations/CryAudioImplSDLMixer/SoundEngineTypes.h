// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
typedef uint             SampleId;
typedef uint             ListenerId;
typedef std::vector<int> ChannelList;

struct SAudioTrigger final : public IAudioTrigger
{
	SAudioTrigger() = default;
	virtual ~SAudioTrigger() override = default;

	SAudioTrigger(SAudioTrigger const&) = delete;
	SAudioTrigger(SAudioTrigger&&) = delete;
	SAudioTrigger& operator=(SAudioTrigger const&) = delete;
	SAudioTrigger& operator=(SAudioTrigger&&) = delete;

	// IAudioTrigger
	virtual ERequestStatus Load() const override                                       { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                     { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override { return ERequestStatus::Success; }
	// ~ IAudioTrigger

	SampleId sampleId = 0;
	float    attenuationMinDistance = 0.0f;
	float    attenuationMaxDistance = 100.0f;
	int      volume = 128;
	int      loopCount = 1;
	bool     bPanningEnabled = true;
	bool     bStartEvent = true;
};

struct SAudioParameter final : public IParameter
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

struct SAudioEvent final : public IAudioEvent, public CPoolObject<SAudioEvent, stl::PSyncNone>
{
	explicit SAudioEvent(CATLEvent& event)
		: audioEvent(event)
	{}

	SAudioEvent(SAudioEvent const&) = delete;
	SAudioEvent(SAudioEvent&&) = delete;
	SAudioEvent& operator=(SAudioEvent const&) = delete;
	SAudioEvent& operator=(SAudioEvent&&) = delete;

	// IAudioEvent
	virtual ERequestStatus Stop();
	// ~ IAudioEvent

	CATLEvent&           audioEvent;
	ChannelList          channels;
	const SAudioTrigger* pStaticData = nullptr;
};

class CAudioStandaloneFile final : public IAudioStandaloneFile
{
public:

	CAudioStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile)
		: atlFile(atlStandaloneFile)
		, fileName(szFile)
	{

	}
	virtual ~CAudioStandaloneFile() override = default;

	CAudioStandaloneFile(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile(CAudioStandaloneFile&&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile&&) = delete;

	CATLStandaloneFile&                atlFile;
	SampleId                           sampleId = 0;                       // ID unique to the file, only needed for the 'finished' request
	CryFixedStringT<MaxFilePathLength> fileName;
	ChannelList                        channels;
};

typedef std::vector<SAudioEvent*>          EventInstanceList;
typedef std::vector<CAudioStandaloneFile*> StandAloneFileInstanceList;

struct SAudioObject final : public IAudioObject, public CPoolObject<SAudioObject, stl::PSyncNone>
{
	explicit SAudioObject(uint32 id)
		: audioObjectId(id)
		, bPositionChanged(false)
	{}

	virtual ~SAudioObject() override = default;

	SAudioObject(SAudioObject const&) = delete;
	SAudioObject(SAudioObject&&) = delete;
	SAudioObject& operator=(SAudioObject const&) = delete;
	SAudioObject& operator=(SAudioObject&&) = delete;

	// IAudioObject
	virtual ERequestStatus Update() override;
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override;
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override;
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override;
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override;
	virtual ERequestStatus StopAllTriggers() override;
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus SetName(char const* const szName) override;
	// ~ IAudioObject

	const uint32               audioObjectId;
	CObjectTransformation      position;
	EventInstanceList          events;
	StandAloneFileInstanceList standaloneFiles;
	bool                       bPositionChanged;
};

struct SAudioListener final : public IAudioListener
{
	explicit SAudioListener(const ListenerId id)
		: listenerId(id)
	{}

	virtual ~SAudioListener() override = default;

	// IAudioListener
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	// ~ IAudioListener

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
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
