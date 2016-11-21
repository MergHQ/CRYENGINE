// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <portaudio.h>
#include <atomic>

// Forward declare C struct
struct SNDFILE_tag;
typedef struct SNDFILE_tag SNDFILE;

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CAudioObject;

class CAudioEvent final : public IAudioEvent
{
public:

	explicit CAudioEvent(AudioEventId const _audioEventId);
	virtual ~CAudioEvent() override;

	CAudioEvent(CAudioEvent const&) = delete;
	CAudioEvent(CAudioEvent&&) = delete;
	CAudioEvent& operator=(CAudioEvent const&) = delete;
	CAudioEvent& operator=(CAudioEvent&&) = delete;

	bool         Execute(
	  int const numLoops,
	  double const sampleRate,
	  CryFixedStringT<512> const& filePath,
	  PaStreamParameters const& streamParameters);
	void Stop();
	void Reset();
	void Update();

	SNDFILE*           pSndFile;
	PaStream*          pStream;
	void*              pData;
	CAudioObject*      pPAAudioObject;
	int                numChannels;
	int                remainingLoops;
	AudioEventId const audioEventId;
	uint32             pathId;
	PaSampleFormat     sampleFormat;
	std::atomic<bool>  bDone;
};
}
}
}
