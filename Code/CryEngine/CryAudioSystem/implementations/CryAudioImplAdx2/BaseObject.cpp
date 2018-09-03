// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"

#include "Impl.h"
#include "Event.h"
#include "Trigger.h"
#include "Parameter.h"
#include "Listener.h"

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
	if (voiceEvent == CRIATOMEX_VOICE_EVENT_REMOVE)
	{
		g_mutex.Lock();
		CriAtomExPlaybackId const id = (pRemoved != nullptr) ? pRemoved->playback_id : pRequest->playback_id;
		auto const iter = g_activeEvents.find(id);

		if (iter != g_activeEvents.end())
		{
			auto const pAtlEvent = iter->second;
			g_activeEvents.erase(iter);
			g_mutex.Unlock();

			gEnv->pAudioSystem->ReportFinishedEvent(*pAtlEvent, true);
		}
		else
		{
			g_mutex.Unlock();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
{
	ZeroStruct(m_3dAttributes);
	m_p3dSource = criAtomEx3dSource_Create(&g_3dSourceConfig, nullptr, 0);
	m_pPlayer = criAtomExPlayer_Create(&g_playerConfig, nullptr, 0);
	CRY_ASSERT_MESSAGE(m_pPlayer != nullptr, "m_pPlayer is null pointer");
	m_events.reserve(2);
	criAtomEx_SetVoiceEventCallback(VoiceEventCallback, nullptr);
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::~CBaseObject()
{
	criAtomExPlayer_Destroy(m_pPlayer);

	for (auto const pEvent : m_events)
	{
		pEvent->SetObject(nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent)
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
			criAtomEx_ApplyDspBusSnapshot(pTrigger->GetCueName(), pTrigger->GetChangeoverTime());
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
void CBaseObject::StopAllTriggers()
{
	criAtomExPlayer_Stop(m_pPlayer);

	for (auto const pEvent : m_events)
	{
		pEvent->SetPlaybackId(CRIATOMEX_INVALID_PLAYBACK_ID);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::PlayFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::StopFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::StartEvent(CTrigger const* const pTrigger, CEvent* const pEvent)
{
	bool eventStarted = false;

	criAtomExPlayer_Set3dListenerHn(m_pPlayer, g_pListener->GetHandle());
	criAtomExPlayer_Set3dSourceHn(m_pPlayer, m_p3dSource);

	auto const iter = g_acbHandles.find(pTrigger->GetAcbId());

	if (iter != g_acbHandles.end())
	{
		criAtomExPlayer_SetCueName(m_pPlayer, iter->second, pTrigger->GetCueName());
		CriAtomExPlaybackId const id = criAtomExPlayer_Start(m_pPlayer);

		while (true)
		{
			// Loop is needed because callbacks don't work for events that fail to start.
			CriAtomExPlaybackStatus const status = criAtomExPlayback_GetStatus(id);

			if (status != CRIATOMEXPLAYBACK_STATUS_PREP)
			{
				if (status == CRIATOMEXPLAYBACK_STATUS_PLAYING)
				{
					pEvent->SetPlaybackId(id);
					pEvent->SetTriggerId(pTrigger->GetId());
					pEvent->SetObject(this);
					m_events.push_back(pEvent);

					g_mutex.Lock();
					g_activeEvents[id] = &pEvent->GetATLEvent();
					g_mutex.Unlock();

					eventStarted = true;
				}

				break;
			}
		}
	}
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, R"(Cue "%s" failed to play because ACB file "%s" was not loaded)", static_cast<char const*>(pTrigger->GetCueName()), pTrigger->GetCueSheetName());
	}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	return eventStarted;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEvent(uint32 const triggerId)
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
void CBaseObject::PauseEvent(uint32 const triggerId)
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
void CBaseObject::ResumeEvent(uint32 const triggerId)
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
void CBaseObject::RemoveEvent(CEvent* const pEvent)
{
	if (!stl::find_and_erase(m_events, pEvent))
	{
		Cry::Audio::Log(ELogType::Error, "Tried to remove an event from an object that does not own that event");
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::MutePlayer(CriBool const shouldMute)
{
	CriFloat32 const volume = (shouldMute == CRI_TRUE) ? 0.0f : 1.0f;
	criAtomExPlayer_SetVolume(m_pPlayer, volume);
	criAtomExPlayer_UpdateAll(m_pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::PausePlayer(CriBool const shouldPause)
{
	criAtomExPlayer_Pause(m_pPlayer, shouldPause);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
