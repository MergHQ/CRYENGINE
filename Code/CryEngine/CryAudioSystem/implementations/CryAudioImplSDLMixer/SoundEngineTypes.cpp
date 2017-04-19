// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SoundEngineTypes.h"
#include "SoundEngine.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioEvent::Stop()
{
	if (SoundEngine::StopEvent(this))
	{
		return ERequestStatus::Pending;
	}
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::Update()
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::Set3DAttributes(SObject3DAttributes const& attributes)
{
	SoundEngine::SetAudioObjectPosition(this, attributes.transformation);
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetParameter(IParameter const* const pIAudioParameter, float const value)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent)
{
	if ((pIAudioTrigger != nullptr) && (pIAudioEvent != nullptr))
	{
		SAudioTrigger const* const pTrigger = static_cast<SAudioTrigger const* const>(pIAudioTrigger);
		SAudioEvent* const pEvent = static_cast<SAudioEvent* const>(pIAudioEvent);

		if (SoundEngine::ExecuteEvent(this, pTrigger, pEvent))
		{
			return ERequestStatus::Success;
		}
	}
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::StopAllTriggers()
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::PlayFile(IAudioStandaloneFile* const pIFile)
{
	if (SoundEngine::PlayFile(this, static_cast<CAudioStandaloneFile*>(pIFile)))
	{
		return ERequestStatus::Success;

	}
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::StopFile(IAudioStandaloneFile* const pIFile)
{
	if (SoundEngine::StopFile(this, static_cast<CAudioStandaloneFile*>(pIFile)))
	{
		return ERequestStatus::Pending;
	}
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetName(char const* const szName)
{
	// SDL_mixer does not have the concept of audio objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioListener::Set3DAttributes(SObject3DAttributes const& attributes)
{
	SoundEngine::SetListenerPosition(listenerId, attributes.transformation);
	return ERequestStatus::Success;
}

}

}
}
