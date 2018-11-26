// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "BaseObject.h"
#include "Event.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, IEvent* const pIEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if ((pIObject != nullptr) && (pIEvent != nullptr))
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);
		auto const pEvent = static_cast<CEvent*>(pIEvent);

		switch (m_triggerType)
		{
		case ETriggerType::Trigger:
			{
				switch (m_eventType)
				{
				case EEventType::Start:
					{
						if (pObject->StartEvent(this, pEvent))
						{
							requestResult = ERequestStatus::Success;
						}

						break;
					}
				case EEventType::Stop:
					{
						pObject->StopEvent(m_id);
						requestResult = ERequestStatus::SuccessDoNotTrack;

						break;
					}
				case EEventType::Pause:
					{
						pObject->PauseEvent(m_id);
						requestResult = ERequestStatus::SuccessDoNotTrack;
						break;
					}
				case EEventType::Resume:
					{
						pObject->ResumeEvent(m_id);
						requestResult = ERequestStatus::SuccessDoNotTrack;

						break;
					}
				}

				break;
			}
		case ETriggerType::Snapshot:
			{
				criAtomEx_ApplyDspBusSnapshot(static_cast<CriChar8 const*>(m_cueName), m_changeoverTime);
				requestResult = ERequestStatus::SuccessDoNotTrack;

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown ETriggerType: %" PRISIZE_T, m_triggerType);

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object or event pointer passed to the Adx2 implementation of %s.", __FUNCTION__);
	}

	return requestResult;
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
