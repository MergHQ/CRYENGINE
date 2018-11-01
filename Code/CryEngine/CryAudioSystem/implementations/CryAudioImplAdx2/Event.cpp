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
CEvent::~CEvent()
{
	if (m_pObject != nullptr)
	{
		m_pObject->RemoveEvent(this);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Stop()
{
	criAtomExPlayback_Stop(m_playbackId);
	m_playbackId = CRIATOMEX_INVALID_PLAYBACK_ID;

	return ERequestStatus::Success;
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
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
