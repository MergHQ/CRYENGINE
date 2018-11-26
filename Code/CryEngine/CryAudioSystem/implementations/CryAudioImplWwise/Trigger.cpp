// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Common.h"
#include "Event.h"
#include "Impl.h"
#include "Object.h"

#include <Logger.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void EndEventCallback(AkCallbackType callbackType, AkCallbackInfo* pCallbackInfo)
{
	if ((callbackType == AK_EndOfEvent) && !g_pImpl->IsToBeReleased() && (pCallbackInfo->pCookie != nullptr))
	{
		auto const pEvent = static_cast<CEvent* const>(pCallbackInfo->pCookie);
		pEvent->m_toBeRemoved = true;
		gEnv->pAudioSystem->ReportFinishedEvent(pEvent->m_event, true);
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
	auto const pEvent = static_cast<CEvent*>(pCookie);

	if (pEvent != nullptr)
	{
		pEvent->m_id = eventId;
		gEnv->pAudioSystem->ReportFinishedEvent(pEvent->m_event, wwiseResult == AK_Success);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, IEvent* const pIEvent)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((pIObject != nullptr) && (pIEvent != nullptr))
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		auto const pEvent = static_cast<CEvent*>(pIEvent);

		// If the user executes a trigger on the global object we want to post events only to that particular object and not globally!
		AkGameObjectID objectId = g_globalObjectId;

		if (pObject->GetId() != AK_INVALID_GAME_OBJECT)
		{
			objectId = pObject->GetId();
			pObject->PostEnvironmentAmounts();
		}

		AkPlayingID const id = AK::SoundEngine::PostEvent(m_id, objectId, AK_EndOfEvent, &EndEventCallback, pEvent);

		if (id != AK_INVALID_PLAYING_ID)
		{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
			pEvent->SetName(m_name.c_str());

			{
				CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
				g_playingIds[id] = pEvent;
			}
#endif      // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

			pEvent->m_id = id;
			pEvent->m_pObject = pObject;
			pEvent->m_maxAttenuation = m_maxAttenuation;

			pObject->SetDistanceToListener();
			pEvent->SetInitialVirtualState(pObject->GetDistanceToListener());

			pObject->AddEvent(pEvent);
			pObject->SetNeedsToUpdateVirtualStates(pObject->GetId() != g_globalObjectId);

			result = (pEvent->m_state == EEventState::Virtual) ? ERequestStatus::SuccessVirtual : ERequestStatus::Success;
		}
		else
		{
			// if posting an Event failed, try to prepare it, if it isn't prepared already
			Cry::Audio::Log(ELogType::Warning, "Failed to Post Wwise event %" PRISIZE_T, pEvent->m_id);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid AudioObjectData, TriggerData or EventData passed to the Wwise implementation of ExecuteTrigger.");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Load() const
{
	return SetLoaded(true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Unload() const
{
	return SetLoaded(false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::LoadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::UnloadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoaded(bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;
	AkUniqueID id = m_id;
	AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload, &id, 1);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		Cry::Audio::Log(
			ELogType::Warning,
			"Wwise - PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
			bLoad ? "Preparation_Load" : "Preparation_Unload",
			m_id,
			wwiseResult);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoadedAsync(IEvent* const pIEvent, bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;

	CEvent* const pEvent = static_cast<CEvent*>(pIEvent);

	if (pEvent != nullptr)
	{
		AkUniqueID id = m_id;
		AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(
			bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
			&id,
			1,
			&PrepareEventCallback,
			pEvent);

		if (IS_WWISE_OK(wwiseResult))
		{
			pEvent->m_id = m_id;
			pEvent->m_state = EEventState::Unloading;
			result = ERequestStatus::Success;
		}
		else
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
				bLoad ? "Preparation_Load" : "Preparation_Unload",
				m_id,
				wwiseResult);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error,
		                "Wwise - Invalid IEvent passed to the Wwise implementation of %sTriggerAsync",
		                bLoad ? "Load" : "Unprepare");
	}

	return result;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
