// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Event.h"
#include "Parameter.h"
#include "StandaloneFile.h"
#include "SwitchState.h"
#include "Trigger.h"
#include "SoundEngine.h"

#include <SDL_mixer.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
	// SDL_mixer does not have the concept of objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetVolume(SampleId const sampleId, float const value)
{
	m_volumeMultipliers[sampleId] = value;

	if (!g_bMuted)
	{
		float const volumeMultiplier = GetVolumeMultiplier(this, sampleId);

		for (auto const pEvent : m_events)
		{
			auto const pTrigger = pEvent->m_pTrigger;

			if ((pTrigger != nullptr) && (pTrigger->GetSampleId() == sampleId))
			{
				int const mixVolume = GetAbsoluteVolume(pTrigger->GetVolume(), volumeMultiplier);

				for (auto const channel : pEvent->m_channels)
				{
					Mix_Volume(channel, mixVolume);
				}
			}
		}
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
