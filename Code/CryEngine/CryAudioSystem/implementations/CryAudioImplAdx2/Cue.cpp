// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Cue.h"
#include "BaseObject.h"
#include "CueInstance.h"
#include "Impl.h"
#include "Listener.h"

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
ETriggerResult CCue::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ETriggerResult result = ETriggerResult::Failure;

	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);

	switch (m_actionType)
	{
	case EActionType::Start:
		{
			CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();

			criAtomExPlayer_Set3dListenerHn(pPlayer, g_pListener->GetHandle());
			criAtomExPlayer_Set3dSourceHn(pPlayer, pBaseObject->Get3dSource());

			auto const iter = g_acbHandles.find(m_cueSheetId);

			if (iter != g_acbHandles.end())
			{
				m_pAcbHandle = iter->second;
				auto const cueName = static_cast<CriChar8 const*>(m_name.c_str());

				criAtomExPlayer_SetCueName(pPlayer, m_pAcbHandle, cueName);
				CriAtomExPlaybackId const playbackId = criAtomExPlayer_Start(pPlayer);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
				auto const pCueInstance = g_pImpl->ConstructCueInstance(triggerInstanceId, m_id, playbackId, this, pBaseObject);
#else
				auto const pCueInstance = g_pImpl->ConstructCueInstance(triggerInstanceId, m_id, playbackId, this);
#endif                // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

				if (criAtomExPlayback_GetStatus(playbackId) == CRIATOMEXPLAYBACK_STATUS_PLAYING)
				{
					pBaseObject->AddCueInstance(pCueInstance);
					result = ((pCueInstance->GetFlags() & ECueInstanceFlags::IsVirtual) != 0) ? ETriggerResult::Virtual : ETriggerResult::Playing;
				}
				else
				{
					pCueInstance->SetFlag(ECueInstanceFlags::IsPending);
					pBaseObject->AddPendingCueInstance(pCueInstance);
					result = ETriggerResult::Pending;
				}
			}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, R"(Cue "%s" failed to play because ACB file "%s" was not loaded)",
				                static_cast<char const*>(m_name), static_cast<char const*>(m_cueSheetName));
			}
#endif        // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

			break;
		}
	case EActionType::Stop:
		{
			pBaseObject->StopCue(m_id);
			result = ETriggerResult::DoNotTrack;

			break;
		}
	case EActionType::Pause:
		{
			pBaseObject->PauseCue(m_id);
			result = ETriggerResult::DoNotTrack;
			break;
		}
	case EActionType::Resume:
		{
			pBaseObject->ResumeCue(m_id);
			result = ETriggerResult::DoNotTrack;

			break;
		}
	default:
		{
			break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CCue::Stop(IObject* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->StopCue(m_id);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
