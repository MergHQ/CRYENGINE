// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "StandaloneFile.h"
#include "Object.h"
#include "SoundEngine.h"

#include <SDL_mixer.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CStandaloneFile::Play(IObject* const pIObject)
{
	ERequestStatus status = ERequestStatus::Failure;

	auto const pObject = static_cast<CObject*>(pIObject);
	status = SoundEngine::PlayFile(pObject, this);

	return status;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CStandaloneFile::Stop(IObject* const pIObject)
{
	ERequestStatus status = ERequestStatus::Failure;

	auto const pObject = static_cast<CObject*>(pIObject);

	for (auto const pTempStandaloneFile : pObject->m_standaloneFiles)
	{
		if (pTempStandaloneFile == this)
		{
			// need to make a copy because the callback
			// registered with Mix_ChannelFinished can edit the list
			ChannelList const channels = m_channels;

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
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
