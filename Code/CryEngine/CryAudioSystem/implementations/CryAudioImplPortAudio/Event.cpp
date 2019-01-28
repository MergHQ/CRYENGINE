// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Object.h"
#include "EventInstance.h"
#include "Impl.h"

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif        // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	auto const pObject = static_cast<CObject*>(pIObject);

	if (actionType == EActionType::Start)
	{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
		CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, pathId, pObject, this);
#else
		CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, pathId);
#endif        // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

		requestResult = pEventInstance->Execute(
			numLoops,
			sampleRate,
			filePath,
			streamParameters) ? ERequestStatus::Success : ERequestStatus::Failure;

		if (requestResult == ERequestStatus::Success)
		{
			pObject->RegisterEventInstance(pEventInstance);
		}
	}
	else
	{
		pObject->StopEvent(pathId);

		requestResult = ERequestStatus::SuccessDoNotTrack;
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Stop(IObject* const pIObject)
{
	auto const pObject = static_cast<CObject*>(pIObject);
	pObject->StopEvent(pathId);
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
