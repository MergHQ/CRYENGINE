// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Event.h"
#include "Impl.h"
#include "Listener.h"
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
void CObject::Update(float const deltaTime)
{
	if (!m_events.empty())
	{
		float distance = 0.0f;
		float angle = 0.0f;
		GetDistanceAngleToObject(g_pListener->GetTransformation(), m_transformation, distance, angle);

		auto iter(m_events.begin());
		auto iterEnd(m_events.end());

		while (iter != iterEnd)
		{
			auto const pEvent = *iter;

			if (pEvent->m_toBeRemoved)
			{
				gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEvent->m_triggerInstanceId);
				g_pImpl->DestructEvent(pEvent);

				if (iter != (iterEnd - 1))
				{
					(*iter) = m_events.back();
				}

				m_events.pop_back();
				iter = m_events.begin();
				iterEnd = m_events.end();
			}
			else
			{
				if (pEvent->m_pTrigger != nullptr)
				{
					for (auto const channelIndex : pEvent->m_channels)
					{
						SetChannelPosition(pEvent->m_pTrigger, channelIndex, distance, angle);
					}
				}

				++iter;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopEvent(uint32 const id)
{
	for (auto const pEvent : m_events)
	{
		if (pEvent->m_triggerId == id)
		{
			pEvent->Stop();
		}
	}
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
