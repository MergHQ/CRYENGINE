// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CueInstance.h"
#include "Common.h"
#include "BaseObject.h"
#include "Cue.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
bool CCueInstance::PrepareForPlayback(CBaseObject* const pBaseObject)
{
	bool isPlaying = false;

	if (criAtomExPlayback_GetStatus(m_playbackId) == CRIATOMEXPLAYBACK_STATUS_PLAYING)
	{
		CriAtomExAcbHn const pAcbHandle = m_pCue->GetAcbHandle();
		auto const cueName = static_cast<CriChar8 const*>(m_pCue->GetName());
		CriAtomExCueInfo cueInfo;

		if (criAtomExAcb_GetCueInfoByName(pAcbHandle, cueName, &cueInfo) == CRI_TRUE)
		{
			if (cueInfo.pos3d_info.doppler_factor > 0.0f)
			{
				SetFlag(ECueInstanceFlags::HasDoppler);
			}
		}

		if (criAtomExAcb_IsUsingAisacControlByName(pAcbHandle, cueName, g_szAbsoluteVelocityAisacName) == CRI_TRUE)
		{
			SetFlag(ECueInstanceFlags::HasAbsoluteVelocity);
		}

		RemoveFlag(ECueInstanceFlags::IsPending);
		pBaseObject->UpdateVelocityTracking();

		isPlaying = true;
	}

	return isPlaying;
}

//////////////////////////////////////////////////////////////////////////
void CCueInstance::Stop()
{
	criAtomExPlayback_Stop(m_playbackId);
}

//////////////////////////////////////////////////////////////////////////
void CCueInstance::Pause()
{
	criAtomExPlayback_Pause(m_playbackId, CRI_TRUE);
}

//////////////////////////////////////////////////////////////////////////
void CCueInstance::Resume()
{
	criAtomExPlayback_Pause(m_playbackId, CRI_FALSE);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
