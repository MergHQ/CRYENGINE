// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Object.h"
#include "EventInstance.h"
#include "Impl.h"

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif        // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
ETriggerResult CEvent::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ETriggerResult result = ETriggerResult::Failure;

	auto const pObject = static_cast<CObject*>(pIObject);

	if (actionType == EActionType::Start)
	{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
		CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, *this, *pObject);
#else
		CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, *this);
#endif        // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

		result = pEventInstance->Execute(
			numLoops,
			sampleRate,
			filePath,
			streamParameters) ? ETriggerResult::Playing : ETriggerResult::Failure;

		if (result == ETriggerResult::Playing)
		{
			pObject->RegisterEventInstance(pEventInstance);
		}
	}
	else
	{
		pObject->StopEvent(pathId);

		result = ETriggerResult::DoNotTrack;
	}

	return result;
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
