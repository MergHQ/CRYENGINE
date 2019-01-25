// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CueInstance.h"

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
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
