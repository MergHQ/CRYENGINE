// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <portaudio.h>
#include <atomic>
#include <PoolObject.h>

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

class CAudioEvent final : public IAudioEvent, public CPoolObject<CAudioEvent, stl::PSyncNone>
{
public:

	explicit CAudioEvent(CATLEvent& _audioEvent);
	virtual ~CAudioEvent() override;

	CAudioEvent(CAudioEvent const&) = delete;
	CAudioEvent(CAudioEvent&&) = delete;
	CAudioEvent& operator=(CAudioEvent const&) = delete;
	CAudioEvent& operator=(CAudioEvent&&) = delete;

	bool         Execute(
	  int const numLoops,
	  double const sampleRate,
	  CryFixedStringT<MaxFilePathLength> const& filePath,
	  PaStreamParameters const& streamParameters);
	void Update();

	// IAudioEvent
	virtual ERequestStatus Stop() override;
	// ~IAudioEvent

	SNDFILE*          pSndFile;
	PaStream*         pStream;
	void*             pData;
	CAudioObject*     pPAAudioObject;
	int               numChannels;
	int               remainingLoops;
	CATLEvent&        audioEvent;
	uint32            pathId;
	PaSampleFormat    sampleFormat;
	std::atomic<bool> bDone;
};
}
}
}
