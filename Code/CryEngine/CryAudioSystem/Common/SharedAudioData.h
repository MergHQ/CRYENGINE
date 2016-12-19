// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryCore/CryCrc32.h>

#define AUDIO_INVALID_CRC32 (0xFFFFffff)

/**
 * @namespace CryAudio
 * @brief Most parent audio namespace used throughout the entire engine.
 */
namespace CryAudio
{
/**
 * @namespace CryAudio::Impl
 * @brief Sub-namespace of the CryAudio namespace used by audio middleware implementations.
 */
namespace Impl
{
/**
 * @struct SObject3DAttributes
 * @brief A struct holding velocity and transformation of audio objects.
 */
struct SObject3DAttributes
{
	SObject3DAttributes()
		: velocity(ZERO)
	{}

	CObjectTransformation transformation;
	Vec3                  velocity;
};

/**
 * A utility function to convert a string value to an Id.
 * @param szSource - string to convert
 * @return a 32bit CRC computed on the lower case version of the passed string
 */
inline uint32 AudioStringToId(char const* const szSource)
{
	return CCrc32::ComputeLowercase(szSource);
}
}
}

static const CryAudio::Impl::SObject3DAttributes g_sNullAudioObjectAttributes;
