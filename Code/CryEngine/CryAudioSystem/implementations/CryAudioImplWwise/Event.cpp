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
			// Stopping an event of this type is not supported!
			CRY_ASSERT(false);
			break;
		}
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::SetInitialVirtualState(float const distance)
{
	m_state = ((m_maxAttenuation > 0.0f) && (m_maxAttenuation < distance)) ? EEventState::Virtual : EEventState::Playing;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::UpdateVirtualState(float const distance)
{
	EEventState const state = ((m_maxAttenuation > 0.0f) && (m_maxAttenuation < distance)) ? EEventState::Virtual : EEventState::Playing;

	if (m_state != state)
	{
		m_state = state;

		if (m_state == EEventState::Virtual)
		{
			gEnv->pAudioSystem->ReportVirtualizedEvent(m_event);
		}
		else
		{
			gEnv->pAudioSystem->ReportPhysicalizedEvent(m_event);
		}
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
