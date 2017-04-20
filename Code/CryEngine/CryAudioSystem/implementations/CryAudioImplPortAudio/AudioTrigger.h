// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <portaudio.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
enum class EEventType : EnumFlagsType
{
	None,
	Start,
	Stop,
};

class CAudioTrigger final : public IAudioTrigger
{
public:

	explicit CAudioTrigger(
	  uint32 const pathId_,
	  int const numLoops_,
	  double const sampleRate_,
	  EEventType const eventType_,
	  CryFixedStringT<MaxFilePathLength> const& filePath_,
	  PaStreamParameters const& streamParameters_)
		: pathId(pathId_)
		, numLoops(numLoops_)
		, sampleRate(sampleRate_)
		, eventType(eventType_)
		, filePath(filePath_)
		, streamParameters(streamParameters_)
	{}

	virtual ~CAudioTrigger() override = default;

	CAudioTrigger(CAudioTrigger const&) = delete;
	CAudioTrigger& operator=(CAudioTrigger const&) = delete;

	// IAudioTrigger
	virtual ERequestStatus Load() const override                                       { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                     { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override { return ERequestStatus::Success; }
	// ~IAudioTrigger

	uint32 const                             pathId;
	int const                                numLoops;
	double const                             sampleRate;
	EEventType const                         eventType;
	CryFixedStringT<MaxFilePathLength> const filePath;
	PaStreamParameters const                 streamParameters;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
