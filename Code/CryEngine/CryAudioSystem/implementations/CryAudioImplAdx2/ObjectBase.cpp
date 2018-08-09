// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ObjectBase.h"

#include "Impl.h"
#include "Event.h"
#include "Trigger.h"
#include "Parameter.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
CryLockT<CRYLOCK_RECURSIVE> g_mutex;
std::unordered_map<CriAtomExPlaybackId, CATLEvent*> g_activeEvents;

//////////////////////////////////////////////////////////////////////////
void VoiceEventCallback(
	void* pObj,
	CriAtomExVoiceEvent voiceEvent,
	CriAtomExVoiceInfoDetail const* pRequest,
	CriAtomExVoiceInfoDetail const* pRemoved,
	CriAtomExVoiceInfoDetail const* pRemovedInGroup)
{
	if (voiceEvent != CRIATOMEX_VOICE_EVENT_ALLOCATE)
	{
		g_mutex.Lock();
		CriAtomExPlaybackId const id = (pRemoved != nullptr) ? pRemoved->playback_id : pRequest->playback_id;
		// Map is needed because the void pointer always contains the last started event.
		auto const iter = g_activeEvents.find(id);
		CRY_ASSERT_MESSAGE(iter != g_activeEvents.end(), "Playback Id is not in active events");
		auto const pAtlEvent = iter->second;
		g_activeEvents.erase(iter);
		g_mutex.Unlock();

		gEnv->pAudioSystem->ReportFinishedEvent(*pAtlEvent, true);
	}
}

//////////////////////////////////////////////////////////////////////////
CObjectBase::CObjectBase()
{
	ZeroStruct(m_transformation);
	m_p3dSource = criAtomEx3dSource_Create(CImpl::Get3dSourceConfig(), nullptr, 0);
	m_pPlayer = criAtomExPlayer_Create(CImpl::GetPlayerConfig(), nullptr, 0);
	CRY_ASSERT_MESSAGE(m_pPlayer != nullptr, "m_pPlayer is null pointer");
	m_events.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
CObjectBase::~CObjectBase()
{
	criAtomExPlayer_Destroy(m_pPlayer);

	for (auto const pEvent : m_events)
	{
		pEvent->SetObject(nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::Update()
{
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::SetTransformation(CObjectTransformation const& transformation)
{
	TranslateTransformation(transformation, m_transformation);

	criAtomEx3dSource_SetPosition(m_p3dSource, &m_transformation.pos);
	criAtomEx3dSource_SetOrientation(m_p3dSource, &m_transformation.fwd, &m_transformation.up);
	criAtomEx3dSource_Update(m_p3dSource);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObjectBase::ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;
	auto const pTrigger = static_cast<CTrigger const*>(pITrigger);
	auto const pEvent = static_cast<CEvent*>(pIEvent);

	if ((pTrigger != nullptr) && (pEvent != nullptr))
	{
		if (pTrigger->GetTriggerType() == ETriggerType::Trigger)
		{
			EEventType const eventType = pTrigger->GetEventType();

			switch (eventType)
			{
			case EEventType::Start:
				{
					if (StartEvent(pTrigger, pEvent))
					{
						criAtomEx_SetVoiceEventCallback(VoiceEventCallback, static_cast<void*>(pEvent));
						requestResult = ERequestStatus::Success;
					}
				}
				break;
			case EEventType::Stop:
				{
					StopEvent(pTrigger->GetId());
					requestResult = ERequestStatus::SuccessDoNotTrack;
				}
				break;
			case EEventType::Pause:
				{
					PauseEvent(pTrigger->GetId());
					requestResult = ERequestStatus::SuccessDoNotTrack;
				}
				break;
			case EEventType::Resume:
				{
					ResumeEvent(pTrigger->GetId());
					requestResult = ERequestStatus::SuccessDoNotTrack;
				}
				break;
			}
		}
		else
		{
			criAtomEx_ApplyDspBusSnapshot(pTrigger->GetCueName(), 0);
			requestResult = ERequestStatus::SuccessDoNotTrack;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid trigger or event pointers passed to the Adx2 implementation of ExecuteTrigger.");
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::StopAllTriggers()
{
	criAtomExPlayer_Stop(m_pPlayer);

	for (auto const pEvent : m_events)
	{
		pEvent->SetPlaybackId(CRIATOMEX_INVALID_PLAYBACK_ID);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObjectBase::PlayFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObjectBase::StopFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObjectBase::SetName(char const* const szName)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectBase::StartEvent(CTrigger const* const pTrigger, CEvent* const pEvent)
{
	bool eventStarted = false;

	criAtomExPlayer_Set3dListenerHn(m_pPlayer, g_pListener);
	criAtomExPlayer_Set3dSourceHn(m_pPlayer, m_p3dSource);

	CriAtomExPlaybackId id = CRIATOMEX_INVALID_PLAYBACK_ID;
	auto const iter = g_acbHandles.find(pTrigger->GetAcbId());

	if (iter != g_acbHandles.end())
	{
		criAtomExPlayer_SetCueName(m_pPlayer, iter->second, pTrigger->GetCueName());
		id = criAtomExPlayer_Start(m_pPlayer);
		pEvent->SetPlaybackId(id);
		// TODO: If a cue that needs a selector get started before the selector was set,
		// The event will stay in the debug draw indefinitely.
		// Find solution to solve this issue.
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, R"(Cue "%s" failed to play because ACB file "%s" was not loaded)", pTrigger->GetCueName(), pTrigger->GetCueSheetName());
	}

	if (id != CRIATOMEX_INVALID_PLAYBACK_ID)
	{
		g_mutex.Lock();
		g_activeEvents[id] = &pEvent->GetATLEvent();
		g_mutex.Unlock();

		pEvent->SetTriggerId(pTrigger->GetId());
		pEvent->SetObject(this);
		m_events.push_back(pEvent);
		eventStarted = true;
	}

	return eventStarted;
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::StopEvent(uint32 const triggerId)
{
	for (auto const pEvent : m_events)
	{
		if (pEvent->GetTriggerId() == triggerId)
		{
			pEvent->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::PauseEvent(uint32 const triggerId)
{
	for (auto const pEvent : m_events)
	{
		if (pEvent->GetTriggerId() == triggerId)
		{
			pEvent->Pause();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::ResumeEvent(uint32 const triggerId)
{
	for (auto const pEvent : m_events)
	{
		if (pEvent->GetTriggerId() == triggerId)
		{
			pEvent->Resume();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectBase::RemoveEvent(CEvent* const pEvent)
{
	if (!stl::find_and_erase(m_events, pEvent))
	{
		Cry::Audio::Log(ELogType::Error, "Tried to remove an event from an audio object that does not own that event");
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
