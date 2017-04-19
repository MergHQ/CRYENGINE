// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObject.h"
#include "AudioEvent.h"
#include "AudioTrigger.h"

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::PortAudio;

//////////////////////////////////////////////////////////////////////////
void CAudioObject::StopAudioEvent(uint32 const pathId)
{
	for (auto const& pEvent : m_activeEvents)
	{
		if (pEvent->pathId == pathId)
		{
			pEvent->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObject::RegisterAudioEvent(CAudioEvent* const pEvent)
{
	m_activeEvents.push_back(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CAudioObject::UnregisterAudioEvent(CAudioEvent* const pEvent)
{
	m_activeEvents.erase(std::remove(m_activeEvents.begin(), m_activeEvents.end(), pEvent), m_activeEvents.end());
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::Update()
{
	for (auto const pEvent : m_activeEvents)
	{
		pEvent->Update();
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::Set3DAttributes(SObject3DAttributes const& attributes)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetParameter(IParameter const* const pIAudioParameter, float const value)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;
	CAudioTrigger const* const pAudioTrigger = static_cast<CAudioTrigger const* const>(pIAudioTrigger);
	CAudioEvent* const pAudioEvent = static_cast<CAudioEvent*>(pIAudioEvent);

	if ((pAudioTrigger != nullptr) && (pAudioEvent != nullptr))
	{
		if (pAudioTrigger->eventType == EPortAudioEventType::Start)
		{
			requestResult = pAudioEvent->Execute(
			  pAudioTrigger->numLoops,
			  pAudioTrigger->sampleRate,
			  pAudioTrigger->filePath,
			  pAudioTrigger->streamParameters) ? ERequestStatus::Success : ERequestStatus::Failure;

			if (requestResult == ERequestStatus::Success)
			{
				pAudioEvent->pPAAudioObject = this;
				pAudioEvent->pathId = pAudioTrigger->pathId;
				RegisterAudioEvent(pAudioEvent);
			}
		}
		else
		{
			StopAudioEvent(pAudioTrigger->pathId);

			// Return failure here so the ATL does not keep track of this event.
			requestResult = ERequestStatus::Failure;
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the PortAudio implementation of ExecuteTrigger.");
	}

	return requestResult;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::StopAllTriggers()
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::PlayFile(IAudioStandaloneFile* const pIFile)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::StopFile(IAudioStandaloneFile* const pIFile)
{
	return ERequestStatus::Success;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetName(char const* const szName)
{
	// PortAudio does not have the concept of audio objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}
