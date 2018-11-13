// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Event.h"
#include "Trigger.h"
#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CObject::StopEvent(uint32 const pathId)
{
	for (auto const pEvent : m_activeEvents)
	{
		if (pEvent->pathId == pathId)
		{
			pEvent->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::RegisterEvent(CEvent* const pEvent)
{
	m_activeEvents.push_back(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CObject::UnregisterEvent(CEvent* const pEvent)
{
	m_activeEvents.erase(std::remove(m_activeEvents.begin(), m_activeEvents.end(), pEvent), m_activeEvents.end());
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	for (auto const pEvent : m_activeEvents)
	{
		pEvent->Update();
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CTransformation const& transformation)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironmentConnection const* const pIEnvironmentConnection, float const amount)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(IParameterConnection const* const pIParameterConnection, float const value)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchStateConnection const* const pISwitchStateConnection)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusion(float const occlusion)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
{
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::ExecuteTrigger(ITriggerConnection const* const pITriggerConnection, IEvent* const pIEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;
	CTrigger const* const pTrigger = static_cast<CTrigger const* const>(pITriggerConnection);
	CEvent* const pEvent = static_cast<CEvent*>(pIEvent);

	if ((pTrigger != nullptr) && (pEvent != nullptr))
	{
		if (pTrigger->eventType == EEventType::Start)
		{
			requestResult = pEvent->Execute(
				pTrigger->numLoops,
				pTrigger->sampleRate,
				pTrigger->filePath,
				pTrigger->streamParameters) ? ERequestStatus::Success : ERequestStatus::Failure;

			if (requestResult == ERequestStatus::Success)
			{
				pEvent->pObject = this;
				pEvent->pathId = pTrigger->pathId;
				RegisterEvent(pEvent);
			}
		}
		else
		{
			StopEvent(pTrigger->pathId);

			// Return failure here so the audio system does not keep track of this event.
			requestResult = ERequestStatus::Failure;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid AudioObjectData, TriggerData or EventData passed to the PortAudio implementation of ExecuteTrigger.");
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopAllTriggers()
{
	for (auto const pEvent : m_activeEvents)
	{
		pEvent->Stop();
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::PlayFile(IStandaloneFileConnection* const pIStandaloneFileConnection)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::StopFile(IStandaloneFileConnection* const pIStandaloneFileConnection)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
	// PortAudio does not have the concept of audio objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
