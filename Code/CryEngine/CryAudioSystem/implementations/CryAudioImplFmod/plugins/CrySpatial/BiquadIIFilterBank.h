// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BiquadIIFilter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
namespace Plugins
{
struct SBiquadIIFilterBank final
{
	explicit SBiquadIIFilterBank(float const sampleRate)
		: filterBand00(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand01(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand02(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand03(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand04(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand05(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Input Buffer Azimuth
		, filterBand06(BiquadIIFilter(EBiquadType::Highshelf, sampleRate)) // Input Buffer Elevation
		, filterBand07(BiquadIIFilter(EBiquadType::Highshelf, sampleRate)) // Input Buffer Elevation
		, filterBand08(BiquadIIFilter(EBiquadType::Highshelf, sampleRate)) // Input Buffer Elevation
		, filterBand09(BiquadIIFilter(EBiquadType::Highshelf, sampleRate)) // Direct Buffer
		, filterBand10(BiquadIIFilter(EBiquadType::Peak, sampleRate))      // Concealed Buffer
		, filterBand11(BiquadIIFilter(EBiquadType::Highshelf, sampleRate)) // Concealed Buffer
	{
	}

	BiquadIIFilter filterBand00;
	BiquadIIFilter filterBand01;
	BiquadIIFilter filterBand02;
	BiquadIIFilter filterBand03;
	BiquadIIFilter filterBand04;
	BiquadIIFilter filterBand05;
	BiquadIIFilter filterBand06;
	BiquadIIFilter filterBand07;
	BiquadIIFilter filterBand08;
	BiquadIIFilter filterBand09;
	BiquadIIFilter filterBand10;
	BiquadIIFilter filterBand11;
};
} // namespace Plugins
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio