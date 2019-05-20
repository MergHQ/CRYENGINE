// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
class CTransformation;
namespace Impl
{
namespace SDL_mixer
{
class CEventInstance;
class CImpl;
class CObject;
class CEvent;

extern bool g_bMuted;
extern CImpl* g_pImpl;

using SampleId = uint;
using ChannelList = std::vector<int>;
using EventInstances = std::vector<CEventInstance*>;

using Objects = std::vector<CObject*>;
extern Objects g_objects;

float GetVolumeMultiplier(CObject* const pObject, SampleId const sampleId);
int   GetAbsoluteVolume(int const eventVolume, float const multiplier);
void  GetDistanceAngleToObject(
	CTransformation const& listenerTransformation,
	CTransformation const& objectTransformation,
	float& distance,
	float& angle);
void SetChannelPosition(CEvent const& event, int const channelID, float const distance, float const angle);

#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
extern size_t g_loadedSampleSize;
#endif // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE
}      // namespace SDL_mixer
}      // namespace Impl
}      // namespace CryAudio