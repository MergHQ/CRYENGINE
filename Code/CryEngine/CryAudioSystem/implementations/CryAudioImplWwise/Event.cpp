// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Common.h"
#include "EventInstance.h"
#include "Impl.h"
#include "Object.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
AkCallbackType SystemToWwiseCallbacks(ESystemEvents const events)
{
	int callbackTypes = AK_EndOfEvent;

	if ((events& ESystemEvents::OnBar) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncBar;
	}

	if ((events& ESystemEvents::OnBeat) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncBeat;
	}

	if ((events& ESystemEvents::OnEntry) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncEntry;
	}

	if ((events& ESystemEvents::OnExit) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncExit;
	}

	if ((events& ESystemEvents::OnGrid) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncGrid;
	}

	if ((events& ESystemEvents::OnSyncPoint) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncPoint;
	}

	if ((events& ESystemEvents::OnUserMarker) != ESystemEvents::None)
	{
		callbackTypes |= AK_MusicSyncUserCue;
	}

	return static_cast<AkCallbackType>(callbackTypes);
}

//////////////////////////////////////////////////////////////////////////
ESystemEvents WwiseToSystemCallback(AkCallbackType const callbackType)
{
	ESystemEvents events = ESystemEvents::None;

	switch (callbackType)
	{
	case AK_MusicSyncBar:
		{
			events = ESystemEvents::OnBar;
			break;
		}
	case AK_MusicSyncBeat:
		{
			events = ESystemEvents::OnBeat;
			break;
		}
	case AK_MusicSyncEntry:
		{
			events = ESystemEvents::OnEntry;
			break;
		}
	case AK_MusicSyncExit:
		{
			events = ESystemEvents::OnExit;
			break;
		}
	case AK_MusicSyncGrid:
		{
			events = ESystemEvents::OnGrid;
			break;
		}
	case AK_MusicSyncPoint:
		{
			events = ESystemEvents::OnSyncPoint;
			break;
		}
	case AK_MusicSyncUserCue:
		{
			events = ESystemEvents::OnUserMarker;
			break;
		}
	default:
		{
			break;
		}
	}

	return events;
}

//////////////////////////////////////////////////////////////////////////
void EventCallback(AkCallbackType callbackType, AkCallbackInfo* pCallbackInfo)
{
	switch (callbackType)
	{
	case AK_EndOfEvent:
		{
			if (!g_pImpl->IsToBeReleased() && (pCallbackInfo->pCookie != nullptr))
			{
				auto const pEventInstance = static_cast<CEventInstance*>(pCallbackInfo->pCookie);
				pEventInstance->SetToBeRemoved();
			}

			break;
		}
	case AK_MusicSyncBar:     // Intentional fall-through.
	case AK_MusicSyncBeat:    // Intentional fall-through.
	case AK_MusicSyncEntry:   // Intentional fall-through.
	case AK_MusicSyncExit:    // Intentional fall-through.
	case AK_MusicSyncGrid:    // Intentional fall-through.
	case AK_MusicSyncPoint:   // Intentional fall-through.
	case AK_MusicSyncUserCue: // Intentional fall-through.
		{
			if (!g_pImpl->IsToBeReleased() && (pCallbackInfo->pCookie != nullptr))
			{
				auto const pEventInstance = static_cast<CEventInstance*>(pCallbackInfo->pCookie);
				gEnv->pAudioSystem->ReportTriggerConnectionInstanceCallback(pEventInstance->GetTriggerInstanceId(), WwiseToSystemCallback(callbackType));
			}

			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void PrepareEventCallback(
	AkUniqueID eventId,
	void const* pBankPtr,
	AKRESULT wwiseResult,
	AkMemPoolId memPoolId,
	void* pCookie)
{
	auto const pEventInstance = static_cast<CEventInstance*>(pCookie);

	if (pEventInstance != nullptr)
	{
		pEventInstance->SetPlayingId(eventId);
	}
}

//////////////////////////////////////////////////////////////////////////
ETriggerResult CEvent::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	return ExecuteInternally(pIObject, triggerInstanceId, AK_EndOfEvent);
}

//////////////////////////////////////////////////////////////////////////
ETriggerResult CEvent::ExecuteWithCallbacks(IObject* const pIObject, TriggerInstanceId const triggerInstanceId, STriggerCallbackData const& callbackData)
{
	return ExecuteInternally(pIObject, triggerInstanceId, SystemToWwiseCallbacks(callbackData.events));
}

//////////////////////////////////////////////////////////////////////////
ETriggerResult CEvent::ExecuteInternally(IObject* const pIObject, TriggerInstanceId const triggerInstanceId, AkCallbackType const callbackTypes)
{
	ETriggerResult result = ETriggerResult::Failure;

	auto const pObject = static_cast<CObject*>(pIObject);

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, *this, *pObject);
#else
	CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, *this);
#endif      // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE

	pObject->SetAuxSendValues();

	AkPlayingID const playingId = AK::SoundEngine::PostEvent(m_id, pObject->GetId(), callbackTypes, &EventCallback, pEventInstance);

	if (playingId != AK_INVALID_PLAYING_ID)
	{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
		{
			CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
			g_playingIds[playingId] = pEventInstance;
		}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE

		pEventInstance->SetPlayingId(playingId);
		pObject->AddEventInstance(pEventInstance);

		result = (pEventInstance->GetState() == EEventInstanceState::Virtual) ? ETriggerResult::Virtual : ETriggerResult::Playing;
	}
	else
	{
		g_pImpl->DestructEventInstance(pEventInstance);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Stop(IObject* const pIObject)
{
	auto const pObject = static_cast<CObject*>(pIObject);
	pObject->StopEvent(m_id);
}

//////////////////////////////////////////////////////////////////////////
void CEvent::DecrementNumInstances()
{
	CRY_ASSERT_MESSAGE(m_numInstances > 0, "Number of event instances must be at least 1 during %s", __FUNCTION__);
	--m_numInstances;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
