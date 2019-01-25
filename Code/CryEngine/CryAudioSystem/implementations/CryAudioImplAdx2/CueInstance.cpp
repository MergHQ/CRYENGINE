// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CueInstance.h"

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
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

//////////////////////////////////////////////////////////////////////////
void CCueInstance::UpdateVirtualState()
{
	// TODO: Needs implementation when we can check the virtual state or audibility.
}

//////////////////////////////////////////////////////////////////////////
void CCueInstance::UpdatePlaybackState()
{
	// Workaround for missing end callback.
	if (criAtomExPlayback_GetStatus(m_playbackId) == CRIATOMEXPLAYBACK_STATUS_REMOVED)
	{
		m_flags |= ECueInstanceFlags::ToBeRemoved;
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
