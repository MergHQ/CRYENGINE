// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CEvent::Stop()
{
	criAtomExPlayback_Stop(m_playbackId);
	m_playbackId = CRIATOMEX_INVALID_PLAYBACK_ID;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Pause()
{
	criAtomExPlayback_Pause(m_playbackId, CRI_TRUE);
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Resume()
{
	criAtomExPlayback_Pause(m_playbackId, CRI_FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CEvent::UpdateVirtualState()
{
	// TODO: Needs implementation when we can check the virtual state or audibility.
}

//////////////////////////////////////////////////////////////////////////
void CEvent::UpdatePlaybackState()
{
	// Workaround for missing end callback.
	if (criAtomExPlayback_GetStatus(m_playbackId) == CRIATOMEXPLAYBACK_STATUS_REMOVED)
	{
		m_flags |= EEventFlags::ToBeRemoved;
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
