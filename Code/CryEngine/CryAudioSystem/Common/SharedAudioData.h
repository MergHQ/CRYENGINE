// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryCore/CryCrc32.h>

#define AUDIO_INVALID_CRC32 (0xFFFFffff)

namespace CryAudio
{
namespace Impl
{
struct SAudioObject3DAttributes
{
	SAudioObject3DAttributes() : velocity(ZERO){}
	CAudioObjectTransformation transformation;
	Vec3                       velocity;
};

//////////////////////////////////////////////////////////////////////////
inline uint32 AudioStringToId(char const* const _szSource)
{
	return CCrc32::ComputeLowercase(_szSource);
}
}
}

static const CryAudio::Impl::SAudioObject3DAttributes g_sNullAudioObjectAttributes;
