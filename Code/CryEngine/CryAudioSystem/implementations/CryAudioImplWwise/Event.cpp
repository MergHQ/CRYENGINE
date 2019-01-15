// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Object.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CEvent::Stop()
{
	switch (m_state)
	{
	case EEventState::Playing:
	case EEventState::Virtual:
		{
			AK::SoundEngine::StopPlayingID(m_id, 10);
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
void CEvent::UpdateVirtualState()
{
	m_state = ((m_maxAttenuation > 0.0f) && (m_maxAttenuation < m_pObject->GetDistanceToListener())) ? EEventState::Virtual : EEventState::Playing;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
