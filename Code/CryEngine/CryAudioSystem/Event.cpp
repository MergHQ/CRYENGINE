// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Object.h"
#include "Common/IEvent.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CEvent::Stop()
{
	m_pImplData->Stop();
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Release()
{
	m_pImplData = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_szTriggerName = nullptr;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CEvent::SetVirtual()
{
	m_state = EEventState::Virtual;

	if ((m_pObject->GetFlags() & EObjectFlags::Virtual) == 0)
	{
		bool isVirtual = true;

		for (auto const pEvent : m_pObject->GetActiveEvents())
		{
			if (pEvent->m_state != EEventState::Virtual)
			{
				isVirtual = false;
				break;
			}
		}

		if (isVirtual)
		{
			m_pObject->SetFlag(EObjectFlags::Virtual);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			m_pObject->ResetObstructionRays();
#endif      // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEvent::SetPlaying()
{
	m_state = EEventState::Playing;
	m_pObject->RemoveFlag(EObjectFlags::Virtual);
}
} // namespace CryAudio
