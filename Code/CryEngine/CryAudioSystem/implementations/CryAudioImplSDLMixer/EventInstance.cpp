// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EventInstance.h"
#include "Event.h"

#include <SDL_mixer.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
void CEventInstance::Stop()
{
	// need to make a copy because the callback
	// registered with Mix_ChannelFinished can edit the list
	ChannelList const channels = m_channels;
	int const fadeOutTime = m_pEvent->GetFadeOutTime();

	for (int const channel : channels)
	{
		if (fadeOutTime == 0)
		{
			Mix_HaltChannel(channel);
		}
		else
		{
			Mix_FadeOutChannel(channel, fadeOutTime);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::Pause()
{
	// need to make a copy because the callback
	// registered with Mix_ChannelFinished can edit the list
	ChannelList const channels = m_channels;

	for (int const channel : channels)
	{
		Mix_Pause(channel);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::Resume()
{
	// need to make a copy because the callback
	// registered with Mix_ChannelFinished can edit the list
	ChannelList channels = m_channels;

	for (int const channel : channels)
	{
		Mix_Resume(channel);
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
