// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BiquadIIFilterBank.h"
#include <AK/SoundEngine/Common/IAkMixerPlugin.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <memory>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
enum class EVoiceType
{
	None,
	IsSpatialized,
	NotSpatialized,
};

enum class ESourceDirection
{
	None,
	Left,
	Right,
};

struct SAkUserData final
{
	explicit SAkUserData()
		: pVoiceDelayBuffer(nullptr)
		, voiceDelayPrev(0)
		, voiceCycle(0)
		, gameAzimuth(0.0f)
		, gameElevation(0.0f)
		, quadrant(0)
		, quadrantFine(0.0f)
		, inputType(EVoiceType::None)
		, lastSourceDirection(ESourceDirection::None)
		, pFilterBankA(nullptr)
		, pFilterBankB(nullptr)
	{
	}

	~SAkUserData()
	{
		delete pFilterBankA;
		delete pFilterBankB;
	}

	std::unique_ptr<AkAudioBuffer> pVoiceDelayBuffer;

	int                            voiceDelayPrev;
	int                            voiceCycle;

	AkReal32                       gameAzimuth;
	AkReal32                       gameElevation;

	int                            quadrant;
	float                          quadrantFine;

	EVoiceType                     inputType;
	ESourceDirection               lastSourceDirection;

	SBiquadIIFilterBank*           pFilterBankA;
	SBiquadIIFilterBank*           pFilterBankB;
};
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio