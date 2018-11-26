// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Object.h"
#include "Event.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, IEvent* const pIEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if ((pIObject != nullptr) && (pIEvent != nullptr))
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		auto const pEvent = static_cast<CEvent*>(pIEvent);

		if (eventType == EEventType::Start)
		{
			requestResult = pEvent->Execute(
				numLoops,
				sampleRate,
				filePath,
				streamParameters) ? ERequestStatus::Success : ERequestStatus::Failure;

			if (requestResult == ERequestStatus::Success)
			{
				pEvent->pObject = pObject;
				pEvent->pathId = pathId;
				pObject->RegisterEvent(pEvent);
			}
		}
		else
		{
			pObject->StopEvent(pathId);

			// Return failure here so the audio system does not keep track of this event.
			requestResult = ERequestStatus::Failure;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object or event pointer passed to the PortAudio implementation of %s.", __FUNCTION__);
	}

	return requestResult;
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
