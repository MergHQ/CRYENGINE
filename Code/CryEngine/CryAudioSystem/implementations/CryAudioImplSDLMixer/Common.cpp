// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "Listener.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
bool g_bMuted = false;
CListener* g_pListener = nullptr;
CObject* g_pObject = nullptr;
Objects g_objects;

//////////////////////////////////////////////////////////////////////////
int GetAbsoluteVolume(int const triggerVolume, float const multiplier)
{
	int absoluteVolume = static_cast<int>(static_cast<float>(triggerVolume) * multiplier);
	absoluteVolume = crymath::clamp(absoluteVolume, 0, 128);
	return absoluteVolume;
}

//////////////////////////////////////////////////////////////////////////
float GetVolumeMultiplier(CObject* const pObject, SampleId const sampleId)
{
	float volumeMultiplier = 1.0f;
	auto const volumeMultiplierPair = pObject->m_volumeMultipliers.find(sampleId);

	if (volumeMultiplierPair != pObject->m_volumeMultipliers.end())
	{
		volumeMultiplier = volumeMultiplierPair->second;
	}

	return volumeMultiplier;
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
