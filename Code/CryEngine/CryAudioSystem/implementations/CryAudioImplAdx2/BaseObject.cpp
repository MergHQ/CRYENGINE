// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"

#include "Impl.h"
#include "Cue.h"
#include "CueInstance.h"
#include "Listener.h"
#include "Cvars.h"

#include <CryThreading/CryThread.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
CryCriticalSection g_cs;

//////////////////////////////////////////////////////////////////////////
static void ProcessCallback(CCueInstance* const pCueInstance, CriAtomExPlaybackEvent const playbackEvent)
{
	switch (playbackEvent)
	{
	case CriAtomExPlaybackEvent::CRIATOMEX_PLAYBACK_EVENT_FROM_NORMAL_TO_VIRTUAL:
		{
			pCueInstance->SetFlag(ECueInstanceFlags::IsVirtual);

			break;
		}
	case CriAtomExPlaybackEvent::CRIATOMEX_PLAYBACK_EVENT_FROM_VIRTUAL_TO_NORMAL:
		{
			pCueInstance->RemoveFlag(ECueInstanceFlags::IsVirtual);

			break;
		}
	case CriAtomExPlaybackEvent::CRIATOMEX_PLAYBACK_EVENT_REMOVE:
		{
			pCueInstance->SetFlag(ECueInstanceFlags::ToBeRemoved);

			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
static void PlaybackEventCallback(void* pObject, CriAtomExPlaybackEvent playbackEvent, CriAtomExPlaybackInfoDetail const* pInfo)
{
	if ((playbackEvent != CriAtomExPlaybackEvent::CRIATOMEX_PLAYBACK_EVENT_ALLOCATE) && (pObject != nullptr))
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pObject);

		{
			CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Adx2::g_cs);

			CueInstances const& pendingCueInstances = pBaseObject->GetPendingCueInstances();

			for (auto const pPendingCueInstance : pendingCueInstances)
			{
				if (pPendingCueInstance->GetPlaybackId() == pInfo->id)
				{
					ProcessCallback(pPendingCueInstance, playbackEvent);

					break;
				}
			}

			CueInstances const& cueInstances = pBaseObject->GetCueInstances();

			for (auto const pCueInstance : cueInstances)
			{
				if (pCueInstance->GetPlaybackId() == pInfo->id)
				{
					ProcessCallback(pCueInstance, playbackEvent);

					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
	: m_flags(EObjectFlags::IsVirtual) // Set to virtual because voices always start in virtual state.
{
	ZeroStruct(m_3dAttributes);
	m_p3dSource = criAtomEx3dSource_Create(&g_3dSourceConfig, nullptr, 0);
	m_pPlayer = criAtomExPlayer_Create(&g_playerConfig, nullptr, 0);
	criAtomExPlayer_SetPlaybackEventCallback(m_pPlayer, PlaybackEventCallback, this);
	CRY_ASSERT_MESSAGE(m_pPlayer != nullptr, "m_pPlayer is null pointer during %s", __FUNCTION__);
	m_cueInstances.reserve(2);
	m_pendingCueInstances.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::~CBaseObject()
{
	criAtomExPlayer_Destroy(m_pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Update(float const deltaTime)
{
	EObjectFlags const previousFlags = m_flags;

	if (!m_pendingCueInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;

		auto iter(m_pendingCueInstances.begin());
		auto iterEnd(m_pendingCueInstances.end());

		while (iter != iterEnd)
		{
			CCueInstance* const pCueInstance = *iter;

			if (pCueInstance->PrepareForPlayback())
			{
				AddCueInstance(pCueInstance);
				UpdateVelocityTracking();

				if (iter != (iterEnd - 1))
				{
					(*iter) = m_pendingCueInstances.back();
				}

				m_pendingCueInstances.pop_back();
				iter = m_pendingCueInstances.begin();
				iterEnd = m_pendingCueInstances.end();
			}
			else
			{
				UpdateVirtualState(pCueInstance);

				++iter;
			}
		}
	}

	bool removedCueInstance = false;

	if (!m_cueInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;

		auto iter(m_cueInstances.begin());
		auto iterEnd(m_cueInstances.end());

		while (iter != iterEnd)
		{
			CCueInstance* const pCueInstance = *iter;

			if ((pCueInstance->GetFlags() & ECueInstanceFlags::ToBeRemoved) != 0)
			{
				gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pCueInstance->GetTriggerInstanceId());
				g_pImpl->DestructCueInstance(pCueInstance);
				removedCueInstance = true;

				if (iter != (iterEnd - 1))
				{
					(*iter) = m_cueInstances.back();
				}

				m_cueInstances.pop_back();
				iter = m_cueInstances.begin();
				iterEnd = m_cueInstances.end();
			}
			else
			{
				UpdateVirtualState(pCueInstance);

				++iter;
			}
		}
	}

	if ((previousFlags != m_flags) && !m_cueInstances.empty())
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

	if (removedCueInstance)
	{
		UpdateVelocityTracking();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVirtualState(CCueInstance* const pCueInstance)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	// Always update in production code for debug draw.
	if ((pCueInstance->GetFlags() & ECueInstanceFlags::IsVirtual) == 0)
	{
		m_flags &= ~EObjectFlags::IsVirtual;
	}
#else
	if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
	{
		if ((pCueInstance->GetFlags() & ECueInstanceFlags::IsVirtual) == 0)
		{
			m_flags &= ~EObjectFlags::IsVirtual;
		}
	}
#endif      // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopAllTriggers()
{
	criAtomExPlayer_Stop(m_pPlayer);

	for (auto const pCueInstance : m_cueInstances)
	{
		pCueInstance->SetFlag(ECueInstanceFlags::ToBeRemoved);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	m_name = szName;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddCueInstance(CCueInstance* const pCueInstance)
{
	if ((pCueInstance->GetFlags() & ECueInstanceFlags::IsVirtual) == 0)
	{
		m_flags &= ~EObjectFlags::IsVirtual;
	}
	else if (m_cueInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	m_cueInstances.push_back(pCueInstance);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddPendingCueInstance(CCueInstance* const pCueInstance)
{
	m_pendingCueInstances.push_back(pCueInstance);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopCue(uint32 const cueId)
{
	for (auto const pCueInstance : m_cueInstances)
	{
		if (pCueInstance->GetCueId() == cueId)
		{
			pCueInstance->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::PauseCue(uint32 const cueId)
{
	for (auto const pCueInstance : m_cueInstances)
	{
		if (pCueInstance->GetCueId() == cueId)
		{
			pCueInstance->Pause();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::ResumeCue(uint32 const cueId)
{
	for (auto const pCueInstance : m_cueInstances)
	{
		if (pCueInstance->GetCueId() == cueId)
		{
			pCueInstance->Resume();
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
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
