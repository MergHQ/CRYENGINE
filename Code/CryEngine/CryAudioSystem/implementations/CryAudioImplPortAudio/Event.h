// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <portaudio.h>
#include <atomic>
#include <PoolObject.h>

// Forward declare C struct
struct SNDFILE_tag;
using SNDFILE = struct SNDFILE_tag;

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
enum class EEventState
{
	None                  = 0,
	Playing               = BIT(0),
	Stopped               = BIT(1),
	Done                  = BIT(2),
	WaitingForDestruction = BIT(3),
};

class CObject;

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(TriggerInstanceId const triggerInstanceId);
	~CEvent();

	bool Execute(
		int const numLoops,
		double const sampleRate,
		CryFixedStringT<MaxFilePathLength> const& filePath,
		PaStreamParameters const& streamParameters);
	void              Update();
	void              Stop();

	TriggerInstanceId GetTriggerInstanceId() const { return m_triggerInstanceId; }

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	SNDFILE*                 pSndFile;
	PaStream*                pStream;
	void*                    pData;
	CObject*                 pObject;
	int                      numChannels;
	int                      remainingLoops;
	uint32                   pathId;
	PaSampleFormat           sampleFormat;
	std::atomic<EEventState> state;
	bool                     toBeRemoved;

private:

	void Reset();

	TriggerInstanceId const m_triggerInstanceId;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
