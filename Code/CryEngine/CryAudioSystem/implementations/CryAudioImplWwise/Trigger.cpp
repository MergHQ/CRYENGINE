// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Common.h"
#include "Event.h"

#include <Logger.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
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
		gEnv->pAudioSystem->ReportFinishedEvent(pEvent->m_atlEvent, wwiseResult == AK_Success);
	}
}
//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(AkUniqueID const id, float const maxAttenuation)
	: m_id(id)
	, m_maxAttenuation(maxAttenuation)
{
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
