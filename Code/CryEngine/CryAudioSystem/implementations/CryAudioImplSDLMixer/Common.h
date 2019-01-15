// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
class CTransformation;
namespace Impl
{
namespace SDL_mixer
{
class CEvent;
class CImpl;
class CListener;
class CObject;
class CStandaloneFile;
class CTrigger;

extern bool g_bMuted;
extern CImpl* g_pImpl;
extern CListener* g_pListener;
extern CObject* g_pObject;

using SampleId = uint;
using ChannelList = std::vector<int>;
using EventInstances = std::vector<CEvent*>;
using StandAloneFileInstanceList = std::vector<CStandaloneFile*>;

using Objects = std::vector<CObject*>;
extern Objects g_objects;

float GetVolumeMultiplier(CObject* const pObject, SampleId const sampleId);
int   GetAbsoluteVolume(int const triggerVolume, float const multiplier);
void  GetDistanceAngleToObject(
	CTransformation const& listenerTransformation,
	CTransformation const& objectTransformation,
	float& distance,
	float& angle);
void SetChannelPosition(CTrigger const* const pTrigger, int const channelID, float const distance, float const angle);
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio