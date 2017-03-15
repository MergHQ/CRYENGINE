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

	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::Set3DAttributes(SObject3DAttributes const& attributes)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetParameter(IParameter const* const pIAudioParameter, float const value)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent)
{
	ERequestStatus requestResult = eRequestStatus_Failure;
	CAudioTrigger const* const pAudioTrigger = static_cast<CAudioTrigger const* const>(pIAudioTrigger);
	CAudioEvent* const pAudioEvent = static_cast<CAudioEvent*>(pIAudioEvent);

	if ((pAudioTrigger != nullptr) && (pAudioEvent != nullptr))
	{
		if (pAudioTrigger->eventType == ePortAudioEventType_Start)
		{
			requestResult = pAudioEvent->Execute(
			  pAudioTrigger->numLoops,
			  pAudioTrigger->sampleRate,
			  pAudioTrigger->filePath,
			  pAudioTrigger->streamParameters) ? eRequestStatus_Success : eRequestStatus_Failure;

			if (requestResult == eRequestStatus_Success)
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
			requestResult = eRequestStatus_Failure;
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
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::PlayFile(IAudioStandaloneFile* const pIFile)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::StopFile(IAudioStandaloneFile* const pIFile)
{
	return eRequestStatus_Success;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetName(char const* const szName)
{
	// PortAudio does not have the concept of audio objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return eRequestStatus_Success;
}
