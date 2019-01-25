// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EventInstance.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
constexpr AkTimeMs g_fadeDuration = 10;

//////////////////////////////////////////////////////////////////////////
void CEventInstance::Stop()
{
	switch (m_state)
	{
	case EEventInstanceState::Playing: // Intentional fall-through.
	case EEventInstanceState::Virtual:
		{
			AK::SoundEngine::StopPlayingID(m_playingId, g_fadeDuration);
			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(false, "Unsupported event state during %s", __FUNCTION__);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::UpdateVirtualState(float const distance)
{
	m_state = ((m_maxAttenuation > 0.0f) && (m_maxAttenuation < distance)) ? EEventInstanceState::Virtual : EEventInstanceState::Playing;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
