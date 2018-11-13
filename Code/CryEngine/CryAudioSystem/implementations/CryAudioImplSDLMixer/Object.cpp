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
void CObject::SetParameter(IParameterConnection const* const pIParameterConnection, float const value)
{
	if (pIParameterConnection != nullptr)
	{
		auto const pParameter = static_cast<CParameter const* const>(pIParameterConnection);
		float parameterValue = pParameter->GetMultiplier() * crymath::clamp(value, 0.0f, 1.0f) + pParameter->GetShift();
		parameterValue = crymath::clamp(parameterValue, 0.0f, 1.0f);
		SampleId const sampleId = pParameter->GetSampleId();
		m_volumeMultipliers[sampleId] = parameterValue;
		SetVolume(sampleId);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchStateConnection const* const pISwitchStateConnection)
{
	if (pISwitchStateConnection != nullptr)
	{
		auto const pSwitchState = static_cast<CSwitchState const* const>(pISwitchStateConnection);
		float const switchValue = pSwitchState->GetValue();
		SampleId const sampleId = pSwitchState->GetSampleId();
		m_volumeMultipliers[sampleId] = switchValue;
		SetVolume(sampleId);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::ExecuteTrigger(ITriggerConnection const* const pITriggerConnection, IEvent* const pIEvent)
{
	if ((pITriggerConnection != nullptr) && (pIEvent != nullptr))
	{
		auto const pTrigger = static_cast<CTrigger const*>(pITriggerConnection);
		auto const pEvent = static_cast<CEvent*>(pIEvent);
		return SoundEngine::ExecuteEvent(this, pTrigger, pEvent);
	}

	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::PlayFile(IStandaloneFileConnection* const pIStandaloneFileConnection)
{
	return SoundEngine::PlayFile(this, static_cast<CStandaloneFile*>(pIStandaloneFileConnection));
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::StopFile(IStandaloneFileConnection* const pIStandaloneFileConnection)
{
	ERequestStatus status = ERequestStatus::Failure;

	auto const pStandaloneFile = static_cast<CStandaloneFile*>(pIStandaloneFileConnection);

	for (auto const pTempStandaloneFile : m_standaloneFiles)
	{
		if (pTempStandaloneFile == pStandaloneFile)
		{
			// need to make a copy because the callback
			// registered with Mix_ChannelFinished can edit the list
			ChannelList const channels = pStandaloneFile->m_channels;

			for (auto const channel : channels)
			{
				Mix_HaltChannel(channel);
			}

			status = ERequestStatus::Pending;
			break;
		}
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
	// SDL_mixer does not have the concept of objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetVolume(SampleId const sampleId)
{
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
