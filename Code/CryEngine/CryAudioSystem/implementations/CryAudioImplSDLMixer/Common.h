// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CEvent;
class CListener;
class CObject;
class CStandaloneFile;

extern bool g_bMuted;
extern CListener* g_pListener;

using SampleId = uint;
using ChannelList = std::vector<int>;
using EventInstanceList = std::vector<CEvent*>;
using StandAloneFileInstanceList = std::vector<CStandaloneFile*>;

float GetVolumeMultiplier(CObject* const pObject, SampleId const sampleId);
int   GetAbsoluteVolume(int const triggerVolume, float const multiplier);
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio