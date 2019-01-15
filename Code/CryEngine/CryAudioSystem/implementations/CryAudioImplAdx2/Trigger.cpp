// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "BaseObject.h"
#include "Event.h"
#include "Impl.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);

		switch (m_triggerType)
		{
		case ETriggerType::Trigger:
			{
				switch (m_actionType)
				{
				case EActionType::Start:
					{
						auto const pEvent = g_pImpl->ConstructEvent(triggerInstanceId);

						if (pObject->StartEvent(this, pEvent))
						{
							pEvent->SetTriggerId(m_id);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
							pEvent->SetName(m_cueName.c_str());
#endif              // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

							requestResult = ((pEvent->GetFlags() & EEventFlags::IsVirtual) != 0) ? ERequestStatus::SuccessVirtual : ERequestStatus::Success;
						}

						break;
					}
				case EActionType::Stop:
					{
						pObject->StopEvent(m_id);
						requestResult = ERequestStatus::SuccessDoNotTrack;

						break;
					}
				case EActionType::Pause:
					{
						pObject->PauseEvent(m_id);
						requestResult = ERequestStatus::SuccessDoNotTrack;
						break;
					}
				case EActionType::Resume:
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

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);
		pObject->StopEvent(m_id);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
