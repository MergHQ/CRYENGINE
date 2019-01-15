// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"

#include "Impl.h"
#include "Event.h"
#include "Trigger.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Listener.h"
#include "Cvars.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
	: m_flags(EObjectFlags::None)
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	, m_absoluteVelocity(0.0f)
	, m_absoluteVelocityNormalized(0.0f)
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
{
	ZeroStruct(m_3dAttributes);
	m_p3dSource = criAtomEx3dSource_Create(&g_3dSourceConfig, nullptr, 0);
	m_pPlayer = criAtomExPlayer_Create(&g_playerConfig, nullptr, 0);
	CRY_ASSERT_MESSAGE(m_pPlayer != nullptr, "m_pPlayer is null pointer during %s", __FUNCTION__);
	m_events.reserve(2);
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
void CBaseObject::Update(float const deltaTime)
{
	EObjectFlags const previousFlags = m_flags;
	bool removedEvent = false;

	if (!m_events.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	auto iter(m_events.begin());
	auto iterEnd(m_events.end());

	while (iter != iterEnd)
	{
		auto const pEvent = *iter;

		if ((pEvent->GetFlags() & EEventFlags::ToBeRemoved) != 0)
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEvent->GetTriggerInstanceId());
			g_pImpl->DestructEvent(pEvent);
			removedEvent = true;

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_events.back();
			}

			m_events.pop_back();
			iter = m_events.begin();
			iterEnd = m_events.end();
		}
		else
		{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
			// Always update in production code for debug draw.
			pEvent->UpdateVirtualState();

			if ((pEvent->GetFlags() & EEventFlags::IsVirtual) == 0)
			{
				m_flags &= ~EObjectFlags::IsVirtual;
			}
#else
			if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
			{
				pEvent->UpdateVirtualState();

				if ((pEvent->GetFlags() & EEventFlags::IsVirtual) == 0)
				{
					m_flags &= ~EObjectFlags::IsVirtual;
				}
			}
#endif      // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
			pEvent->UpdatePlaybackState();

			++iter;
		}
	}

	if ((previousFlags != m_flags) && !m_events.empty())
	{
		if (((previousFlags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::IsVirtual) == 0))
		{
			gEnv->pAudioSystem->ReportPhysicalizedObject(this);
		}
		else if (((previousFlags& EObjectFlags::IsVirtual) == 0) && ((m_flags& EObjectFlags::IsVirtual) != 0))
		{
			gEnv->pAudioSystem->ReportVirtualizedObject(this);
		}
	}

	if (removedEvent)
	{
		UpdateVelocityTracking();
	}
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
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddEvent(CEvent* const pEvent)
{
	pEvent->UpdateVirtualState();

	if ((m_flags& EObjectFlags::IsVirtual) != 0)
	{
		if ((pEvent->GetFlags() & EEventFlags::IsVirtual) == 0)
		{
			m_flags &= ~EObjectFlags::IsVirtual;
		}
	}
	else if (m_events.empty())
	{
		if ((pEvent->GetFlags() & EEventFlags::IsVirtual) != 0)
		{
			m_flags |= EObjectFlags::IsVirtual;
		}
	}

	m_events.push_back(pEvent);
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
		CriAtomExAcbHn const acbHandle = iter->second;
		CriChar8 const* const cueName = pTrigger->GetCueName();

		criAtomExPlayer_SetCueName(m_pPlayer, acbHandle, cueName);
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
					pEvent->SetObject(this);

					CriAtomExCueInfo cueInfo;

					if (criAtomExAcb_GetCueInfoByName(acbHandle, cueName, &cueInfo) == CRI_TRUE)
					{
						if (cueInfo.pos3d_info.doppler_factor > 0.0f)
						{
							pEvent->SetFlag(EEventFlags::HasDoppler);
						}
					}

					if (criAtomExAcb_IsUsingAisacControlByName(acbHandle, cueName, s_szAbsoluteVelocityAisacName) == CRI_TRUE)
					{
						pEvent->SetFlag(EEventFlags::HasAbsoluteVelocity);
					}

					AddEvent(pEvent);
					UpdateVelocityTracking();

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

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVelocityTracking()
{
	bool trackAbsoluteVelocity = false;
	bool trackDoppler = false;

	Events::iterator iter = m_events.begin();
	Events::const_iterator iterEnd = m_events.end();

	while ((iter != iterEnd) && !(trackAbsoluteVelocity && trackDoppler))
	{
		auto const pEvent = *iter;
		trackAbsoluteVelocity |= ((pEvent->GetFlags() & EEventFlags::HasAbsoluteVelocity) != 0);
		trackDoppler |= ((pEvent->GetFlags() & EEventFlags::HasDoppler) != 0);
		++iter;
	}

	if (trackAbsoluteVelocity)
	{
		if (g_cvars.m_maxVelocity > 0.0f)
		{
			m_flags |= EObjectFlags::TrackAbsoluteVelocity;
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Adx2 - Cannot enable absolute velocity tracking, because s_Adx2MaxVelocity is not greater than 0.");
		}
	}
	else
	{
		m_flags &= ~EObjectFlags::TrackAbsoluteVelocity;

		criAtomExPlayer_SetAisacControlByName(m_pPlayer, s_szAbsoluteVelocityAisacName, 0.0f);
		criAtomExPlayer_UpdateAll(m_pPlayer);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
		m_absoluteVelocity = 0.0f;
		m_absoluteVelocityNormalized = 0.0f;
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	}

	if (trackDoppler)
	{
		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) == 0)
		{
			m_flags |= EObjectFlags::TrackVelocityForDoppler;
			g_numObjectsWithDoppler++;
		}
	}
	else
	{
		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			m_flags &= ~EObjectFlags::TrackVelocityForDoppler;

			CriAtomExVector const zeroVelocity{ 0.0f, 0.0f, 0.0f };
			criAtomEx3dSource_SetVelocity(m_p3dSource, &zeroVelocity);
			criAtomEx3dSource_Update(m_p3dSource);

			CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists during %s", __FUNCTION__);
			g_numObjectsWithDoppler--;
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
