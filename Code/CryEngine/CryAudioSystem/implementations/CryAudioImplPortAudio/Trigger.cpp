// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Object.h"
#include "Event.h"
#include "Impl.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);

		if (eventType == EEventType::Start)
		{
			CEvent* const pEvent = g_pImpl->ConstructEvent(triggerInstanceId);

			requestResult = pEvent->Execute(
				numLoops,
				sampleRate,
				filePath,
				streamParameters) ? ERequestStatus::Success : ERequestStatus::Failure;

			if (requestResult == ERequestStatus::Success)
			{
				pEvent->pObject = pObject;
				pEvent->pathId = pathId;
				pEvent->SetName(m_name.c_str());
				pObject->RegisterEvent(pEvent);
			}
		}
		else
		{
			pObject->StopEvent(pathId);

			// Return failure here so the audio system does not keep track of this event.
			requestResult = ERequestStatus::SuccessDoNotTrack;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object or event pointer passed to the PortAudio implementation of %s.", __FUNCTION__);
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		pObject->StopEvent(pathId);
	}
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
